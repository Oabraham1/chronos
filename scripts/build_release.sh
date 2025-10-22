#!/bin/bash
# build_releases.sh - Build binaries for all platforms

set -e

VERSION="1.0.1"
PROJECT="chronos"
BUILD_DIR="build-release"
DIST_DIR="dist"

echo "🚀 Building Chronos v${VERSION} Release Binaries"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create dist directory
mkdir -p ${DIST_DIR}

# Determine platform
case "$(uname -s)" in
    Linux*)     PLATFORM=Linux;;
    Darwin*)    PLATFORM=macOS;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=Windows;;
    *)          PLATFORM=Unknown;;
esac

echo -e "${GREEN}Building on: ${PLATFORM}${NC}"
echo ""

# Function to build for a specific configuration
build() {
    local platform=$1
    local arch=$2
    local cmake_flags=$3

    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Building ${platform}-${arch}...${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    local build_name="${BUILD_DIR}-${platform}-${arch}"
    local archive_name="${PROJECT}-${VERSION}-${platform}-${arch}"

    # Clean previous build
    rm -rf ${build_name}

    # Configure
    echo "Configuring..."
    cmake -B ${build_name} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=install \
        -DBUILD_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        ${cmake_flags}

    # Build
    echo "Building..."
    if [[ "$PLATFORM" == "Darwin" ]]; then
        cmake --build ${build_name} --config Release -j$(sysctl -n hw.ncpu)
    elif [[ "$PLATFORM" == "Linux" ]]; then
        cmake --build ${build_name} --config Release -j$(nproc)
    else
        cmake --build ${build_name} --config Release -j4
    fi

    # Install to temporary directory
    echo "Installing..."
    cmake --install ${build_name} --prefix ${build_name}/install

    # Create archive
    echo "Creating archive..."
    cd ${build_name}/install

    if [ "${platform}" = "Windows" ] || [ "${platform}" = "windows" ]; then
        zip -r "../../${DIST_DIR}/${archive_name}.zip" * > /dev/null 2>&1
        echo -e "${GREEN}✓ Created ${DIST_DIR}/${archive_name}.zip${NC}"
    else
        tar czf "../../${DIST_DIR}/${archive_name}.tar.gz" *
        echo -e "${GREEN}✓ Created ${DIST_DIR}/${archive_name}.tar.gz${NC}"
    fi

    cd ../..
    echo ""
}

# Build for current platform
if [ "${PLATFORM}" = "Linux" ]; then
    echo -e "${BLUE}Building Linux binaries...${NC}"
    echo ""

    # Linux x86_64
    build "linux" "x86_64" ""

    # Linux ARM64 (if cross-compile tools available)
    if command -v aarch64-linux-gnu-gcc &> /dev/null; then
        echo -e "${BLUE}Cross-compiling for ARM64...${NC}"
        build "linux" "arm64" "-DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
    fi

elif [ "${PLATFORM}" = "macOS" ]; then
    echo -e "${BLUE}Building macOS binaries...${NC}"
    echo ""

    # macOS Universal (Intel + Apple Silicon)
    build "macos" "universal" "-DCMAKE_OSX_ARCHITECTURES='x86_64;arm64'"

elif [ "${PLATFORM}" = "Windows" ]; then
    echo -e "${BLUE}Building Windows binaries...${NC}"
    echo ""

    # Windows x86_64
    build "windows" "x86_64" "-G 'Visual Studio 17 2022' -A x64"
fi

# Build Python wheel (platform-specific)
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Building Python wheel...${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

python3 -m pip install --upgrade build wheel > /dev/null 2>&1
python3 -m build

# Copy wheels to dist
if [ -d "dist" ]; then
    find dist -name "*.whl" -exec cp {} ${DIST_DIR}/ \;
    find dist -name "*.tar.gz" -exec cp {} ${DIST_DIR}/ \;
fi

echo -e "${GREEN}✓ Python packages created${NC}"
echo ""

# Create checksums
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Generating checksums...${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

cd ${DIST_DIR}

if command -v sha256sum &> /dev/null; then
    sha256sum * > SHA256SUMS 2>/dev/null || true
elif command -v shasum &> /dev/null; then
    shasum -a 256 * > SHA256SUMS 2>/dev/null || true
fi

echo -e "${GREEN}✓ Checksums generated${NC}"
echo ""

cd ..

# Summary
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✅ All builds complete!${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo "Artifacts in ${DIST_DIR}:"
ls -lh ${DIST_DIR}
echo ""

echo "Next steps:"
echo "  1. Test each binary"
echo "  2. Create GitHub release"
echo "  3. Upload binaries: gh release upload v${VERSION} ${DIST_DIR}/*"
echo "  4. Publish to PyPI: twine upload ${DIST_DIR}/*.whl"
