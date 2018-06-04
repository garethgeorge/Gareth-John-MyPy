from collections import defaultdict 
import sys
from tqdm import tqdm

def get_raw_events(file):
    events = []
    with open(file, "r") as f:
        for line in f:
            if line[0] != "\t": continue

            for line in f:
                colon = line.index(':') 
                data = [x.strip() for x in line[colon+1:].split(",")]

                if line.startswith("\tOPCODE,"):
                    # events.append(("OP", data))
                    yield ("OP", data)
                elif line.startswith("\tGC START"):
                    # events.append(("GC START", data))
                    yield (("GC START", data))
                elif line.startswith("\tGC END"):
                    # events.append(("GC END", data))
                    yield (("GC END", data))
                elif line.startswith("Dump End Time"):
                    # events.append(("DUMP END", None))
                    yield (("DUMP END", None))
                    break 
                else:
                    print(line)
                    raise Exception("unknown line contents")
    return events

def group_by_dump(event_stream):
    events = []
    for event in tqdm(event_stream):
        if event[0] == "DUMP END":
            yield events 
            events = []
        else:
            events.append(event)

def get_time_deltas(event_stream):
    def get_ts(event):
        return int(event[1][-1])

    last_event = next(event_stream)
    last_event_ts = get_ts(last_event)
    for event in event_stream:
        if event[1] is None: continue 

        event_ts = get_ts(event)
        delta = event_ts - last_event_ts 

        if last_event[0] == "OP": 
            yield "OP", last_event[1][0], delta
        else:
            yield last_event[0], None, None
        last_event = event 
        last_event_ts = event_ts

print("running! input file: " + sys.argv[1])

counts = defaultdict(int) 
durations = defaultdict(int)
generator = get_raw_events(sys.argv[1])
for events in group_by_dump(generator):
    for type, op, delta in get_time_deltas(iter(events)):
        if type == "OP":
            counts[op] += 1
            durations[op] += delta

for key in counts:
    print("%s - %d - %f" % (key, counts[key], float(durations[key]) / float(counts[key])))
print("Done.")