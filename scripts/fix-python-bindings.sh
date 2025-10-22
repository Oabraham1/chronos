#!/bin/bash
# Fix Python bindings installation issues

set -e

echo "======================================"
echo "  Chronos Python Bindings Fixer      "
echo "======================================"
echo ""

# Detect script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHRONOS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Chronos directory: $CHRONOS_DIR"
echo ""

# Clean up old artifacts
echo "Step 1: Cleaning up old build artifacts..."
cd "$CHRONOS_DIR"

# Try to remove, using sudo if needed
rm -rf python/chronos_gpu.egg-info 2>/dev/null || {
    echo "Need sudo to clean up old artifacts..."
    sudo rm -rf python/chronos_gpu.egg-info
}

rm -rf *.egg-info 2>/dev/null
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true

echo "✓ Cleanup complete"
echo ""

# Detect Python
PYTHON_CMD=$(command -v python3)
PYTHON_VERSION=$($PYTHON_CMD --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
echo "Detected Python $PYTHON_VERSION at $PYTHON_CMD"
echo ""

# Check for Homebrew Python (PEP 668)
IS_HOMEBREW_PYTHON=false
if [[ "$OSTYPE" == "darwin"* ]] && [[ "$PYTHON_CMD" == *"homebrew"* ]]; then
    IS_HOMEBREW_PYTHON=true
    echo "⚠️  Homebrew-managed Python detected (PEP 668 restrictions apply)"
    echo ""
fi

# Present options
echo "Choose installation method:"
echo ""
echo "  1. Virtual environment (recommended - isolated from system)"
echo "  2. User site-packages (simple - uses --user flag)"
if $IS_HOMEBREW_PYTHON; then
    echo "  3. Break system packages (not recommended - uses --break-system-packages)"
fi
echo "  q. Quit without installing"
echo ""
read -p "Enter choice [1-2${IS_HOMEBREW_PYTHON:+/3}/q]: " choice

case $choice in
    1)
        echo ""
        echo "Creating virtual environment..."
        VENV_DIR="$HOME/.chronos-venv"

        # Remove old venv if exists
        if [ -d "$VENV_DIR" ]; then
            read -p "Remove existing venv at $VENV_DIR? [y/N]: " remove_old
            if [[ $remove_old =~ ^[Yy]$ ]]; then
                rm -rf "$VENV_DIR"
            fi
        fi

        $PYTHON_CMD -m venv "$VENV_DIR"
        source "$VENV_DIR/bin/activate"

        echo "Upgrading pip..."
        pip install --upgrade pip setuptools wheel --quiet

        echo "Installing Chronos..."
        pip install -e "$CHRONOS_DIR"

        echo ""
        echo "✓ Installation complete!"
        echo ""
        echo "To use Chronos Python bindings:"
        echo "  source $VENV_DIR/bin/activate"
        echo "  python3 your_script.py"
        echo ""
        echo "Test now:"
        python3 -c "from chronos import Partitioner; print('✓ Success! Python bindings working.')" || {
            echo "⚠️  Import test failed"
            echo "Try setting library path:"
            if [[ "$OSTYPE" == "darwin"* ]]; then
                echo "  export DYLD_LIBRARY_PATH=\"/usr/local/lib:\${DYLD_LIBRARY_PATH}\""
            else
                echo "  export LD_LIBRARY_PATH=\"/usr/local/lib:\${LD_LIBRARY_PATH}\""
            fi
        }

        deactivate
        ;;

    2)
        echo ""
        echo "Installing to user site-packages..."
        $PYTHON_CMD -m pip install -e "$CHRONOS_DIR" --user

        echo ""
        echo "✓ Installation complete!"
        echo ""
        echo "Test the installation:"

        # Set library path for testing
        if [[ "$OSTYPE" == "darwin"* ]]; then
            export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"
        else
            export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"
        fi

        python3 -c "from chronos import Partitioner; print('✓ Success! Python bindings working.')" || {
            echo ""
            echo "⚠️  Import test failed"
            echo ""
            echo "You may need to set your library path:"
            if [[ "$OSTYPE" == "darwin"* ]]; then
                echo "  export DYLD_LIBRARY_PATH=\"/usr/local/lib:\${DYLD_LIBRARY_PATH}\""
                echo "  echo 'export DYLD_LIBRARY_PATH=\"/usr/local/lib:\${DYLD_LIBRARY_PATH}\"' >> ~/.zshrc"
            else
                echo "  export LD_LIBRARY_PATH=\"/usr/local/lib:\${LD_LIBRARY_PATH}\""
                echo "  echo 'export LD_LIBRARY_PATH=\"/usr/local/lib:\${LD_LIBRARY_PATH}\"' >> ~/.bashrc"
            fi
        }
        ;;

    3)
        if ! $IS_HOMEBREW_PYTHON; then
            echo "Invalid choice"
            exit 1
        fi

        echo ""
        echo "Installing with --break-system-packages..."
        echo "⚠️  Warning: This may interfere with system Python packages"
        read -p "Continue? [y/N]: " confirm

        if [[ ! $confirm =~ ^[Yy]$ ]]; then
            echo "Cancelled"
            exit 0
        fi

        $PYTHON_CMD -m pip install -e "$CHRONOS_DIR" --break-system-packages

        echo ""
        echo "✓ Installation complete!"
        echo ""
        echo "Test the installation:"
        python3 -c "from chronos import Partitioner; print('✓ Success! Python bindings working.')"
        ;;

    q|Q)
        echo "Installation cancelled"
        exit 0
        ;;

    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "For more help, see:"
echo "  - Troubleshooting: $CHRONOS_DIR/TROUBLESHOOTING.md"
echo "  - Run diagnostics: $CHRONOS_DIR/scripts/chronos-doctor.sh"
echo ""
