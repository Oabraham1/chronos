#!/usr/bin/env bash
# GPU performance benchmarking script

set -eo pipefail

OUTPUT_DIR="benchmarks/$(date +%Y%m%d_%H%M%S)"
METRICS=("utilization.gpu" "memory.used" "temperature.gpu")
declare -a PERCENTAGES=(10 20 30 40 50 60 70 80)

mkdir -p "$OUTPUT_DIR"

run_benchmark() {
    local perc=$1
    local logfile="$OUTPUT_DIR/${perc}perc.log"
    
    echo "Testing ${perc}% allocation"
    ./build/gpu_partitioner -p "$perc" -t 30 &
    local pid=$!
    
    nvidia-smi \
        --query-gpu=$(IFS=,; echo "${METRICS[*]}") \
        --format=csv -l 1 > "$logfile" &
    local monitor_pid=$!
    
    wait $pid
    kill $monitor_pid
    
    # Process results
    awk -F',' -v perc="$perc" '
    NR > 1 {
        sum_util += $1; 
        sum_mem += $2; 
        sum_temp += $3; 
        count++
    } 
    END {
        printf "%d,%.1f,%.1f,%.1f\n", 
        perc, sum_util/count, sum_mem/count, sum_temp/count
    }' "$logfile" >> "$OUTPUT_DIR/summary.csv"
}

echo "percentage,utilization(%),memory_usage(MB),temp(c)" > "$OUTPUT_DIR/summary.csv"

for perc in "${PERCENTAGES[@]}"; do
    run_benchmark "$perc"
done

echo "Benchmark results saved to $OUTPUT_DIR"