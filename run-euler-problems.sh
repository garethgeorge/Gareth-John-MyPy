cd ./build_archive_profiled/ 
for BIN in *; do 
    echo "Running binary $BIN"
    rm -f ./profiling_data.txt
    for FILE in ../test/euler_problems/*.py; do 
        echo "Running euler problem $FILE"
        time ./$BIN < $FILE 2>> ../profiling_data/$BIN.profiling
    done 
done
cd .. 
