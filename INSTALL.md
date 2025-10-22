# Chronos Installation Guide

Complete installation instructions for all platforms and use cases.

## Quick Install (Recommended)

### Linux / macOS

```bash
# Clone repository
git clone https://github.com/oabraham1/chronos.git
cd chronos

# Run quick installer
chmod +x scripts/install-quick.sh
./scripts/install-quick.sh
```

This script will automatically:
1. ✅ Install system dependencies
2. ✅ Build Chronos from source
3. ✅ Install system-wide
4. ✅ Set up Python bindings (with PEP 668 handling)
5. ✅ Configure library paths
6. ✅ Run verification tests

**Note for macOS/Homebrew Python users:** The installer will detect PEP 668 restrictions and offer you a choice of installation methods (virtual environment recommended).

### Windows

```powershell
# Using Git Bash or WSL (recommended)
git clone https://github.com/oabraham1/chronos.git
cd chronos

# Build with CMake
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
cmake --install .
```

---

## Manual Installation

### Prerequisites

#### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ocl-icd-opencl-dev \
    opencl-headers \
    python3 \
    python3-pip \
    python3-venv \
    git
```

#### Fedora / RHEL / CentOS

```bash
sudo dnf install -y \
    gcc \
    gcc-c++ \
    cmake \
    ocl-icd-devel \
    opencl-headers \
    python3 \
    python3-pip \
    git
```

#### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake python3 git
```

OpenCL is built into macOS, no additional installation needed.

#### Windows

