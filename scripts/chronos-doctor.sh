#!/bin/bash
# Chronos Doctor - Diagnose installation issues

echo "╔════════════════════════════════════════╗"
echo "║     Chronos Installation Doctor       ║"
echo "╚════════════════════════════════════════╝"
echo ""

ISSUES_FOUND=0

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass() {
    echo -e "${GREEN}✓${NC} $1"
}

fail() {
    echo -e "${RED}✗${NC} $1"
    ((ISSUES_FOUND++))
}

warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

info() {
    echo "  → $1"
}

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. System Information"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
info "OS: $(uname -s) $(uname -r)"
info "Architecture: $(uname -m)"
info "Shell: $SHELL"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. PATH Check"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
if echo "$PATH" | grep -q "/usr/local/bin"; then
    pass "/usr/local/bin is in PATH"
else
    fail "/usr/local/bin is NOT in PATH"
    info "Add to your shell config:"
    info "  echo 'export PATH=\"/usr/local/bin:\$PATH\"' >> ~/.zshrc"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. Chronos CLI"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check if binary exists
if [ -f "/usr/local/bin/chronos" ]; then
    pass "Binary found at /usr/local/bin/chronos"
elif [ -f "/usr/local/bin/chronos_cli" ]; then
    warn "Found chronos_cli but not chronos"
    info "Create symlink: sudo ln -s /usr/local/bin/chronos_cli /usr/local/bin/chronos"
else
    fail "Chronos binary not found"
    info "Run: ./scripts/install_quick.sh"
fi

# Check if in PATH
if command -v chronos &>/dev/null; then
    pass "chronos command is accessible"
    info "Location: $(which chronos)"
else
    fail "chronos command not found in PATH"
fi

# Check library dependencies (macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo ""
    if [ -f "/usr/local/bin/chronos" ] || [ -f "/usr/local/bin/chronos_cli" ]; then
        BINARY="/usr/local/bin/chronos"
        [ ! -f "$BINARY" ] && BINARY="/usr/local/bin/chronos_cli"

        if otool -L "$BINARY" 2>/dev/null | grep -q "libchronos.dylib"; then
            if [ -f "/usr/local/lib/libchronos.dylib" ]; then
                pass "libchronos.dylib found"
            else
                fail "libchronos.dylib not found at /usr/local/lib/"
            fi

            # Check library path
            if [ -n "$DYLD_LIBRARY_PATH" ] && echo "$DYLD_LIBRARY_PATH" | grep -q "/usr/local/lib"; then
                pass "DYLD_LIBRARY_PATH includes /usr/local/lib"
            else
                warn "DYLD_LIBRARY_PATH not set"
                info "Add to ~/.zshrc:"
                info "  export DYLD_LIBRARY_PATH=\"/usr/local/lib:\${DYLD_LIBRARY_PATH}\""
            fi
        fi
    fi
fi

# Test CLI
echo ""
if command -v chronos &>/dev/null; then
    if chronos stats &>/dev/null; then
        pass "chronos stats works"
    else
        fail "chronos stats failed"
        info "Try: export DYLD_LIBRARY_PATH=\"/usr/local/lib:\${DYLD_LIBRARY_PATH}\""
    fi
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. Python Environment"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check Python
if command -v python3 &>/dev/null; then
    PYTHON_CMD=$(command -v python3)
    PYTHON_VERSION=$($PYTHON_CMD --version 2>&1)
    pass "Python found: $PYTHON_VERSION"
    info "Executable: $PYTHON_CMD"
else
    fail "python3 not found"
fi

# Check for multiple Python versions
echo ""
info "Checking for multiple Python installations..."
for py in python3.9 python3.10 python3.11 python3.12 python3.13 python3.14; do
    if command -v $py &>/dev/null; then
        PY_PATH=$(which $py)
        PY_VER=$($py --version 2>&1)
        info "  Found: $PY_VER at $PY_PATH"
    fi
done

# Check pip
echo ""
if command -v pip3 &>/dev/null; then
    PIP_VERSION=$(pip3 --version)
    pass "pip3 found: $PIP_VERSION"
else
    warn "pip3 not found"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5. Python Bindings"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check if package is installed
