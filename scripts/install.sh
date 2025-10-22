#!/bin/bash
#
# Chronos GPU Partitioner installer
# This script installs Chronos to /usr/local/bin

set -e

if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   echo "Please use: curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh | sudo bash" 1>&2
   exit 1
fi

echo "Chronos GPU Partitioner Installer"
echo "================================="

if [ "$(uname)" == "Darwin" ]; then
    OS="macos"
    echo "Detected macOS system"
elif [ "$(uname)" == "Linux" ]; then
    OS="linux"
    echo "Detected Linux system"
else
    echo "Unsupported operating system. Chronos supports macOS and Linux."
    exit 1
fi

TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

echo "Fetching latest release information..."
LATEST_RELEASE_URL=$(curl -s https://api.github.com/repos/oabraham1/chronos/releases/latest | grep "browser_download_url.*chronos-$OS" | cut -d : -f 2,3 | tr -d \")

if [ -z "$LATEST_RELEASE_URL" ]; then
    echo "Error: Could not find the latest release URL. Please check your internet connection."
    exit 1
fi

echo "Downloading Chronos..."
curl -L -o "chronos-$OS" "$LATEST_RELEASE_URL"

if [ ! -f "chronos-$OS" ]; then
    echo "Error: Download failed. Please try again later."
    exit 1
fi

echo "Installing Chronos..."
chmod +x "chronos-$OS"
mv "chronos-$OS" /usr/local/bin/chronos

echo "Creating lock directory..."
mkdir -p /tmp/chronos_locks
chmod 777 /tmp/chronos_locks

cd - > /dev/null
rm -rf "$TMP_DIR"

# Verify installation
if command -v chronos >/dev/null 2>&1; then
    echo ""
    echo "Chronos has been successfully installed!"
    echo "Run 'chronos help' to get started."
    chronos version
else
    echo "Installation failed. Please try installing from source."
    exit 1
fi
