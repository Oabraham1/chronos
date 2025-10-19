#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
BUILD_DIR="build"

echo "======================================"
echo "  Chronos GPU Partitioner Installer  "
echo "======================================"
echo ""

if [ "$EUID" -ne 0 ]; then
   echo "Note: This script will request sudo for installation"
   echo ""
fi

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            echo "$ID"
        else
            echo "linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

install_dependencies() {
    local os=$(detect_os)

    echo "Installing dependencies for $os..."

    case $os in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y build-essential cmake ocl-icd-opencl-dev opencl-headers python3 python3-pip python3-venv
            ;;
        fedora|rhel|centos)
            sudo dnf install -y gcc gcc-c++ cmake ocl-icd-devel opencl-headers python3 python3-pip
            ;;
        macos)
            if ! command -v brew &> /dev/null; then
                echo "Error: Homebrew not found. Please install from https://brew.sh"
                exit 1
            fi
            brew install cmake python3
            ;;
        *)
            echo "Warning: Unknown OS. Please install dependencies manually:"
            echo "  - C++ compiler (gcc/clang)"
            echo "  - CMake 3.10+"
            echo "  - OpenCL headers and libraries"
            echo "  - Python 3.7+"
            exit 1
            ;;
    esac
}

build_chronos() {
    echo ""
    echo "Building Chronos..."

    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..

    local num_cores
    if [[ "$OSTYPE" == "darwin"* ]]; then
        num_cores=$(sysctl -n hw.ncpu)
    else
        num_cores=$(nproc)
    fi

    make -j"$num_cores"

    echo ""
    echo "Running tests..."
    ctest --output-on-failure || echo "Warning: Some tests failed (this may be OK if no GPU)"

    cd ..
}

cleanup_build_artifacts() {
    echo "Cleaning up old build artifacts..."

    # Clean Python build artifacts
    rm -rf python/chronos_gpu.egg-info 2>/dev/null || sudo rm -rf python/chronos_gpu.egg-info 2>/dev/null
    rm -rf *.egg-info 2>/dev/null
    rm -rf python/__pycache__ 2>/dev/null
    rm -rf python/**/__pycache__ 2>/dev/null

    # Clean build directories
    find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null
    find . -type d -name "*.egg-info" -exec rm -rf {} + 2>/dev/null

    echo "✓ Build artifacts cleaned"
}

