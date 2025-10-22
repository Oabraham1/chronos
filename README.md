# Chronos

**Fair GPU Time-Sharing for Everyone**

[![CI](https://github.com/oabraham1/chronos/workflows/Chronos%20CI/badge.svg)](https://github.com/oabraham1/chronos/actions)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![PyPI](https://img.shields.io/pypi/v/chronos-gpu)](https://pypi.org/project/chronos-gpu/)
[![Downloads](https://img.shields.io/pypi/dm/chronos-gpu)](https://pypi.org/project/chronos-gpu/)
[![Documentation](https://img.shields.io/badge/docs-latest-brightgreen)](https://oabraham1.github.io/chronos/)

> **Time-based GPU partitioning with automatic expiration**
> Simple. Fair. Just works.™

```python
from chronos import Partitioner

# Get 50% of GPU 0 for 1 hour - guaranteed
with Partitioner().create(device=0, memory=0.5, duration=3600) as partition:
    train_model()  # Your code here
    # Auto-cleanup when done
```

---

## The Problem

You have **one expensive GPU** and **multiple users** who need it.

**Without Chronos:**
```
❌ Resource conflicts and crashes
❌ No fair allocation
❌ Manual coordination required
❌ Wasted compute time
❌ Politics and frustration
```

**With Chronos:**
```
✅ Everyone gets guaranteed time
✅ Automatic resource cleanup
✅ Zero conflicts
✅ < 1% performance overhead
✅ No manual coordination
```

---

## Quick Start

### Install

```bash
# PyPI (recommended)
pip install chronos-gpu

# Or quick script
curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh | sudo bash

# Or from source
git clone https://github.com/oabraham1/chronos
cd chronos && ./install-quick.sh
```

### Use (CLI)

```bash
# Check your GPUs
chronos stats

# Allocate 50% of GPU 0 for 1 hour
chronos create 0 0.5 3600

# List active partitions
chronos list

# It auto-expires - no cleanup needed!
```

### Use (Python)

```python
from chronos import Partitioner

p = Partitioner()

# Simple usage
with p.create(device=0, memory=0.5, duration=3600) as partition:
    import torch
    model = torch.nn.Sequential(...).cuda()
    model.fit(X, y)
    # Automatic cleanup
```

---

## Why Chronos?

### 🎯 Fair Allocation
Time-based partitions mean no resource hogging. Everyone gets their fair share.

### ⚡ Ultra-Fast
- **3.2ms** partition creation
- **< 1%** GPU overhead
- **Sub-second** expiration accuracy

### 🔒 Isolated & Safe
- Per-user partitions
- Memory enforcement
- Automatic expiration
- No manual cleanup

### 🌍 Universal
- **Any GPU**: NVIDIA, AMD, Intel, Apple Silicon
- **Any OS**: Linux, macOS, Windows
- **Any Framework**: PyTorch, TensorFlow, JAX, etc.

### 🎓 Perfect For
- **Research labs** with shared GPUs
- **Small teams** with limited hardware
- **Universities** with many students
- **Development** environments

---

## Features

| Feature | Chronos | NVIDIA MIG | MPS | Time-Slicing |
|---------|---------|------------|-----|--------------|
| Time-based allocation | ✅ | ❌ | ❌ | ❌ |
| Auto-expiration | ✅ | ❌ | ❌ | ❌ |
| Multi-vendor GPU | ✅ | ❌ | ❌ | ✅ |
| User isolation | ✅ | ✅ | ❌ | ❌ |
| Zero setup | ✅ | ❌ | ❌ | ❌ |
| < 1% overhead | ✅ | ✅ | ✅ | ❌ |

---

## Examples

### Research Lab Setup

```bash
#!/bin/bash
# Allocate GPU for the team every morning

chronos create 0 0.30 28800 --user alice   # 30%, 8 hours
chronos create 0 0.20 28800 --user bob     # 20%, 8 hours
chronos create 0 0.15 28800 --user carol   # 15%, 8 hours
# 35% left for ad-hoc use
```

### ML Training with Auto-Save

```python
from chronos import Partitioner
import torch

with Partitioner().create(device=0, memory=0.5, duration=14400) as p:
    model = MyModel().cuda()

    for epoch in range(1000):
        train_epoch(model)

        # Auto-save when time is running out
        if p.time_remaining < 600:  # 10 minutes left
            torch.save(model.state_dict(), 'checkpoint.pt')
            print("Checkpoint saved!")
            break
```

### Jupyter Notebook

```python
from chronos import Partitioner

# At the start of your notebook
p = Partitioner()
partition = p.create(device=0, memory=0.5, duration=7200)  # 2 hours

# Your analysis here
import tensorflow as tf
model = build_model()
model.fit(data)

# Check remaining time
print(f"Time left: {partition.time_remaining}s")

# Release when done (or it auto-expires)
partition.release()
```

---

## Performance

Benchmarked on Ubuntu 22.04 with NVIDIA RTX 3080:

| Operation | Latency | Overhead |
|-----------|---------|----------|
| Create partition | 3.2ms ± 0.5ms | - |
| Release partition | 1.8ms ± 0.3ms | - |
| GPU compute | - | **0.8%** |
| Memory tracking | 0.1ms | - |

**24-hour stress test:** 1.2M operations, zero failures, zero memory leaks.

[Full benchmarks →](docs/BENCHMARKS.md)

---

## Documentation

- **[Installation Guide](INSTALL.md)** - Platform-specific setup
- **[User Guide](docs/USER_GUIDE.md)** - Complete tutorial
- **[API Reference](https://oabraham1.github.io/chronos/)** - Full API docs
- **[FAQ](docs/FAQ.md)** - Common questions
- **[Benchmarks](docs/BENCHMARKS.md)** - Performance data

---

## Installation Methods

### PyPI (Recommended)
```bash
pip install chronos-gpu
```

### Quick Install Script
```bash
# Linux/macOS
curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install.sh | sudo bash

# Or user install (no sudo)
curl -sSL https://raw.githubusercontent.com/oabraham1/chronos/main/install-user.sh | bash
```

### Docker
```bash
docker pull ghcr.io/oabraham1/chronos:latest
docker run --gpus all ghcr.io/oabraham1/chronos:latest chronos stats
```

### From Source
```bash
git clone https://github.com/oabraham1/chronos
cd chronos
mkdir build && cd build
cmake .. && make
sudo make install
```

[Full installation guide →](INSTALL.md)

---

## Architecture

```
┌─────────────────────────────────────────┐
│           User Applications             │
│    (PyTorch, TensorFlow, JAX, etc.)     │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│         Chronos Partitioner             │
│  ┌──────────────────────────────────┐   │
│  │  Time-Based Allocation Engine    │   │
│  └──────────────────────────────────┘   │
│  ┌──────────────────────────────────┐   │
│  │    Memory Enforcement Layer      │   │
│  └──────────────────────────────────┘   │
│  ┌──────────────────────────────────┐   │
│  │   Auto-Expiration Monitor        │   │
│  └──────────────────────────────────┘   │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│         OpenCL Runtime Layer            │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│      GPU Hardware (Any Vendor)          │
└─────────────────────────────────────────┘
```

**Key Components:**
- **C++ Core**: High-performance partition management
- **Python Bindings**: Easy-to-use API
- **CLI Tool**: Command-line interface
- **Monitor Thread**: Automatic expiration handling
- **Lock Files**: Inter-process coordination

---

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

**Good first issues:**
- Add more examples
- Improve error messages
- Write tests
- Update documentation
- Fix bugs

---

## Community

- **Discord**: [Join our community](https://discord.gg/chronos) *(coming soon)*
- **Discussions**: [GitHub Discussions](https://github.com/oabraham1/chronos/discussions)
- **Twitter**: [@oabraham1](https://twitter.com/chef_jiggy)
- **Issues**: [Report bugs](https://github.com/oabraham1/chronos/issues)

---

## Citation

If you use Chronos in research, please cite:

```bibtex
@software{chronos2025,
  title={Chronos: Time-Based GPU Partitioning for Fair Resource Sharing},
  author={Abraham, Ojima},
  year={2025},
  url={https://github.com/oabraham1/chronos},
  version={1.0.1}
}
```

---

## License

**Apache License 2.0** - Use it anywhere, for anything.

See [LICENSE](LICENSE) for full terms.

---

## Support

- **Documentation**: [Full docs](https://oabraham1.github.io/chronos/)
- **Email**: abrahamojima2018@gmail.com
- **Issues**: [GitHub Issues](https://github.com/oabraham1/chronos/issues)

---

## Acknowledgments

Thanks to all contributors and early adopters who helped shape Chronos!

Special thanks to the open-source community for inspiration and support.

---

<div align="center">

**Made with ❤️ by researchers, for researchers**

[⭐ Star us on GitHub](https://github.com/oabraham1/chronos) • [📦 Install from PyPI](https://pypi.org/project/chronos-gpu/) • [📚 Read the docs](https://oabraham1.github.io/chronos/)

</div>
