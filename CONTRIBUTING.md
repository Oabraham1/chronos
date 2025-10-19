# Contributing to Chronos

Thank you for your interest in contributing to Chronos! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Submitting Changes](#submitting-changes)
- [Community](#community)

---

## Code of Conduct

We are committed to providing a welcoming and inclusive environment. Please be respectful and professional in all interactions.

**Our Standards:**
- Be respectful of differing viewpoints and experiences
- Accept constructive criticism gracefully
- Focus on what is best for the community
- Show empathy towards other community members

---

## Getting Started

### Prerequisites

- C++14 compatible compiler (GCC 7+, Clang 6+, MSVC 2019+)
- CMake 3.10 or higher
- OpenCL development libraries
- Python 3.7+ (for Python bindings)
- Git

### First Contribution?

Look for issues labeled with:
- `good first issue` - Great for newcomers
- `help wanted` - We'd love assistance on these
- `documentation` - Help improve our docs

---

## Development Setup

### 1. Fork and Clone
```bash
# Fork the repository on GitHub, then:
git clone https://github.com/YOUR_USERNAME/chronos.git
cd chronos
git remote add upstream https://github.com/oabraham1/chronos.git
```

### 2. Build from Source
```bash
# Quick build and test
./scripts/build_and_test.sh

# Or manual build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..
make -j$(nproc)
ctest --output-on-failure
```

### 3. Install Development Tools
```bash
# Code formatting
sudo apt-get install clang-format

# Python development
pip install -e ".[dev]"
```

### 4. Verify Setup
```bash
# Test CLI
./build/bin/chronos_cli help

# Test Python bindings
python3 -c "import chronos; print(chronos.__version__)"

# Run tests
cd build && ctest
```

---

## How to Contribute

### Reporting Bugs

**Before submitting a bug report:**
- Check existing issues to avoid duplicates
- Verify the bug exists in the latest version
- Collect relevant information (OS, version, error messages)

**Good bug reports include:**
- Clear, descriptive title
- Steps to reproduce
- Expected vs actual behavior
- Environment details (OS, GPU, OpenCL version)
- Relevant logs or error messages
- Minimal code example if applicable

### Suggesting Features

**Feature requests should include:**
- Clear use case and motivation
- Proposed API or interface (if applicable)
- Potential implementation approach
- Any alternatives considered

**Note:** We prioritize features that:
- Align with Chronos's core mission (temporary GPU partitions)
- Benefit multiple users
- Don't compromise performance or simplicity
- Maintain API stability

### Contributing Code

1. **Check for existing work** - Look for related issues or PRs
2. **Discuss major changes** - Open an issue first for significant features
3. **Start small** - Begin with bug fixes or small improvements
4. **Write tests** - All new features need tests
5. **Update docs** - Keep documentation in sync with code changes

---

## Coding Standards

### C++ Style

We use `clang-format` for consistent formatting with the Google style guide (4-space indentation, 100-column limit):
```bash
# Format all source files
find src include apps tests examples benchmarks -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
  -exec clang-format -i {} \;
```

**Key conventions:**
- 4 spaces for indentation (no tabs)
- Class names: `PascalCase`
- Function names: `snake_case`
- Constants: `UPPER_SNAKE_CASE`
- Member variables: `snake_case_` (trailing underscore)
- Smart pointers over raw pointers
- RAII for resource management

**Example:**
```cpp
class PartitionManager {
   public:
    PartitionManager(size_t total_memory);

    bool create_partition(int user_id, double fraction);
    void release_partition(int partition_id);

   private:
    size_t total_memory_;
    std::unique_ptr<OpenCLContext> context_;
};
```

### Python Style

Follow PEP 8:
```bash
# Install formatting tools
pip install black isort flake8

# Format code
black python/
isort python/
flake8 python/
```

### General Guidelines

**Code Quality:**
- Keep functions small and focused (< 50 lines preferred)
- Prefer composition over inheritance
- Document public APIs with clear docstrings/comments
- Handle errors gracefully with informative messages
- Avoid premature optimization

**Comments:**
- Explain *why*, not *what*
- Document complex algorithms
- Add TODO comments for known issues (but fix them before releases!)
- Use `// SAFETY:` for unsafe operations with justification

**Avoid:**
- Magic numbers (use named constants)
- Deep nesting (> 3 levels)
- Global state
- Overly clever code
- Platform-specific hacks without clear documentation

---

## Testing Guidelines

### Writing Tests

**All new code must include tests:**
```cpp
// C++ test example
#include <cassert>
#include "chronos.h"

int testPartitionCreation() {
    chronos::ChronosPartitioner partitioner;

    // Test valid parameters
    std::string id = partitioner.createPartition(0, 0.5, 10);
    assert(!id.empty());

    // Test invalid parameters
    std::string invalid = partitioner.createPartition(0, 1.5, 10);
    assert(invalid.empty());

    // Cleanup
    partitioner.releasePartition(id);

    return 0;
}
```
```python
# Python test example (using unittest)
import unittest
from chronos import Partitioner, ChronosError

class TestPartitioner(unittest.TestCase):
    def test_partition_context_manager(self):
        """Test that partition is automatically released."""
        p = Partitioner()

        with p.create(device=0, memory=0.3, duration=5) as partition:
            # Partition should exist
            partitions = p.list()
            self.assertEqual(len(partitions), 1)

        # Partition should be released after context exit
        partitions = p.list()
        self.assertEqual(len(partitions), 0)
```

### Running Tests
```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test
./tests/test_partitioner

# Run Python tests
cd python/tests
python3 -m unittest discover -v

# Run with verbose output
ctest -V

# Run with memory checking (Linux)
valgrind --leak-check=full ./tests/test_partitioner
```

### Test Coverage

- Aim for > 80% code coverage
- Test both success and failure paths
- Include edge cases (empty, null, boundary values)
- Test platform-specific code on multiple platforms

---

## Submitting Changes

### Before Submitting

**Checklist:**
- [ ] Code follows style guidelines (run clang-format)
- [ ] Tests added/updated and passing
- [ ] Documentation updated
- [ ] No compiler warnings
- [ ] Commit messages are clear
- [ ] Branch is up to date with main

### Commit Messages

Follow this format:
```
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain the problem this commit solves and why you chose
this particular solution.

Fixes #123
```

**Good commit messages:**
```
Add memory limit enforcement for partitions

Implements per-partition memory tracking and enforcement
to prevent partitions from exceeding allocated size.
Uses OpenCL memory queries to monitor usage.

Fixes #45
```

**Examples by type:**
- `feat: Add auto-expiration for temporary partitions`
- `fix: Prevent race condition in partition release`
- `docs: Update installation instructions for macOS`
- `test: Add edge cases for fraction validation`
- `refactor: Simplify partition creation logic`
- `perf: Optimize GPU memory allocation`

### Pull Request Process

1. **Create a feature branch:**
```bash
   git checkout -b feature/your-feature-name
```

2. **Make your changes with clear commits**

3. **Update from upstream:**
```bash
   git fetch upstream
   git rebase upstream/main
```

4. **Push to your fork:**
```bash
   git push origin feature/your-feature-name
```

5. **Open a Pull Request:**
   - Use a clear, descriptive title
   - Reference related issues
   - Describe what changed and why
   - Include testing notes
   - Add screenshots for UI changes

**PR Template:**
```markdown
## Description
Brief description of changes.

## Motivation
Why is this change needed? What problem does it solve?

## Changes
- Change 1
- Change 2

## Testing
How were these changes tested?

## Checklist
- [ ] Tests pass locally
- [ ] Code follows style guidelines
- [ ] Documentation updated
- [ ] No new warnings

Fixes #(issue number)
```

### Review Process

- Maintainers will review within 3-5 business days
- Address feedback by pushing new commits
- Once approved, maintainers will merge
- Celebrate! 🎉

---

## Community

### Getting Help

- **GitHub Issues:** Bug reports and feature requests
- **GitHub Discussions:** Questions, ideas, and general discussion
- **Documentation:** Check docs/ folder first

### Communication

- Be patient - maintainers are often volunteers
- Provide context in questions
- Share what you've already tried
- Be open to feedback

### Recognition

Contributors are recognized in:
- CHANGELOG.md for each release
- README.md contributors section
- Release notes

---

## Development Tips

### Performance Considerations

- Profile before optimizing
- Benchmark changes that affect performance
- Document any performance tradeoffs

### Debugging
```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Run with debugging
gdb ./build/bin/chronos_cli
lldb ./build/bin/chronos_cli

# Check for memory leaks
valgrind --leak-check=full ./build/bin/chronos_cli stats
```

### Documentation

- Keep examples up to date
- Test all code snippets in docs
- Use clear, simple language
- Include expected output

---

## Project Structure
```
chronos/
├── src/                    # C++ source files
│   ├── core/              # Core partitioning logic
│   ├── utils/             # Utility functions
│   ├── platform/          # Platform-specific code
│   └── config/            # Configuration management
├── include/               # Public headers
├── apps/                  # CLI application
├── python/                # Python bindings
├── tests/                 # Test files
├── examples/              # Example programs
├── benchmarks/            # Performance benchmarks
├── docs/                  # Documentation
└── scripts/               # Build and release scripts
```

---

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.

All source files must include the Apache 2.0 license header:
```cpp
/**
 * @file filename.cpp
 * @brief Brief description
 *
 * Copyright (c) 2025 Ojima Abraham
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Your Name
 * @date 2025
 */
```

---

## Questions?

If anything is unclear, please open an issue and ask! We're here to help.

**Thank you for contributing to Chronos!** 🚀