install_chronos() {
    echo ""
    echo "Installing Chronos..."

    cd "$BUILD_DIR"
    sudo make install
    cd ..

    # Clean up before Python installation
    cleanup_build_artifacts

    # Create chronos symlink if it doesn't exist
    if [ -f "/usr/local/bin/chronos_cli" ] && [ ! -f "/usr/local/bin/chronos" ]; then
        echo "Creating chronos symlink..."
        sudo ln -s /usr/local/bin/chronos_cli /usr/local/bin/chronos
    fi

    sudo mkdir -p /tmp/chronos_locks
    sudo chmod 777 /tmp/chronos_locks

    echo ""
    echo "Installing Python bindings..."

    # Detect Python executable and version
    PYTHON_CMD=$(command -v python3)
    PYTHON_VERSION=$($PYTHON_CMD --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    echo "Using Python $PYTHON_VERSION at $PYTHON_CMD"

    # Check if we're in a Homebrew-managed Python environment
    IS_HOMEBREW_PYTHON=false
    if [[ "$OSTYPE" == "darwin"* ]] && [[ "$PYTHON_CMD" == *"homebrew"* ]]; then
        IS_HOMEBREW_PYTHON=true
    fi

    # Clean up any old egg-info directories that might cause permission issues
    echo "Cleaning up old build artifacts..."
    rm -rf python/chronos_gpu.egg-info 2>/dev/null || sudo rm -rf python/chronos_gpu.egg-info 2>/dev/null
    rm -rf *.egg-info 2>/dev/null

    # Try to install Python bindings with various strategies
    if $IS_HOMEBREW_PYTHON; then
        echo "Detected Homebrew-managed Python (PEP 668 protected)"
        echo ""
        echo "Choose installation method:"
        echo "  1. Virtual environment (recommended)"
        echo "  2. User site-packages (--user flag)"
        echo "  3. Break system packages (--break-system-packages)"
        echo "  4. Skip Python bindings installation"
        echo ""
        read -p "Enter choice [1-4]: " choice

        case $choice in
            1)
                echo "Creating virtual environment..."
                VENV_DIR="$HOME/.chronos-venv"

                # Remove old venv if it exists
                rm -rf "$VENV_DIR" 2>/dev/null

                $PYTHON_CMD -m venv "$VENV_DIR"
                source "$VENV_DIR/bin/activate"

                # Upgrade pip first
                pip install --upgrade pip setuptools wheel

                # Install Chronos
                pip install -e . && {
                    echo ""
                    echo "✓ Python bindings installed in virtual environment"
                    echo ""
                    echo "To use Python bindings, activate the environment:"
                    echo "  source $VENV_DIR/bin/activate"
                } || {
                    echo "⚠  Virtual environment installation failed"
                    echo "Falling back to user installation..."
                    deactivate
                    $PYTHON_CMD -m pip install -e . --user
                }
                deactivate 2>/dev/null
                ;;
            2)
                echo "Installing to user site-packages..."
                $PYTHON_CMD -m pip install -e . --user || {
                    echo "⚠  User installation failed. Trying pip3..."
                    pip3 install -e . --user 2>/dev/null || echo "⚠  Could not install Python bindings"
                }
                ;;
            3)
                echo "Installing with --break-system-packages..."
                $PYTHON_CMD -m pip install -e . --break-system-packages
                ;;
            4)
                echo "Skipping Python bindings installation"
                echo ""
                echo "You can install manually later with:"
                echo "  ./scripts/fix-python-bindings.sh"
                echo ""
                echo "Or manually:"
                echo "  # Clean old artifacts"
                echo "  rm -rf python/chronos_gpu.egg-info"
                echo "  # Install in venv"
                echo "  python3 -m venv ~/.chronos-venv"
                echo "  source ~/.chronos-venv/bin/activate"
                echo "  pip install -e /path/to/chronos"
                ;;
            *)
                echo "Invalid choice. Skipping Python bindings installation"
                ;;
        esac
    else
        # Not Homebrew Python, try normal installation methods
        if $PYTHON_CMD -m pip install -e . --user 2>/dev/null; then
            echo "✓ Python bindings installed to user site-packages"
        elif $PYTHON_CMD -m pip install -e . 2>/dev/null; then
            echo "✓ Python bindings installed to system site-packages"
        elif command -v pip3 &>/dev/null && pip3 install -e . --user 2>/dev/null; then
            echo "✓ Python bindings installed via pip3 (user)"
            echo "⚠  Note: Installed with pip3 which may use a different Python version"
        else
            echo "⚠  Python binding installation encountered issues"
            echo "   You may need to install manually:"
            echo "   $PYTHON_CMD -m pip install -e . --user"
        fi
    fi

    # Add library path to shell config for macOS
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # Detect shell
        if [ -n "$ZSH_VERSION" ] || [[ "$SHELL" == *"zsh"* ]]; then
            SHELL_RC="$HOME/.zshrc"
        elif [ -n "$BASH_VERSION" ] || [[ "$SHELL" == *"bash"* ]]; then
            SHELL_RC="$HOME/.bashrc"
        else
            SHELL_RC="$HOME/.profile"
        fi

        if ! grep -q "DYLD_LIBRARY_PATH.*usr/local/lib" "$SHELL_RC" 2>/dev/null; then
            echo 'export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"' >> "$SHELL_RC"
            echo "✓ Added library path to $SHELL_RC"
        fi

        # Set for current session
        export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"
    fi
}

verify_installation() {
    echo ""
    echo "Verifying installation..."

    # Create symlink from chronos_cli to chronos if needed
    if [ -f "/usr/local/bin/chronos_cli" ] && [ ! -f "/usr/local/bin/chronos" ]; then
        echo "Creating chronos symlink..."
        sudo ln -s /usr/local/bin/chronos_cli /usr/local/bin/chronos
    fi

    if command -v chronos &> /dev/null; then
        echo "✓ CLI installed successfully"
        # Test with stats instead of help to avoid potential crash
        if chronos stats > /dev/null 2>&1; then
            echo "✓ CLI working"
        else
            echo "⚠ CLI installed but may have issues - try running: chronos stats"
        fi
    elif [ -f "/usr/local/bin/chronos" ]; then
        echo "✓ CLI installed to /usr/local/bin/chronos"
        echo "⚠ /usr/local/bin not in PATH - adding it now..."

        # Detect shell and add to appropriate RC file
        if [ -n "$ZSH_VERSION" ]; then
            SHELL_RC="$HOME/.zshrc"
        elif [ -n "$BASH_VERSION" ]; then
            SHELL_RC="$HOME/.bashrc"
        else
            SHELL_RC="$HOME/.profile"
        fi

        if ! grep -q "/usr/local/bin" "$SHELL_RC" 2>/dev/null; then
            echo 'export PATH="/usr/local/bin:$PATH"' >> "$SHELL_RC"
            echo "✓ Added /usr/local/bin to PATH in $SHELL_RC"
        fi

        export PATH="/usr/local/bin:$PATH"

        if command -v chronos &> /dev/null; then
            echo "✓ CLI now accessible"
        else
            echo "⚠ Please restart your terminal or run: source $SHELL_RC"
        fi
    else
        echo "✗ CLI not found in /usr/local/bin"
        return 1
    fi

    # Check Python bindings
    echo ""
    if python3 -c "from chronos import Partitioner" 2>/dev/null; then
        echo "✓ Python bindings installed successfully"
    else
        # Check if it's in a venv
        if [ -d "$HOME/.chronos-venv" ]; then
            echo "Python bindings installed in virtual environment"
            echo "To test, run:"
            echo "  source ~/.chronos-venv/bin/activate"
            echo "  python3 -c \"from chronos import Partitioner; print('Success!')\""
        else
            echo "✗ Python bindings not available in current environment"
            echo ""
            echo "Quick fix options:"
            echo ""
            echo "  Option 1 - Use virtual environment:"
            echo "    python3 -m venv ~/.chronos-venv"
            echo "    source ~/.chronos-venv/bin/activate"
            echo "    pip install -e ~/oss/chronos"
            echo ""
            echo "  Option 2 - Install to user:"
            echo "    python3 -m pip install -e ~/oss/chronos --user"
            echo ""
            echo "  Option 3 - Just use the CLI (Python optional):"
            echo "    chronos stats"
        fi
    fi

    if [ -d "/tmp/chronos_locks" ]; then
        echo "✓ Lock directory created"
    fi
}

