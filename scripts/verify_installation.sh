#!/bin/bash
# verify_installation.sh - Verify Chronos installation is working

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASSED=0
FAILED=0

pass() {
    echo -e "${GREEN}✓ $1${NC}"
    ((PASSED++))
}

fail() {
    echo -e "${RED}✗ $1${NC}"
    ((FAILED++))
}

warn() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Chronos Installation Verification"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check 1: CLI Binary
echo "1. Checking CLI binary..."
if command -v chronos &> /dev/null; then
    VERSION=$(chronos --version 2>&1 || echo "unknown")
    pass "CLI binary found: $VERSION"
else
    fail "CLI binary not found in PATH"
    info "Try: export PATH=/usr/local/bin:\$PATH"
fi
echo ""

# Check 2: CLI Help
echo "2. Checking CLI help..."
if chronos help &> /dev/null; then
    pass "CLI help works"
else
    fail "CLI help command failed"
fi
echo ""

# Check 3: Python Import
echo "3. Checking Python bindings..."
if python3 -c "import chronos" 2>/dev/null; then
    PY_VERSION=$(python3 -c "import chronos; print(chronos.__version__ if hasattr(chronos, '__version__') else 'unknown')" 2>/dev/null || echo "unknown")
    pass "Python import works (version: $PY_VERSION)"
else
    fail "Python import failed"
    info "Try: pip install chronos-gpu"
    info "Or: export PYTHONPATH=/path/to/chronos/python:\$PYTHONPATH"
fi
echo ""

# Check 4: Shared Library
echo "4. Checking shared library..."
if [ -f "/usr/local/lib/libchronos.so" ] || [ -f "/usr/local/lib/libchronos.dylib" ]; then
    pass "Shared library found"
elif [ -f "build/lib/libchronos.so" ] || [ -f "build/lib/libchronos.dylib" ]; then
    warn "Shared library found in build directory (not installed)"
    info "Run: cd build && sudo make install"
else
    fail "Shared library not found"
fi
echo ""

# Check 5: Lock Directory
echo "5. Checking lock directory..."
if [ -d "/tmp/chronos_locks" ]; then
    if [ -w "/tmp/chronos_locks" ]; then
        pass "Lock directory exists and is writable"
    else
        warn "Lock directory exists but is not writable"
        info "Run: sudo chmod 777 /tmp/chronos_locks"
    fi
else
    fail "Lock directory does not exist"
    info "Run: sudo mkdir -p /tmp/chronos_locks && sudo chmod 777 /tmp/chronos_locks"
fi
echo ""

# Check 6: OpenCL
echo "6. Checking OpenCL..."
if python3 -c "from chronos import Partitioner; p = Partitioner(); p.show_stats()" 2>/dev/null; then
    pass "OpenCL devices detected"
elif chronos stats 2>&1 | grep -q "GPU\|Device"; then
    pass "OpenCL devices detected via CLI"
else
    warn "No OpenCL devices found (or drivers not installed)"
    info "Install GPU drivers and OpenCL runtime"
fi
echo ""

# Check 7: Basic Operations
echo "7. Testing basic operations..."
TEST_RESULT=0

# Try to create a small partition
if python3 << 'EOF' 2>/dev/null
from chronos import Partitioner, ChronosError
import sys
try:
    p = Partitioner()
    partition = p.create(device=0, memory=0.05, duration=3)
    partitions = p.list()
    if len(partitions) == 1:
        partition.release()
        print("OK")
        sys.exit(0)
    else:
        sys.exit(1)
except ChronosError as e:
    if "No OpenCL" in str(e):
        print("NO_GPU")
        sys.exit(0)
    sys.exit(1)
