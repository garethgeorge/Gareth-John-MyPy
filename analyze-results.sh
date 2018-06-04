for FILE in ./profiling_data/*.profiling; do
    echo "Running analysis on file $FILE"
    python ./pytools/profiling-data-analyser-v2.py $FILE > $FILE.agg.txt
done 