1. Install [Visual Studio 2019+](https://visualstudio.microsoft.com/) with C++ tools
2. Install [CMake](https://cmake.org/download/)
3. Install [Python 3.7+](https://www.python.org/downloads/)
4. Install GPU vendor SDK:
   - NVIDIA: [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads)
   - AMD: [ROCm](https://rocmdocs.amd.com/)
   - Intel: [oneAPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/overview.html)

### Build from Source

```bash
# Clone repository
git clone https://github.com/oabraham1/chronos.git
cd chronos

# Create build directory
mkdir build
cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTS=ON \
      -DBUILD_EXAMPLES=ON \
      ..

# Build (use appropriate number of cores)
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS

# Run tests
ctest --output-on-failure

# Install
sudo make install
```

### Install Python Bindings

**Important:** Python 3.11+ on macOS (Homebrew) and some Linux distributions enforce PEP 668. Choose the appropriate method below.

#### Method 1: Virtual Environment (Recommended)

```bash
# Create virtual environment
python3 -m venv ~/.chronos-venv

# Activate it
source ~/.chronos-venv/bin/activate

# Clean old artifacts (if any)
rm -rf python/chronos_gpu.egg-info

# Install Chronos
pip install -e .

# Verify
python3 -c "from chronos import Partitioner; print('Success!')"

# Deactivate when done
deactivate
```

**To use later:**
```bash
source ~/.chronos-venv/bin/activate
python3 your_script.py
```

#### Method 2: User Site-Packages

```bash
# Clean old artifacts
rm -rf python/chronos_gpu.egg-info

# Install to user directory
python3 -m pip install -e . --user

# Verify
python3 -c "from chronos import Partitioner; print('Success!')"
```

#### Method 3: System-Wide (requires --break-system-packages on some systems)

```bash
# For systems with PEP 668 (not recommended)
python3 -m pip install -e . --break-system-packages

# For systems without PEP 668
sudo pip3 install -e .
```

#### Method 4: Use the Helper Script

```bash
# Interactive installation with automatic cleanup
chmod +x scripts/fix-python-bindings.sh
./scripts/fix-python-bindings.sh
```

### Post-Installation Setup

#### Create Lock Directory

```bash
sudo mkdir -p /tmp/chronos_locks
sudo chmod 777 /tmp/chronos_locks
```

#### Configure Library Path (macOS)

```bash
# Add to shell config
echo 'export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"' >> ~/.zshrc
source ~/.zshrc
```

#### Configure Library Path (Linux)

```bash
# Add to shell config
echo 'export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"' >> ~/.bashrc
source ~/.bashrc
```

#### Verify Installation

```bash
# Test CLI
chronos help
chronos stats

# Test Python bindings
python3 -c "from chronos import Partitioner; print('Success!')"
```

---

## Docker Installation

### Using Pre-built Image (Coming Soon)

```bash
docker pull ghcr.io/oabraham1/chronos:latest
docker run --rm --gpus all ghcr.io/oabraham1/chronos:latest stats
```

### Build from Source

```bash
# Clone repository
git clone https://github.com/oabraham1/chronos.git
cd chronos

# Build Docker image
docker build -t chronos:latest .

# Run
docker run --rm --gpus all chronos:latest help
```

### Docker Compose

```bash
# Start services
docker-compose up -d

# Use Chronos
docker-compose exec chronos create 0 0.5 3600
docker-compose exec chronos list

# Stop services
docker-compose down
```

For NVIDIA GPUs, ensure [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html) is installed:

```bash
# Ubuntu/Debian
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
    sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update && sudo apt-get install -y nvidia-container-toolkit
sudo systemctl restart docker
```

---

## Platform-Specific Notes

### Linux

#### Permissions

On some systems, GPU access requires specific group membership:

```bash
# Add user to video/render group
sudo usermod -a -G video $USER
sudo usermod -a -G render $USER

# Log out and back in for changes to take effect
```

#### SELinux

If using SELinux (RHEL/CentOS/Fedora), you may need to adjust permissions:

```bash
sudo semanage fcontext -a -t bin_t "/usr/local/bin/chronos"
sudo restorecon -v /usr/local/bin/chronos
```

### macOS

#### Apple Silicon (M1/M2/M3/M4)

Chronos works natively on Apple Silicon:

```bash
# Build for native architecture
arch -arm64 ./scripts/install-quick.sh
```

#### Homebrew Python (PEP 668)

If you encounter "externally-managed-environment" errors:

1. **Use virtual environment** (recommended):
   ```bash
   python3 -m venv ~/.chronos-venv
   source ~/.chronos-venv/bin/activate
   pip install -e .
   ```

2. **Use --user flag**:
   ```bash
   python3 -m pip install -e . --user
   ```

3. **Use the fix script**:
   ```bash
   ./scripts/fix-python-bindings.sh
   ```

#### Gatekeeper

First run may trigger Gatekeeper:

```bash
# Remove quarantine attribute
xattr -d com.apple.quarantine /usr/local/bin/chronos
```

#### Library Path Issues

If you see "dyld: Library not loaded" errors:

```bash
export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"
echo 'export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"' >> ~/.zshrc
```

### Windows

#### Administrator Privileges

Some operations require administrator privileges:

```powershell
# Run PowerShell as Administrator
chronos create 0 0.5 3600 --user alice
```

#### PATH Setup

Add Chronos to PATH:

```powershell
$env:Path += ";C:\Program Files\Chronos\bin"
```

Or permanently via System Properties → Environment Variables.

---

## Verification

### Test C++ CLI

```bash
# Show help
chronos help

# Show device information
chronos stats

# Check availability
chronos available 0

# Create test partition
chronos create 0 0.1 5
chronos list

# Wait 5 seconds or release manually
chronos release partition_0001
```

### Test Python Bindings

```python
from chronos import Partitioner

# Create partitioner
p = Partitioner()

# Show stats
p.show_stats()

# Check availability
available = p.get_available(device=0)
print(f"GPU 0: {available:.1f}% available")

# Create and release partition
with p.create(device=0, memory=0.1, duration=5) as partition:
    print(f"Created: {partition.partition_id}")

print("Test passed!")
```

### Run Test Suite

```bash
# C++ tests
cd build
ctest --verbose

# Python tests
cd python/tests
python3 -m unittest discover -v
```

---

## Troubleshooting

### Run Diagnostic Script

```bash
chmod +x scripts/chronos-doctor.sh
./scripts/chronos-doctor.sh
```

This will check:
- System information
- PATH configuration
- Binary and library installation
- Python environment and bindings
- OpenCL environment
- Lock directory permissions

### Common Issues

#### 1. "externally-managed-environment" Error

See [Python Bindings Installation](#install-python-bindings) above for solutions.

#### 2. "No OpenCL platforms found"

**Cause:** OpenCL runtime not installed or GPU not detected

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install ocl-icd-libopencl1

# Verify
clinfo  # Should list your GPU
```

#### 3. "Failed to create lock"

**Cause:** Lock directory doesn't exist or wrong permissions

**Solution:**
```bash
sudo mkdir -p /tmp/chronos_locks
sudo chmod 777 /tmp/chronos_locks
```

#### 4. "Command not found: chronos"

**Cause:** Binary not in PATH

**Solution:**
```bash
# Add to PATH
export PATH="/usr/local/bin:$PATH"
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc

# Or use full path
/usr/local/bin/chronos help
```

#### 5. "Could not find Chronos library" (Python)

**Cause:** Shared library not found

**Solution:**
```bash
# Linux
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
echo 'export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"' >> ~/.bashrc

# macOS
export DYLD_LIBRARY_PATH="/usr/local/lib:$DYLD_LIBRARY_PATH"
echo 'export DYLD_LIBRARY_PATH="/usr/local/lib:$DYLD_LIBRARY_PATH"' >> ~/.zshrc
```

#### 6. Permission Denied on egg-info Directory

**Cause:** Old build artifacts from previous installation

**Solution:**
```bash
# Clean up old artifacts
rm -rf python/chronos_gpu.egg-info
# Or use sudo if needed
sudo rm -rf python/chronos_gpu.egg-info

# Then reinstall
python3 -m pip install -e . --user
```

### Fix Python Bindings Script

For persistent Python binding issues:

```bash
chmod +x scripts/fix-python-bindings.sh
./scripts/fix-python-bindings.sh
```

### Complete Troubleshooting Guide

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for the complete troubleshooting guide with detailed solutions.

---

## Uninstallation

### Remove Chronos

```bash
# From build directory
cd build
sudo make uninstall

# Or manually
sudo rm /usr/local/bin/chronos
sudo rm /usr/local/lib/libchronos.*
sudo rm -rf /usr/local/include/chronos

# Remove Python bindings
pip3 uninstall chronos-gpu

# Or from virtual environment
source ~/.chronos-venv/bin/activate
pip uninstall chronos-gpu
deactivate
```

### Clean Build

```bash
# Remove build artifacts
rm -rf build/

# Remove Python artifacts
rm -rf python/chronos_gpu.egg-info
rm -rf python/**/__pycache__

# Remove lock files
rm -rf /tmp/chronos_locks/

# Remove virtual environment
rm -rf ~/.chronos-venv
```

---

## Next Steps

After installation:

1. **Quick Start:** Read [README.md](README.md) for basic usage
2. **User Guide:** See [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for detailed documentation
3. **Python Guide:** Check [python/README.md](python/README.md) for Python API
4. **Examples:** Explore [examples/](examples/) directory
5. **Troubleshooting:** See [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

---

## Getting Help

- **Diagnostic Tool:** `./scripts/chronos-doctor.sh`
- **Fix Python:** `./scripts/fix-python-bindings.sh`
- **Documentation:** https://github.com/oabraham1/chronos
- **Issues:** https://github.com/oabraham1/chronos/issues
- **Discussions:** https://github.com/oabraham1/chronos/discussions
- **Email:** abrahamojima2018@gmail.com

---

## Contributing

Want to contribute? See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## License

Apache License 2.0 - See [LICENSE](LICENSE) for details.
