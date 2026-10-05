#!/usr/bin/env bash
set -euo pipefail

EXE="${1:-./game_of_life_omp}"

THREADS=(1 2 4 8 16)
SIZES=(64 1024 4096)
STEPS=(1000 2000)
REPEATS=5

OUT="results_raw.csv"

if [[ ! -x "$EXE" ]]; then
    echo "Error: executable '$EXE' not found or not executable."
    echo "Compile first, for example:"
    echo "  gcc -O3 -std=c11 -fopenmp Game_Of_Life_OpenMP.c -o game_of_life_omp"
    exit 1
fi

echo "size,steps,threads,run,time_seconds" > "$OUT"

export OMP_PROC_BIND=true
export OMP_PLACES=cores

for size in "${SIZES[@]}"; do
    for steps in "${STEPS[@]}"; do
        for threads in "${THREADS[@]}"; do
            export OMP_NUM_THREADS="$threads"

            for run in $(seq 1 "$REPEATS"); do
                echo "Running: size=$size steps=$steps threads=$threads run=$run/$REPEATS"

                output=$("$EXE" "$size" "$steps")
                time_sec=$(echo "$output" | awk '/GameOfLife:/ {print $NF}')

                if [[ -z "$time_sec" ]]; then
                    echo "Error: could not parse runtime from:"
                    echo "$output"
                    exit 1
                fi

                echo "$size,$steps,$threads,$run,$time_sec" >> "$OUT"
            done
        done
    done
done

echo
echo "Finished."
echo "Raw results: $OUT"
echo "Now run:"
echo "  python3 plot_results.py $OUT"
