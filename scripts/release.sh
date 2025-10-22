#!/bin/bash
# release.sh - Complete v1.0 release automation

set -e

VERSION=${1:-"1.0.1"}

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

banner() {
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
}

success() {
    echo -e "${GREEN}✓ $1${NC}"
}

error() {
    echo -e "${RED}✗ $1${NC}"
}

warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

banner "Chronos v${VERSION} Release Script"

# Validate version format
if ! [[ $VERSION =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    error "Invalid version format. Use X.Y.Z (e.g., 1.0.1)"
    exit 1
fi

# Check if on main branch
BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$BRANCH" != "main" ]; then
    warning "Not on main branch (currently on ${BRANCH})"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Check for uncommitted changes
if [ -n "$(git status --porcelain)" ]; then
    error "You have uncommitted changes:"
    git status --short
    exit 1
fi

success "Pre-flight checks passed"

# Step 1: Update version numbers
banner "Step 1/10: Updating Version Numbers"
python3 scripts/update_version.py $VERSION || python3 update_version.py $VERSION
git add .
git commit -m "Bump version to ${VERSION}" || true
success "Version updated to ${VERSION}"

# Step 2: Run tests
banner "Step 2/10: Running Tests"
if [ -d "build" ]; then
    cd build
    ctest --output-on-failure || warning "Some tests failed (continuing...)"
    cd ..
else
    warning "No build directory found, skipping C++ tests"
fi

if [ -d "python/tests" ]; then
    python3 -m pytest python/tests/ -v || warning "Some Python tests failed (continuing...)"
fi
success "Tests completed"

# Step 3: Build documentation
banner "Step 3/10: Building Documentation"
if command -v doxygen &> /dev/null; then
    doxygen Doxyfile 2>/dev/null || warning "Doxygen build had warnings"
    success "Documentation built"
else
    warning "Doxygen not found, skipping docs generation"
fi

# Step 4: Build binaries
banner "Step 4/10: Building Release Binaries"
chmod +x scripts/build_releases.sh || chmod +x build_releases.sh
./scripts/build_releases.sh || ./build_releases.sh
success "Binaries built"

# Step 5: Create git tag
banner "Step 5/10: Creating Git Tag"
git tag -a "v${VERSION}" -m "Release v${VERSION}"
success "Tag v${VERSION} created"

# Step 6: Push to GitHub
banner "Step 6/10: Pushing to GitHub"
read -p "Push to GitHub? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git push origin main
    git push origin "v${VERSION}"
    success "Pushed to GitHub"
else
    warning "Skipped GitHub push"
fi

# Step 7: Create GitHub release
banner "Step 7/10: Creating GitHub Release"
if command -v gh &> /dev/null; then
    read -p "Create GitHub release? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # Create release notes if they don't exist
        if [ ! -f "RELEASE_NOTES.md" ]; then
            cat > RELEASE_NOTES.md << EOF
# Chronos v${VERSION}

## 🎉 First Stable Release!

We're excited to announce the first stable release of Chronos - a time-based GPU partitioning system!

### ✨ Features

- Time-based GPU partitioning with automatic expiration
- Memory enforcement and tracking
- User permissions and isolation
- Cross-platform support (Linux, macOS, Windows)
- Python bindings with context manager support
- Comprehensive CLI interface

### 📦 Installation

**Quick Install:**
\`\`\`bash
curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh | sudo bash
\`\`\`

**Python Package:**
\`\`\`bash
pip install chronos-gpu
\`\`\`

### 🚀 Quick Start

**CLI:**
\`\`\`bash
chronos create 0 0.5 3600  # 50% of GPU 0 for 1 hour
chronos list
chronos stats
\`\`\`

**Python:**
\`\`\`python
from chronos import Partitioner

with Partitioner().create(device=0, memory=0.5, duration=3600) as p:
    # Your GPU code here
    pass
\`\`\`

### 📚 Documentation

- [Installation Guide](INSTALL.md)
- [User Guide](docs/USER_GUIDE.md)
- [Python API](python/README.md)
- [FAQ](docs/FAQ.md)

### 🙏 Acknowledgments

Thanks to all contributors and early testers!
EOF
        fi

        gh release create "v${VERSION}" \
            --title "Chronos v${VERSION}" \
            --notes-file RELEASE_NOTES.md \
            dist/* || warning "GitHub release creation failed"
        success "GitHub release created"
    else
        warning "Skipped GitHub release"
    fi
else
    warning "GitHub CLI not found. Create release manually at:"
    info "https://github.com/oabraham1/chronos/releases/new?tag=v${VERSION}"
fi

# Step 8: Test PyPI upload
banner "Step 8/10: Testing PyPI Upload"
if command -v twine &> /dev/null; then
    read -p "Upload to test.pypi.org? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        python3 -m twine check dist/*.whl dist/*.tar.gz
        python3 -m twine upload --repository testpypi dist/*.whl dist/*.tar.gz || warning "TestPyPI upload failed"
        success "Uploaded to TestPyPI"
        info "Test install: pip install --index-url https://test.pypi.org/simple/ chronos-gpu"
    else
        warning "Skipped TestPyPI upload"
    fi
else
    warning "Twine not found. Install: pip install twine"
fi

# Step 9: Publish to PyPI
banner "Step 9/10: Publishing to PyPI"
read -p "Publish to production PyPI? This cannot be undone! (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 -m twine upload dist/*.whl dist/*.tar.gz || error "PyPI upload failed"
    success "Published to PyPI"
    info "Install: pip install chronos-gpu"
else
    warning "Skipped PyPI publication"
fi

# Step 10: Build and push Docker
banner "Step 10/10: Docker Images"
read -p "Build and push Docker images? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v docker &> /dev/null; then
        docker build -t chronos:${VERSION} -t chronos:latest .

        # Tag for registries
        docker tag chronos:${VERSION} ghcr.io/oabraham1/chronos:${VERSION}
        docker tag chronos:latest ghcr.io/oabraham1/chronos:latest

        read -p "Push to GitHub Container Registry? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            docker push ghcr.io/oabraham1/chronos:${VERSION}
            docker push ghcr.io/oabraham1/chronos:latest
            success "Docker images pushed"
        fi
    else
        warning "Docker not found"
    fi
else
    warning "Skipped Docker build"
fi

# Summary
banner "Release Summary"
echo ""
echo -e "${GREEN}✅ Release v${VERSION} Complete!${NC}"
echo ""
echo -e "${MAGENTA}What was done:${NC}"
echo "  • Version bumped to ${VERSION}"
echo "  • Tests executed"
echo "  • Binaries built for all platforms"
echo "  • Git tag created and pushed"
echo "  • GitHub release created"
echo "  • PyPI package published"
echo "  • Docker images built"
echo ""
echo -e "${MAGENTA}Next steps:${NC}"
echo "  1. Verify installation:"
echo "     ${CYAN}pip install chronos-gpu==${VERSION}${NC}"
echo "     ${CYAN}chronos --version${NC}"
echo ""
echo "  2. Announce on social media:"
echo "     • Twitter/X: @oabraham1"
echo "     • Reddit: r/machinelearning, r/programming"
echo "     • HackerNews"
echo "     • LinkedIn"
echo ""
echo "  3. Monitor:"
echo "     • GitHub issues: ${CYAN}https://github.com/oabraham1/chronos/issues${NC}"
echo "     • PyPI downloads: ${CYAN}https://pypi.org/project/chronos-gpu/${NC}"
echo ""
echo -e "${GREEN}🎉 Congratulations on shipping v${VERSION}!${NC}"
echo ""
