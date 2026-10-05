#!/bin/bash

N=42000
THREADS=16
REPEATS=3

METHODS=("row" "column")
SCHEDULES=("static" "dynamic" "guided")

echo "Method Schedule Run Time"

for method in "${METHODS[@]}"
do
    for schedule in "${SCHEDULES[@]}"
    do
        for run in $(seq 1 $REPEATS)
        do
            output=$(OMP_NUM_THREADS=$THREADS \
                     OMP_SCHEDULE=$schedule \
                     ./ex4 $N $method)

            time=$(echo "$output" | grep "Time:" | awk '{print $2}')

            echo "$method $schedule $run $time"
        done
    done
done