if $PYTHON_CMD -m pip show chronos-gpu &>/dev/null; then
    pass "chronos-gpu package is installed"

    INSTALL_LOC=$($PYTHON_CMD -m pip show chronos-gpu | grep "Location:" | cut -d: -f2- | xargs)
    EDITABLE_LOC=$($PYTHON_CMD -m pip show chronos-gpu | grep "Editable project location:" | cut -d: -f2- | xargs)

    info "Install location: $INSTALL_LOC"
    if [ -n "$EDITABLE_LOC" ]; then
        info "Editable location: $EDITABLE_LOC"
    fi

    # Check if location is in Python path
    if $PYTHON_CMD -c "import sys; sys.exit(0 if '$INSTALL_LOC' in sys.path else 1)" 2>/dev/null; then
        pass "Package location is in Python path"
    else
        warn "Package location NOT in Python path"
    fi
else
    fail "chronos-gpu package not installed"
    info "Install with: python3 -m pip install -e /path/to/chronos --user"
fi

# Check if package installed to different Python
echo ""
info "Checking other Python versions for chronos-gpu..."
for py in python3.9 python3.10 python3.11 python3.12 python3.13 python3.14; do
    if command -v $py &>/dev/null && [ "$py" != "$(basename $PYTHON_CMD)" ]; then
        if $py -m pip show chronos-gpu &>/dev/null 2>&1; then
            warn "Also found in $py"
            info "  This might cause confusion!"
        fi
    fi
done

# Test import
echo ""
if $PYTHON_CMD -c "from chronos import Partitioner" 2>/dev/null; then
    pass "Can import chronos module"

    # Test creating partitioner
    if $PYTHON_CMD -c "from chronos import Partitioner; p = Partitioner()" 2>/dev/null; then
        pass "Can create Partitioner object"
    else
        fail "Failed to create Partitioner object"
        info "Library issue. Check DYLD_LIBRARY_PATH"
    fi
else
    fail "Cannot import chronos module"
    info "The package is installed but Python can't find it"
    info "This usually means:"
    info "  1. Package installed to different Python version"
    info "  2. Library path not set (DYLD_LIBRARY_PATH on macOS)"
    info ""
    info "Fix: Reinstall to correct Python:"
    info "  python3 -m pip uninstall -y chronos-gpu"
    info "  python3 -m pip install -e /path/to/chronos --user"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6. OpenCL Environment"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if command -v clinfo &>/dev/null; then
    pass "clinfo is installed"
    NUM_DEVICES=$(clinfo 2>/dev/null | grep -c "Device Name" || echo "0")
    if [ "$NUM_DEVICES" -gt 0 ]; then
        pass "Found $NUM_DEVICES OpenCL device(s)"
    else
        warn "No OpenCL devices found"
    fi
else
    warn "clinfo not installed (optional)"
    info "Install with: brew install clinfo  # macOS"
    info "           or: sudo apt install clinfo  # Linux"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7. Lock Directory"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -d "/tmp/chronos_locks" ]; then
    pass "Lock directory exists"
    PERMS=$(ls -ld /tmp/chronos_locks | cut -d' ' -f1)
    if [[ "$PERMS" == *"rwx"*"rwx"*"rwx"* ]]; then
        pass "Permissions are correct (777)"
    else
        warn "Permissions may be wrong: $PERMS"
        info "Fix: sudo chmod 777 /tmp/chronos_locks"
    fi
else
    fail "Lock directory missing"
    info "Create: sudo mkdir -p /tmp/chronos_locks && sudo chmod 777 /tmp/chronos_locks"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ $ISSUES_FOUND -eq 0 ]; then
    echo -e "${GREEN}✅ All checks passed!${NC}"
    echo ""
    echo "Chronos is properly installed and ready to use."
    echo ""
    echo "Try:"
    echo "  chronos stats"
    echo "  python3 -c 'from chronos import Partitioner; print(\"Success!\")'"
else
    echo -e "${YELLOW}⚠️  Found $ISSUES_FOUND issue(s)${NC}"
    echo ""
    echo "Review the failures above and follow the suggested fixes."
    echo ""
    echo "Common fixes:"
    echo "  1. Restart your terminal"
    echo "  2. Run: source ~/.zshrc  # or ~/.bashrc"
    echo "  3. Set library path: export DYLD_LIBRARY_PATH=\"/usr/local/lib:\${DYLD_LIBRARY_PATH}\""
    echo "  4. Reinstall Python bindings: python3 -m pip install -e . --user"
fi

echo ""
echo "For more help: https://github.com/oabraham1/chronos/blob/main/TROUBLESHOOTING.md"
echo ""
