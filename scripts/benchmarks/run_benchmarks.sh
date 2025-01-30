#!/usr/bin/env bash
# Runs benchmarks with different allocation percentages

set -eo pipefail

OUTPUT_DIR="benchmarks/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

declare -a PERCENTAGES=(10 20 30 40 50)
DURATION=60  # Seconds

run_benchmark() {
    local perc=$1
    local logfile="$OUTPUT_DIR/gpu_${perc}perc.log"

    echo "Testing ${perc}% allocation for ${DURATION}s"
    ./build/gpu_partitioner -p "$perc" -t "$DURATION" &
    PID=$!

    # Monitor GPU metrics
    nvidia-smi \
        --query-gpu=utilization.gpu,memory.used,temperature.gpu \
        --format=csv -l 1 > "$logfile" &
    MONITOR_PID=$!

    wait $PID
    kill $MONITOR_PID

    # Process results
    awk -F',' -v perc="$perc" '
    NR > 1 { sum_util += $1; sum_mem += $2; sum_temp += $3; count++ }
    END {
        printf "Alloc: %2d%% | Avg Util: %3.1f%% | Mem: %5.1f MB | Temp: %2.1f°C\n",
        perc, sum_util/count, sum_mem/count, sum_temp/count
    }' "$logfile"
}

for perc in "${PERCENTAGES[@]}"; do
    run_benchmark "$perc"
done
