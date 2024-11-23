#!/bin/bash
make all
wait
# Check if the producer-Consumer exists 
if [ ! -f producer-Consumer ]; then
    echo "producer-Consumer not found!"
    exit 1
fi
./producer-Consumer 2 2 3  2>&1 | tee pc1.txt
wait
./producer-Consumer 2 2 10 2>&1 | tee   pc2.txt
wait
./producer-Consumer 2 5 3  2>&1 | tee pc3.txt
wait
./producer-Consumer 2 5 10 2>&1 | tee   pc4.txt
wait 
./producer-Consumer 2 10 3 2>&1 | tee   pc5.txt
wait 
./producer-Consumer 2 10 10 2>&1 | tee   pc6.txt
wait 
./producer-Consumer 5 2 3  2>&1 | tee pc7.txt
wait 
./producer-Consumer 5 2 10 2>&1 | tee   pc8.txt
wait 
./producer-Consumer 5 5 3  2>&1 | tee pc9.txt
wait 
./producer-Consumer 5 5 10 2>&1 | tee   pc10.txt
wait 
./producer-Consumer 5 10 3 2>&1 | tee   pc11.txt
wait 
./producer-Consumer 5 10 10 2>&1 | tee   pc12.txt
wait 
./producer-Consumer 10 2 3 2>&1 | tee   pc13.txt
wait 
./producer-Consumer 10 2 10 2>&1 | tee   pc14.txt
wait 
./producer-Consumer 10 5 3 2>&1 | tee   pc15.txt
wait 
./producer-Consumer 10 5 10 2>&1 | tee   pc16.txt
wait 
./producer-Consumer 10 10 3 2>&1 | tee   pc17.txt
wait 
./producer-Consumer 10 10 10 2>&1 | tee   pc18.txt
wait

# Check if the file exists 
if [ ! -f time.txt ]; then
    echo "File time.txt not found!"
    exit 1
fi

cat time.txt