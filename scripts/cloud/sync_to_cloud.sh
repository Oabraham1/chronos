#!/usr/bin/env bash
# Sync code to cloud GPU instance

set -eo pipefail

HOST="${1:-user@cloud-instance}"
KEY_FILE="${2:-~/.ssh/cloud-key}"
PROJECT_DIR="/project"

rsync -avz -e "ssh -i $KEY_FILE" \
    --exclude='build/' \
    --exclude='.git/' \
    --exclude='data/' \
    ./ "$HOST:$PROJECT_DIR/"

ssh -i "$KEY_FILE" "$HOST" "
    cd $PROJECT_DIR && 
    mkdir -p build && 
    cd build && 
    cmake -DCMAKE_BUILD_TYPE=Release .. && 
    make -j\$(nproc)
"

echo "Code synced and built successfully on $HOST"