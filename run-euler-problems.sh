cd build 

rm ./profiling_data.txt

for FILE in ../test/euler_problems/*.py; do 
    echo "Running euler problem $FILE"
    time ./mypy < $FILE 
done 