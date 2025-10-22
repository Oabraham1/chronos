#!/bin/bash
# format.sh - Format all source files

set -e

echo "=== Chronos Code Formatter ==="
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Format C++ files
echo -e "${BLUE}Formatting C++ files...${NC}"
find src include apps tests examples benchmarks -type f \
  \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
  -exec clang-format -i {} \;
echo -e "${GREEN}✓ C++ files formatted${NC}"

# Format Python files
echo ""
echo -e "${BLUE}Formatting Python files...${NC}"

# Check if black is installed
if ! command -v black &> /dev/null; then
    echo "Installing black..."
    pip3 install black isort
fi

# Format with black
black python/ --line-length 100 --quiet
echo -e "${GREEN}✓ Python files formatted with black${NC}"

# Sort imports with isort
if command -v isort &> /dev/null; then
    isort python/ --profile black --quiet
    echo -e "${GREEN}✓ Python imports sorted with isort${NC}"
fi

echo ""
echo -e "${GREEN}=== All files formatted successfully! ===${NC}"
echo ""
echo "Modified files:"
git diff --name-only | grep -E '\.(cpp|h|hpp|py)$' || echo "  No changes"