EOF
then
    RESULT=$(python3 << 'EOF' 2>/dev/null
from chronos import Partitioner, ChronosError
import sys
try:
    p = Partitioner()
    partition = p.create(device=0, memory=0.05, duration=3)
    partition.release()
    print("OK")
except ChronosError:
    print("NO_GPU")
EOF
)
    if [ "$RESULT" = "OK" ]; then
        pass "Create/release partition works"
    else
        warn "Basic operations work (no GPU for testing)"
    fi
else
    fail "Basic operations failed"
fi
echo ""

# Check 8: Context Manager
echo "8. Testing Python context manager..."
if python3 << 'EOF' 2>/dev/null
from chronos import Partitioner, ChronosError
import sys
try:
    p = Partitioner()
    with p.create(device=0, memory=0.05, duration=3) as partition:
        pass
    print("OK")
    sys.exit(0)
except ChronosError as e:
    if "No OpenCL" in str(e):
        print("NO_GPU")
        sys.exit(0)
    sys.exit(1)
EOF
then
    CTX_RESULT=$(python3 << 'EOF' 2>/dev/null
from chronos import Partitioner, ChronosError
try:
    p = Partitioner()
    with p.create(device=0, memory=0.05, duration=3) as partition:
        pass
    print("OK")
except ChronosError:
    print("NO_GPU")
EOF
)
    if [ "$CTX_RESULT" = "OK" ]; then
        pass "Context manager works"
    else
        warn "Context manager works (no GPU for testing)"
    fi
else
    fail "Context manager test failed"
fi
echo ""

# Check 9: Documentation
echo "9. Checking documentation..."
DOC_COUNT=0
[ -f "README.md" ] && ((DOC_COUNT++))
[ -f "INSTALL.md" ] && ((DOC_COUNT++))
[ -f "docs/USER_GUIDE.md" ] && ((DOC_COUNT++))
[ -f "docs/FAQ.md" ] && ((DOC_COUNT++))
[ -f "python/README.md" ] && ((DOC_COUNT++))

if [ $DOC_COUNT -ge 4 ]; then
    pass "Documentation files present ($DOC_COUNT/5)"
else
    warn "Some documentation missing ($DOC_COUNT/5)"
fi
echo ""

# Check 10: Examples
echo "10. Checking examples..."
EXAMPLE_COUNT=0
[ -f "examples/simple_partition.cpp" ] && ((EXAMPLE_COUNT++))
[ -f "examples/advanced_usage.cpp" ] && ((EXAMPLE_COUNT++))

if [ $EXAMPLE_COUNT -ge 1 ]; then
    pass "Example files present ($EXAMPLE_COUNT/2)"
else
    warn "Example files missing"
fi
echo ""

# Summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

TOTAL=$((PASSED + FAILED))
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All checks passed ($PASSED/$TOTAL)${NC}"
    echo ""
    echo "Installation verified! You can now use Chronos."
    echo ""
    echo "Quick start:"
    echo "  chronos stats"
    echo "  chronos create 0 0.5 3600"
    echo "  chronos list"
    echo ""
    echo "Python:"
    echo "  from chronos import Partitioner"
    echo "  with Partitioner().create(device=0, memory=0.5, duration=3600) as p:"
    echo "      # Your GPU code here"
    echo "      pass"
    echo ""
    EXIT_CODE=0
elif [ $FAILED -le 2 ]; then
    echo -e "${YELLOW}⚠️  Some checks failed ($PASSED/$TOTAL passed)${NC}"
    echo ""
    echo "Installation mostly works, but some components may need attention."
    echo "Review the failures above and follow the suggested fixes."
    echo ""
    EXIT_CODE=1
else
    echo -e "${RED}❌ Multiple checks failed ($PASSED/$TOTAL passed)${NC}"
    echo ""
    echo "Installation has issues. Please:"
    echo "  1. Review failures above"
    echo "  2. Check INSTALL.md for troubleshooting"
    echo "  3. Report issues at: https://github.com/oabraham1/chronos/issues"
    echo ""
    EXIT_CODE=2
fi

exit $EXIT_CODE
