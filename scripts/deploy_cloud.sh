#!/usr/bin/env bash
# Deploy to cloud GPU providers (AWS/GCP/Azure)
# Usage: ./deploy_cloud.sh [aws|gcp|azure] [options]

set -eo pipefail

# Configuration
INSTANCE_NAME="gpu-node-$(date +%s)"
AWS_GPU_TYPE="g4dn.xlarge"
GCP_GPU_TYPE="nvidia-tesla-t4"
AZURE_GPU_SKU="Standard_NC6s_v3"
SSH_KEY="${HOME}/.ssh/cloud-key"
REGION="us-west1-b" # GCP specific
PORT=8888 # Application port

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

validate_environment() {
    case $1 in
        aws)
            command -v aws >/dev/null 2>&1 || { 
                echo -e "${RED}AWS CLI not found!${NC}"
                exit 1
            }
            ;;
        gcp)
            command -v gcloud >/dev/null 2>&1 || {
                echo -e "${RED}gcloud CLI not found!${NC}"
                exit 1
            }
            ;;
        azure)
            command -v az >/dev/null 2>&1 || {
                echo -e "${RED}Azure CLI not found!${NC}"
                exit 1
            }
            ;;
        *)
            echo -e "${RED}Invalid cloud provider${NC}"
            exit 1
            ;;
    esac
}

deploy_aws() {
    echo -e "${GREEN}Deploying to AWS...${NC}"
    
    # Create EC2 instance
    INSTANCE_ID=$(aws ec2 run-instances \
        --image-id ami-0abcdef1234567890 \
        --instance-type $AWS_GPU_TYPE \
        --key-name $INSTANCE_NAME \
        --security-group-ids sg-0123456789abcdef \
        --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=$INSTANCE_NAME}]" \
        --query 'Instances[0].InstanceId' \
        --output text)
    
    # Wait for running state
    aws ec2 wait instance-running --instance-ids $INSTANCE_ID
    
    # Get public IP
    PUBLIC_IP=$(aws ec2 describe-instances \
        --instance-ids $INSTANCE_ID \
        --query 'Reservations[0].Instances[0].PublicIpAddress' \
        --output text)
    
    echo -e "${YELLOW}Instance IP: $PUBLIC_IP${NC}"
}

deploy_gcp() {
    echo -e "${GREEN}Deploying to GCP...${NC}"
    
    # Create instance
    gcloud compute instances create $INSTANCE_NAME \
        --machine-type n1-standard-4 \
        --zone $REGION \
        --accelerator type=$GCP_GPU_TYPE,count=1 \
        --image-family ubuntu-2204-lts \
        --image-project ubuntu-os-cloud \
        --tags http-server,https-server
    
    # Open application port
    gcloud compute firewall-rules create allow-gpu-app \
        --allow tcp:$PORT \
        --target-tags http-server
    
    # Get external IP
    PUBLIC_IP=$(gcloud compute instances describe $INSTANCE_NAME \
        --zone $REGION \
        --format='get(networkInterfaces[0].accessConfigs[0].natIP)')
    
    echo -e "${YELLOW}Instance IP: $PUBLIC_IP${NC}"
}

deploy_azure() {
    echo -e "${GREEN}Deploying to Azure...${NC}"
    
    # Create resource group
    az group create --name $INSTANCE_NAME --location eastus
    
    # Create VM
    az vm create \
        --resource-group $INSTANCE_NAME \
        --name $INSTANCE_NAME \
        --size $AZURE_GPU_SKU \
        --image UbuntuLTS \
        --admin-username azureuser \
        --generate-ssh-keys
    
    # Open application port
    az vm open-port \
        --resource-group $INSTANCE_NAME \
        --name $INSTANCE_NAME \
        --port $PORT
    
    # Get public IP
    PUBLIC_IP=$(az vm show \
        --resource-group $INSTANCE_NAME \
        --name $INSTANCE_NAME \
        --show-details \
        --query 'publicIps' \
        --output tsv)
    
    echo -e "${YELLOW}Instance IP: $PUBLIC_IP${NC}"
}

setup_instance() {
    echo -e "${GREEN}Setting up instance...${NC}"
    
    # Generate SSH key if not exists
    if [ ! -f $SSH_KEY ]; then
        ssh-keygen -t rsa -b 4096 -f $SSH_KEY -N ""
    fi

    # Wait for SSH access
    until ssh -i $SSH_KEY -o StrictHostKeyChecking=no $USER@$PUBLIC_IP true 2>/dev/null; do
        echo -e "${YELLOW}Waiting for SSH access...${NC}"
        sleep 10
    done

    # Sync and build code
    rsync -avz -e "ssh -i $SSH_KEY" \
        --exclude='build/' \
        --exclude='.git/' \
        ./ $USER@$PUBLIC_IP:/app
    
    ssh -i $SSH_KEY $USER@$PUBLIC_IP "
        cd /app
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j\$(nproc)
    "
}

main() {
    if [ $# -eq 0 ]; then
        echo -e "${RED}Usage: $0 [aws|gcp|azure]${NC}"
        exit 1
    fi

    PROVIDER=$1
    validate_environment $PROVIDER

    case $PROVIDER in
        aws) deploy_aws ;;
        gcp) deploy_gcp ;;
        azure) deploy_azure ;;
    esac

    setup_instance
    
    echo -e "${GREEN}Deployment complete! Connect using:"
    echo -e "ssh -i $SSH_KEY $USER@$PUBLIC_IP${NC}"
    echo -e "Application accessible at: http://$PUBLIC_IP:$PORT"
}

main "$@"