print_usage() {
    echo ""
    echo "======================================"
    echo "  Installation Complete!              "
    echo "======================================"
    echo ""

    if ! command -v chronos &> /dev/null; then
        echo "⚠️  IMPORTANT: chronos is not yet in your PATH"
        echo ""
        echo "To use chronos, either:"
        echo ""
        echo "  Option 1: Restart your terminal (recommended)"
        echo ""
        echo "  Option 2: Run this command now:"

        if [ -n "$ZSH_VERSION" ]; then
            echo "    source ~/.zshrc"
        elif [ -n "$BASH_VERSION" ]; then
            echo "    source ~/.bashrc"
        else
            echo "    source ~/.profile"
        fi

        echo ""
        echo "  Option 3: Use the full path:"
        echo "    /usr/local/bin/chronos stats"
        echo ""
    fi

    echo "Quick Start:"
    echo ""
    echo "  # Check available GPUs"
    echo "  chronos stats"
    echo ""
    echo "  # Create a partition (50% of GPU 0 for 1 hour)"
    echo "  chronos create 0 0.5 3600"
    echo ""
    echo "  # List active partitions"
    echo "  chronos list"
    echo ""

    if python3 -c "from chronos import Partitioner" 2>/dev/null; then
        echo "  # Python usage"
        echo "  python3 << EOF"
        echo "  from chronos import Partitioner"
        echo "  with Partitioner().create(device=0, memory=0.5, duration=3600) as p:"
        echo "      # Your GPU code here"
        echo "      pass"
        echo "  EOF"
    else
        echo "  # Python usage (after setting up virtual environment)"
        echo "  source ~/.chronos-venv/bin/activate"
        echo "  python3 << EOF"
        echo "  from chronos import Partitioner"
        echo "  with Partitioner().create(device=0, memory=0.5, duration=3600) as p:"
        echo "      # Your GPU code here"
        echo "      pass"
        echo "  EOF"
    fi

    echo ""
    echo "Troubleshooting:"
    echo "  Run the doctor script to diagnose issues:"
    echo "    ./scripts/chronos-doctor.sh"
    echo ""
    echo "  Fix Python bindings (if needed):"
    echo "    ./scripts/fix-python-bindings.sh"
    echo ""
    echo "Documentation: https://github.com/oabraham1/chronos"
    echo "Issues: https://github.com/oabraham1/chronos/issues"
    echo ""
}

main() {
    echo "Step 1/4: Detecting system and installing dependencies..."
    install_dependencies

    echo ""
    echo "Step 2/4: Building Chronos..."
    build_chronos

    echo ""
    echo "Step 3/4: Installing Chronos..."
    install_chronos

    echo ""
    echo "Step 4/4: Verifying installation..."
    verify_installation

    print_usage
}

if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "Chronos Quick Installer"
    echo ""
    echo "Usage: $0"
    echo ""
    echo "This script will:"
    echo "  1. Install system dependencies"
    echo "  2. Build Chronos from source"
    echo "  3. Install Chronos system-wide"
    echo "  4. Install Python bindings (with PEP 668 handling)"
    echo ""
    echo "Requirements:"
    echo "  - Ubuntu/Debian, Fedora/RHEL, or macOS"
    echo "  - Sudo access"
    echo "  - Internet connection"
    echo ""
    exit 0
fi

main
