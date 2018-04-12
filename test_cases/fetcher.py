import pdb
import asyncio 
import pipeline.aws as aws
import pipeline
import aiobotocore
import pymongo 
import sys
from datetime import datetime, timedelta
from time import time, sleep
from logger import getLogger 
from config import config 
from pipeline.helper import time_to_epoch

logger = getLogger(__name__)

async def fetcher_setup(loop, mdb, crunch_queue, crunch_job_set):
    global fetch_queue
    fetch_queue = asyncio.PriorityQueue(loop=loop)

    logger.log("setup fetchers")
    logger.log("populating work queue with %d jobs" % len(aws.availability_zones))
    for job in aws.availability_zones:
        job = (0, job["region"], job["az"])
        await fetch_queue.put(job)

    logger.log("Starting %d workers" % config["Worker"]["FetchWorkers"])
    for wid in range(0, config["Worker"]["FetchWorkers"]):
        asyncio.ensure_future(fetcher(loop, wid, mdb, crunch_queue, crunch_job_set))
    
    # sys.exit(0)

async def fetcher(loop, worker_id, mdb, crunch_queue, crunch_job_set):
    logger = getLogger(__name__ + ".%d" % worker_id)
    logger.log("Started worker %d" % worker_id)

    while True:
        lastrun, region, az = await fetch_queue.get()
        job_str = "(%s, %s)" % (region, az)

        logger.log("Fetched job %s" % job_str)
        if lastrun >= time() - config["Worker"]["PollingRate"]:
            logger.log("Waiting %d seconds before running" % (lastrun - (time() - config["Worker"]["PollingRate"])))
            await asyncio.sleep(lastrun - (time() - config["Worker"]["PollingRate"]))
            logger.log("Running job %s" % job_str)
        else:
            logger.log("Running job immediately.")

        # clear any partial fetches -- we don't want them
        record = await mdb.keyvalue.find({
            "Region": region,
            "AvailabilityZone": az,
            "Key": "most_recent_history"
        }).limit(1).to_list(None)
        if len(record) == 1:
            deleted_count = (await mdb.history.delete_many({
                "Region": region,
                "AvailabilityZone": az,
                "Timestamp": {
                    "$lt": record[0]["Value"]["MostRecentTimestamp"],
                }
            })).deleted_count
            logger.log("%s cleared out %d history records that were not acknoledged as fully fetched" 
                % (job_str, deleted_count))
        else:
            deleted_count = (await mdb.history.delete_many({
                "Region": region,
                "AvailabilityZone": az,
            })).deleted_count
            logger.log("%s cleared out %d (all) history records that were not acknoledged as fully fetched" 
                % (job_str, deleted_count))

        # find the new most recent fetched record and use it to compute the window to fetch
        newest_record = await mdb.history.find({
            "Region": region,
            "AvailabilityZone": az,
        }).sort("Timestamp", -1).limit(1).to_list(None)

        logger.log("%s determined the most recent history record to be %r" % (job_str, newest_record))
        
        if len(newest_record) == 0:
            window_start = datetime.now() - timedelta(days=config["Worker"]["FetchBacklogDays"])
        else:
            window_start = datetime.utcfromtimestamp(newest_record[0]["Timestamp"])
        
        window_end = datetime.now()
        window_start_epoch = time_to_epoch(window_start)
        window_end_epoch = time_to_epoch(window_end)

        logger.log("%s Determined to fetch history in time window %r - %r" 
                    % (job_str, window_start.strftime("%Y-%m-%dT%H:%M:%S"), 
                    window_end.strftime("%Y-%m-%dT%H:%M:%S")))

        records_out_bounds = 0
        records_total = 0
        new_records = []
        try:
            async for record in get_spot_price_history(loop, region, az, window_start, window_end):
                records_total += 1
                if record["Timestamp"] <= window_start_epoch or record["Timestamp"] > window_end_epoch:
                    # we throw out any data from before the window requested
                    records_out_bounds += 1
                else:
                    # add the record to the pending transaction, and batch commit
                    # if we have more than 1024 records to write
                    record["Predicted"] = False # has it been used to generate a prediction...
                    # NOTE: this predicted field does not appear to be used for the moment
                    new_records.append(record)
                    if len(new_records) >= 1024 * 4:
                        logger.log("%s\t%d history records inserted. Current timestamp %s" 
                            % (job_str, len(new_records), datetime.utcfromtimestamp(new_records[-1]["Timestamp"]).strftime("%Y-%m-%dT%H:%M:%S")))
                        await mdb.history.insert_many(new_records)
                        new_records = []
        except Exception as e:
            logger.log("%s ERROR!!! Encountered exception %r" % (job_str, e))
            fetch_queue.put((time() + 60, region, az))
            continue
        
        # skip any records that are out of bounds 
        logger.log("%s fetched %d records in total" % (job_str, records_total))
        logger.log("%s skipped %d/%d records because they were out of bounds" % (job_str, records_out_bounds, records_total))

        if len(new_records) > 0:
            await mdb.history.insert_many(new_records)
            logger.log("%s\t%d history records inserted" % (job_str, len(new_records)))
            new_records = []

        # update the key value store's record of the most recent timestamp fetched for the az etc
        most_recent_history = (await mdb.history.find({
            "Region": region,
            "AvailabilityZone": az,
        }).sort("Timestamp", 1).limit(1).to_list(None))[0]
        
        logger.log("%s updating mdb.keyvalue[most_recent_history] to reflect new most recent history %r" 
            % (job_str, most_recent_history))
        await mdb.keyvalue.update_one({
            "Region": region,
            "AvailabilityZone": az,
            "Key": "most_recent_history"
        }, {
            "$set": {
                "Value": {
                    "MostRecentTimestamp": most_recent_history["Timestamp"]
                }
            }
        }, upsert=True)

        logger.log("%s done fetching, creating jobs for az's with unprocessed records" % (job_str))

        cursor = await mdb.history.find({
            "Region": region,
            "AvailabilityZone": az
        }).distinct("InstanceType")

        for inst_type in cursor:
            history_timestamp = (await mdb.history.find({
                "Region": region,
                "AvailabilityZone": az,
                "InstanceType": inst_type
            }).sort("Timestamp", -1).limit(1).to_list(None))[0]["Timestamp"]

            prediction_timestamp = (await mdb.prediction.find({
                "Region": region,
                "AvailabilityZone": az,
                "InstanceType": inst_type,
            }).sort("Timestamp", -1).limit(1).to_list(None))

            if len(prediction_timestamp) > 0:
                prediction_timestamp = prediction_timestamp[0]["Timestamp"]
            else:
                prediction_timestamp = -1

            logger.log("%s\t most recent history for %s is %d, most recent prediction is %d" % 
                    (job_str, inst_type, history_timestamp, prediction_timestamp))
            
            job = (region, az, inst_type)

            if prediction_timestamp < history_timestamp and (job not in crunch_job_set):
                history_count = await mdb.history.find({
                    "Region": region,
                    "AvailabilityZone": az,
                    "InstanceType": inst_type
                }).sort("Timestamp", -1).count()
                
                if history_count > config["Worker"]["MinimumHistoryBacklogToProcess"]:

                    logger.log("%s\t\t adding job for %s since pred time < history time" % (job_str, inst_type))

                    crunch_job_set.add(job)
                    await crunch_queue.put(job)
                
                else:
                    logger.log("%s\t\t skipping job, not enough history (only %d records)" % (job_str, history_count))

        logger.log("Done.")

        await fetch_queue.put((time(), region, az))

async def get_spot_price_history(loop, region, az, start_time, end_time):
    session = aiobotocore.get_session(loop=loop)
    async with session.create_client("ec2", region_name=region,
            aws_access_key_id=config["AWS"]["AccessKeyId"],
            aws_secret_access_key=config["AWS"]["SecretAccessKey"]) as client:
        
        # TODO: add a try except here for the sake of you know... error handling... 

        pag = client.get_paginator("describe_spot_price_history")

        params = {
            "EndTime": end_time.strftime("%Y-%m-%dT%H:%M:%S"),
            "StartTime": start_time.strftime("%Y-%m-%dT%H:%M:%S"),
            "ProductDescriptions": ["Linux/UNIX"],
            "AvailabilityZone": az,
        }
        
        iterator = pag.paginate(**params)
        spot_prices = []
        async for page in iterator:
            for spotdata in page["SpotPriceHistory"]:
                yield {
                    "Region": region,
                    "AvailabilityZone": az,
                    "InstanceType": spotdata["InstanceType"],
                    "ProductDescription": spotdata["ProductDescription"],
                    "SpotPrice": float(spotdata["SpotPrice"]),
                    "Timestamp": time_to_epoch(spotdata["Timestamp"])
                }
