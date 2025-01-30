#!/usr/bin/env bash
# Terminate cloud instances

set -eo pipefail

CLOUD_PROVIDER=${1:-aws}
INSTANCE_ID=${2:-}

case $CLOUD_PROVIDER in
    aws)
        aws ec2 terminate-instances --instance-ids "$INSTANCE_ID"
        ;;
    gcp)
        gcloud compute instances delete "$INSTANCE_ID" --zone=us-west1-b
        ;;
    *)
        echo "Invalid cloud provider"
        exit 1
        ;;
esac

echo "Terminated instance $INSTANCE_ID on $CLOUD_PROVIDER"