# Chronos Troubleshooting Guide

Complete troubleshooting guide for common installation and runtime issues.

## Table of Contents

1. [Installation Issues](#installation-issues)
2. [Python Binding Issues](#python-binding-issues)
3. [Runtime Issues](#runtime-issues)
4. [Platform-Specific Issues](#platform-specific-issues)
5. [Diagnostic Tools](#diagnostic-tools)

---

## Installation Issues

### 1. "externally-managed-environment" Error (PEP 668)

**Problem:**
```
error: externally-managed-environment

× This environment is externally managed
```

**Cause:** Python 3.11+ on macOS (Homebrew) or some Linux distributions enforces PEP 668 to prevent breaking system packages.

**Solutions (in order of recommendation):**

#### Option 1: Use a Virtual Environment (Recommended)
```bash
# Create a virtual environment
python3 -m venv ~/.chronos-venv

# Activate it
source ~/.chronos-venv/bin/activate

# Install Chronos
pip install -e /path/to/chronos

# Use Chronos (within the venv)
python3 -c "from chronos import Partitioner; print('Success!')"

# Deactivate when done
deactivate
```

**To use later:**
```bash
source ~/.chronos-venv/bin/activate
python your_script.py
```

#### Option 2: User Installation
```bash
python3 -m pip install -e /path/to/chronos --user
```

#### Option 3: Break System Packages (Not Recommended)
```bash
python3 -m pip install -e /path/to/chronos --break-system-packages
```

**Best Practice:** Always use virtual environments for project-specific Python packages.

---

### 2. "chronos: command not found"

**Problem:** The `chronos` command isn't found in your shell.

**Cause:** `/usr/local/bin` is not in your PATH.

**Solution:**

```bash
# Check if binary exists
ls -la /usr/local/bin/chronos

# If it exists, add to PATH
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc  # or ~/.bashrc

# Apply immediately
source ~/.zshrc  # or source ~/.bashrc

# Test
chronos help
```

**Quick Fix:**
```bash
# Use full path
/usr/local/bin/chronos stats
```

---

### 3. "dyld: Library not loaded: @rpath/libchronos.dylib" (macOS)

**Problem:** The dynamic library can't be found at runtime.

**Cause:** `DYLD_LIBRARY_PATH` is not set.

**Solution:**

```bash
# Temporary fix (current terminal)
export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"
chronos stats

# Permanent fix
echo 'export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"' >> ~/.zshrc
source ~/.zshrc
```

---

### 4. Multiple Python Versions

**Problem:** Package installed to Python 3.9 but using Python 3.14.

**Diagnosis:**
```bash
# Check where package is installed
pip3 show chronos-gpu

# Check which Python you're using
which python3
python3 --version
```

**Solution:**
```bash
# Uninstall from all Python versions
python3.9 -m pip uninstall -y chronos-gpu  # if exists
python3.14 -m pip uninstall -y chronos-gpu  # if exists

# Install to the Python you actually use
python3 -m pip install -e /path/to/chronos --user

# Or use a virtual environment (recommended)
python3 -m venv ~/.chronos-venv
source ~/.chronos-venv/bin/activate
pip install -e /path/to/chronos
```

---

## Python Binding Issues

### 1. "ModuleNotFoundError: No module named 'chronos'"

**Diagnosis:**
```bash
# Check if package is installed
python3 -m pip list | grep chronos

# Check Python path
python3 -c "import sys; print('\n'.join(sys.path))"
```

**Solutions:**

**A. Package not installed:**
```bash
python3 -m pip install -e /path/to/chronos --user
```

**B. Installed to different Python:**
```bash
# Find which Python has it
python3.9 -m pip show chronos-gpu
python3.14 -m pip show chronos-gpu

# Use the correct Python
python3.14 -m pip install -e /path/to/chronos --user
```

**C. Virtual environment solution:**
```bash
python3 -m venv ~/.chronos-venv
source ~/.chronos-venv/bin/activate
pip install -e /path/to/chronos
```

---

### 2. "Could not find Chronos library"

**Problem:** Python bindings installed but can't find `libchronos.so`/`libchronos.dylib`.

**Solution:**

**Linux:**
```bash
export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"
python3 -c "from chronos import Partitioner"
```

**macOS:**
```bash
export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"
python3 -c "from chronos import Partitioner"
```

**Permanent Fix:**
```bash
# Linux
echo 'export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"' >> ~/.bashrc

# macOS
echo 'export DYLD_LIBRARY_PATH="/usr/local/lib:${DYLD_LIBRARY_PATH}"' >> ~/.zshrc
```

---

## Runtime Issues

### 1. "No OpenCL platforms found"

**Problem:** OpenCL runtime not installed or GPU not detected.

**Solutions:**

**Ubuntu/Debian:**
```bash
sudo apt-get install ocl-icd-libopencl1

# Install vendor-specific runtime
# NVIDIA: CUDA toolkit
# AMD: ROCm or AMDGPU-PRO
# Intel: Intel Compute Runtime

# Verify
clinfo  # Should list your GPU
```

**macOS:**
OpenCL is built into macOS. If you see this error:
- Ensure you're on macOS 10.14+ (earlier versions had limited OpenCL support)
- Check if your GPU is supported: Apple Menu → About This Mac → Graphics

**Windows:**
Install your GPU vendor's SDK:
- NVIDIA: [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads)
- AMD: [ROCm](https://rocmdocs.amd.com/)
- Intel: [oneAPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/overview.html)

---

### 2. "Failed to create lock"

**Problem:** Lock directory doesn't exist or has wrong permissions.

**Solution:**
```bash
sudo mkdir -p /tmp/chronos_locks
sudo chmod 777 /tmp/chronos_locks
```

---

### 3. "GPU portion is locked by another user"

**Problem:** Another user has a partition using that memory range.

**Solutions:**

**A. Check active partitions:**
```bash
chronos list
```

**B. Wait for expiration:**
Partitions automatically expire based on their duration.

**C. Use a different memory fraction:**
```bash
# Instead of 0.5, try 0.3 or 0.2
chronos create 0 0.3 3600
```

**D. Admin can force release (if implemented):**
```bash
sudo chronos release partition_0001
```

---

## Platform-Specific Issues

### macOS

#### Issue: "Operation not permitted" when creating locks

**Solution:**
```bash
# Grant Full Disk Access to Terminal
# System Preferences → Security & Privacy → Full Disk Access → Add Terminal
```

#### Issue: Gatekeeper blocking chronos binary

**Solution:**
```bash
# Remove quarantine attribute
xattr -d com.apple.quarantine /usr/local/bin/chronos

# Or approve in System Preferences
# System Preferences → Security & Privacy → General → Allow chronos
```

---

### Linux

#### Issue: "Permission denied" accessing GPU

**Solution:**
```bash
# Add user to video/render group
sudo usermod -a -G video $USER
sudo usermod -a -G render $USER

# Log out and back in for changes to take effect
```

#### Issue: SELinux blocking access

**Solution:**
```bash
# Adjust SELinux permissions
sudo semanage fcontext -a -t bin_t "/usr/local/bin/chronos"
sudo restorecon -v /usr/local/bin/chronos
```

---

### Windows

#### Issue: "Access denied" when creating partitions

**Solution:**
```powershell
# Run PowerShell as Administrator
# Right-click PowerShell → Run as Administrator
chronos create 0 0.5 3600
```

---

## Diagnostic Tools

### Chronos Doctor

Run the diagnostic script to check your installation:

```bash
chmod +x scripts/chronos-doctor.sh
./scripts/chronos-doctor.sh
```

**What it checks:**
- System information
- PATH configuration
- Binary installation
- Library dependencies
- Python environment
- Python bindings
- OpenCL environment
- Lock directory

**Example output:**
```
✓ CLI installed successfully
✓ chronos stats works
✗ Python bindings not available
  Try: python3 -m pip install -e . --user
```

---

### Manual Diagnosis

#### Check Installation

```bash
# 1. Check binary
which chronos
ls -la /usr/local/bin/chronos

# 2. Check library
ls -la /usr/local/lib/libchronos.*

# 3. Test CLI
chronos help
chronos stats

# 4. Check Python
python3 -c "import chronos; print(chronos.__version__)"
```

#### Check OpenCL

```bash
# Install clinfo
# Ubuntu: sudo apt install clinfo
# macOS: brew install clinfo

# List OpenCL devices
clinfo

# Check if Chronos can see devices
chronos stats
```

#### Check Permissions

```bash
# Check lock directory
ls -ld /tmp/chronos_locks

# Should be: drwxrwxrwx (777)
# If not:
sudo chmod 777 /tmp/chronos_locks
```

---

## Common Error Messages

### "Invalid memory fraction. Must be between 0 and 1"

**Solution:** Use decimal values between 0.0 and 1.0:
```bash
# Wrong
chronos create 0 50 3600

# Correct
chronos create 0 0.5 3600
```

---

### "Not enough available memory on device"

**Solution:** Request less memory or wait for partitions to expire:
```bash
# Check availability
chronos available 0

# Request less
chronos create 0 0.3 3600  # 30% instead of 50%
```

---

## Getting More Help

### Before Asking for Help

1. Run the doctor script:
   ```bash
   ./scripts/chronos-doctor.sh
   ```

2. Check your environment:
   ```bash
   echo "OS: $(uname -s) $(uname -r)"
   echo "Python: $(python3 --version)"
   echo "Chronos: $(chronos --version 2>&1 || echo 'not installed')"
   clinfo 2>&1 | head -20
   ```

3. Try the examples:
   ```bash
   cd examples
   ../build/bin/simple_partition
   ```

### Where to Get Help

- **GitHub Issues:** https://github.com/oabraham1/chronos/issues
- **Discussions:** https://github.com/oabraham1/chronos/discussions
- **Email:** abrahamojima2018@gmail.com

### What to Include

When reporting issues, include:

1. Output of `./scripts/chronos-doctor.sh`
2. Operating system and version
3. Python version: `python3 --version`
4. Error message (full text)
5. Steps to reproduce
6. What you've already tried

---

## Quick Reference

### Reset Everything

```bash
# Remove installation
cd build
sudo make uninstall

# Clean build
cd ..
rm -rf build/

# Remove Python package
python3 -m pip uninstall -y chronos-gpu

# Remove locks
rm -rf /tmp/chronos_locks/

# Reinstall
./scripts/install-quick.sh
```

### Fresh Virtual Environment

```bash
# Remove old venv
rm -rf ~/.chronos-venv

# Create new venv
python3 -m venv ~/.chronos-venv
source ~/.chronos-venv/bin/activate

# Install Chronos
pip install -e /path/to/chronos

# Test
python3 -c "from chronos import Partitioner; print('Success!')"
```

---

**Last Updated:** October 2025

**Need more help?** Open an issue at https://github.com/oabraham1/chronos/issues
