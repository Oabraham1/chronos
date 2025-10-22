#!/bin/bash
#
# Chronos GPU Partitioner user installer
# This script installs Chronos to ~/.local/bin without requiring root privileges

set -e

echo "Chronos GPU Partitioner User Installer"
echo "====================================="

# Detect OS
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

# Create installation directory if it doesn't exist
INSTALL_DIR="$HOME/.local/bin"
mkdir -p "$INSTALL_DIR"

# Add directory to PATH if needed
if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
    echo "Adding $INSTALL_DIR to your PATH in ~/.bashrc or ~/.zshrc"

    if [ -f "$HOME/.zshrc" ]; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.zshrc"
        echo "Please restart your terminal or run 'source ~/.zshrc' to update your PATH"
    elif [ -f "$HOME/.bashrc" ]; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        echo "Please restart your terminal or run 'source ~/.bashrc' to update your PATH"
    else
        echo "Please add the following line to your shell configuration file:"
        echo 'export PATH="$HOME/.local/bin:$PATH"'
    fi
fi

# Create temporary directory
TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

# Get the latest release version
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
mv "chronos-$OS" "$INSTALL_DIR/chronos"

# Create lock directory
echo "Creating lock directory..."
mkdir -p /tmp/chronos_locks
chmod 777 /tmp/chronos_locks 2>/dev/null || echo "Warning: Could not set permissions on lock directory. You may need to run: sudo chmod 777 /tmp/chronos_locks"

# Clean up
cd - > /dev/null
rm -rf "$TMP_DIR"

# Verify installation
if "$INSTALL_DIR/chronos" version 2>/dev/null; then
    echo ""
    echo "Chronos has been successfully installed!"
    echo "Run 'chronos help' to get started."
    "$INSTALL_DIR/chronos" version
else
    echo ""
    echo "Installation complete, but chronos may not be in your PATH yet."
    echo "Try running: $INSTALL_DIR/chronos help"
    echo "Or restart your terminal and run: chronos help"
fi
