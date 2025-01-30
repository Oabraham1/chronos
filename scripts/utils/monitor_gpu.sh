#!/usr/bin/env bash
# Real-time GPU monitoring

set -eo pipefail

INTERVAL=${1:-1} # Update interval in seconds

header_printed=false

while true; do
    stats=$(nvidia-smi \
        --query-gpu=utilization.gpu,memory.used,temperature.gpu \
        --format=csv,noheader,nounits)
    
    if ! $header_printed; then
        echo "GPU_Util(%)  Mem_Used(MB)  Temp(°C)"
        header_printed=true
    fi
    
    awk -F', ' '{printf "%-12s %-13s %-9s\n", $1, $2, $3}' <<< "$stats"
    
    sleep $INTERVAL
done