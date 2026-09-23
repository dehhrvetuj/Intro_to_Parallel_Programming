#!/usr/bin/env bash

set -e

PROGRAM="./integration"
REPETITIONS=5

THREADS=(1 2 4 8 16)
TRAPEZOIDS=(1 100 10000 1000000 100000000)
CHUNK_SIZES=(default 1)

RAW_FILE="results_raw.csv"
RESULT_FILE="results_average.csv"
TEMP_FILE="results_average.tmp"

if [ ! -x "$PROGRAM" ]; then
    echo "Error: $PROGRAM does not exist or is not executable."
    echo "Compile the program first:"
    echo "g++ -O2 -Wall -Wextra -std=c++11 -pthread integration.cpp -o integration"
    exit 1
fi

echo "method,trapezoids,threads,chunk_size,run,time,integral" > "$RAW_FILE"

run_test()
{
    method=$1
    trapezoids=$2
    threads=$3
    chunk_size=$4
    run=$5

    if [ "$method" = "block" ]; then
        output=$($PROGRAM -t "$threads" -n "$trapezoids" -d block)
    elif [ "$chunk_size" = "default" ]; then
        output=$($PROGRAM -t "$threads" -n "$trapezoids" -d dynamic)
    else
        output=$($PROGRAM -t "$threads" -n "$trapezoids" -d dynamic -c "$chunk_size")
    fi

    time_value=$(echo "$output" | awk '/^Runtime:/ {print $2}')
    integral=$(echo "$output" | awk '/^Computed integral:/ {print $3}')

    if [ -z "$time_value" ] || [ -z "$integral" ]; then
        echo "Error: cannot read program output."
        echo "$output"
        exit 1
    fi

    echo "$method,$trapezoids,$threads,$chunk_size,$run,$time_value,$integral" >> "$RAW_FILE"

    echo "$method  n=$trapezoids  threads=$threads  chunk=$chunk_size  run=$run  time=$time_value"
}

# Run block distribution experiments.
for n in "${TRAPEZOIDS[@]}"
do
    for p in "${THREADS[@]}"
    do
        for run in $(seq 1 $REPETITIONS)
        do
            run_test block "$n" "$p" none "$run"
        done
    done
done

# Run dynamic distribution experiments.
for n in "${TRAPEZOIDS[@]}"
do
    for p in "${THREADS[@]}"
    do
        for chunk in "${CHUNK_SIZES[@]}"
        do
            for run in $(seq 1 $REPETITIONS)
            do
                run_test dynamic "$n" "$p" "$chunk" "$run"
            done
        done
    done
done

# Calculate the average time and integral for each configuration.
awk -F, '
NR > 1
{
    key = $1 "," $2 "," $3 "," $4;
    total_time[key] += $6;
    total_integral[key] += $7;
    count[key]++;
}
END
{
    for (key in count)
    {
        print key "," total_time[key] / count[key] "," total_integral[key] / count[key];
    }
}
' "$RAW_FILE" | sort -t, -k1,1 -k2,2n -k4,4 -k3,3n > "$TEMP_FILE"

# Calculate speedup relative to the one-thread result of the same method,
# trapezoid count and chunk size.
echo "method,trapezoids,threads,chunk_size,average_time,average_integral,speedup" > "$RESULT_FILE"

awk -F, '
NR == FNR
{
    if ($3 == 1)
    {
        key = $1 "," $2 "," $4;
        one_thread_time[key] = $5;
    }
    next;
}
{
    key = $1 "," $2 "," $4;
    speedup = one_thread_time[key] / $5;
    print $0 "," speedup;
}
' "$TEMP_FILE" "$TEMP_FILE" >> "$RESULT_FILE"

rm "$TEMP_FILE"

echo
echo "Experiments completed."
echo "Raw data:     $RAW_FILE"
echo "Average data: $RESULT_FILE"
