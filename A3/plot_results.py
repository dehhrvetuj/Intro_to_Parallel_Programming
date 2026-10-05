#!/usr/bin/env python3
import csv
import sys
from collections import defaultdict
from statistics import mean, stdev
from pathlib import Path

import matplotlib.pyplot as plt

csv_path = Path(sys.argv[1] if len(sys.argv) > 1 else "results_raw.csv")

if not csv_path.exists():
    raise SystemExit(f"File not found: {csv_path}")

data = defaultdict(list)

with csv_path.open(newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        key = (int(row["size"]), int(row["steps"]), int(row["threads"]))
        data[key].append(float(row["time_seconds"]))

summary = []
for (size, steps, threads), times in sorted(data.items()):
    summary.append({
        "size": size,
        "steps": steps,
        "threads": threads,
        "mean_time": mean(times),
        "stdev_time": stdev(times) if len(times) > 1 else 0.0,
    })

baseline = {}
for row in summary:
    if row["threads"] == 1:
        baseline[(row["size"], row["steps"])] = row["mean_time"]

for row in summary:
    key = (row["size"], row["steps"])
    if key not in baseline:
        raise SystemExit(
            f"Missing 1-thread baseline for size={row['size']}, steps={row['steps']}"
        )
    row["speedup"] = baseline[key] / row["mean_time"]

summary_path = Path("results_summary.csv")
with summary_path.open("w", newline="") as f:
    writer = csv.DictWriter(
        f,
        fieldnames=["size", "steps", "threads", "mean_time", "stdev_time", "speedup"],
    )
    writer.writeheader()
    writer.writerows(summary)

print()
print("Average results:")
print(f"{'Size':>6} {'Steps':>6} {'Threads':>7} {'Mean(s)':>12} {'Std(s)':>12} {'Speedup':>10}")
for row in summary:
    print(
        f"{row['size']:>6} "
        f"{row['steps']:>6} "
        f"{row['threads']:>7} "
        f"{row['mean_time']:>12.6f} "
        f"{row['stdev_time']:>12.6f} "
        f"{row['speedup']:>10.3f}"
    )

sizes = sorted({row["size"] for row in summary})
steps_values = sorted({row["steps"] for row in summary})

for size in sizes:
    plt.figure()
    all_threads = set()

    for steps in steps_values:
        rows = [r for r in summary if r["size"] == size and r["steps"] == steps]
        rows.sort(key=lambda r: r["threads"])
        if not rows:
            continue

        threads = [r["threads"] for r in rows]
        speedups = [r["speedup"] for r in rows]
        all_threads.update(threads)

        plt.plot(threads, speedups, marker="o", label=f"{steps} steps")

    threads_sorted = sorted(all_threads)
    if threads_sorted:
        plt.plot(threads_sorted, threads_sorted, linestyle="--", label="Ideal speedup")

    plt.xlabel("Number of OpenMP threads")
    plt.ylabel("Speedup")
    plt.title(f"Conway's Game of Life: {size} x {size}")
    plt.xticks(threads_sorted)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"speedup_{size}x{size}.png", dpi=200)
    plt.close()

print()
print("Generated:")
print("  results_summary.csv")
for size in sizes:
    print(f"  speedup_{size}x{size}.png")
