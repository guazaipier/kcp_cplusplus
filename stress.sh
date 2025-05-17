!/bin/bash

echo "Stress testing KCP..."
for i in `seq 1 10000`
do
    echo "Running test $i..."
    nohup ./main  >> /dev/null 2>&1 &
done
