"""
Chronos Stub Module

This module provides stub implementations when the native Chronos library
is not available. It allows the package to be imported and provides helpful
error messages explaining how to fix the installation.
"""

import sys
from typing import Optional, List
from dataclasses import dataclass


class ChronosError(Exception):
    """Exception raised for Chronos-related errors."""
    pass


class ChronosStubError(ChronosError):
    """Exception raised when native library is not available."""

    def __init__(self, message: str = None):
        install_help = self._get_install_help()
        if message:
            full_message = f"{message}\n\n{install_help}"
        else:
            full_message = install_help
        super().__init__(full_message)

    def _get_install_help(self) -> str:
        """Get platform-specific installation help."""
        if sys.platform == "darwin":
            return (
                "Chronos native library not available.\n"
                "This usually means the build failed during pip install.\n\n"
                "To fix on macOS:\n"
                "  1. Ensure Xcode command line tools are installed:\n"
                "     xcode-select --install\n"
                "  2. OpenCL is built-in on macOS, but GPU drivers may need updating\n"
                "  3. Reinstall: pip install --force-reinstall chronos-gpu\n\n"
                "For NVIDIA GPUs, install CUDA Toolkit for MPS support."
            )
        elif sys.platform == "win32":
            return (
                "Chronos native library not available.\n"
                "This usually means the build failed during pip install.\n\n"
                "To fix on Windows:\n"
                "  1. Install Visual Studio Build Tools (with C++ support)\n"
                "  2. Install GPU drivers with OpenCL support:\n"
                "     - NVIDIA: Install CUDA Toolkit\n"
                "     - AMD: Install AMD GPU drivers\n"
                "     - Intel: Install Intel Graphics drivers\n"
                "  3. Reinstall: pip install --force-reinstall chronos-gpu"
            )
        else:  # Linux
            return (
                "Chronos native library not available.\n"
                "This usually means OpenCL development headers are missing.\n\n"
                "To fix on Linux:\n"
                "  Ubuntu/Debian:\n"
                "    sudo apt install ocl-icd-opencl-dev\n"
                "  Fedora/RHEL:\n"
                "    sudo dnf install ocl-icd-devel\n"
                "  Arch:\n"
                "    sudo pacman -S ocl-icd\n\n"
                "Then reinstall: pip install --force-reinstall chronos-gpu\n\n"
                "For NVIDIA GPUs with concurrent execution, also install:\n"
                "  - CUDA Toolkit (for MPS support)\n"
                "For AMD GPUs:\n"
                "  - ROCm (https://rocm.docs.amd.com)"
            )


@dataclass
class PartitionInfo:
    """Information about a GPU partition."""
    partition_id: str
    device_index: int
    memory_fraction: float
    duration_seconds: int
    time_remaining_seconds: int
    username: str
    process_id: int
    active: bool


class Partition:
    """Stub partition that raises an error when used."""

    def __init__(self, *args, **kwargs):
        raise ChronosStubError("Cannot create Partition - native library not available.")

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False

    def release(self):
        pass

    @property
    def time_remaining(self) -> int:
        return 0


class Partitioner:
    """
    Stub Partitioner that raises helpful errors.

    This stub is used when the native Chronos library cannot be loaded.
    """

    def __init__(self):
        raise ChronosStubError()

    def create(self, device: int, memory: float, duration: int,
               user: Optional[str] = None) -> 'Partition':
        raise ChronosStubError("Cannot create partition - native library not available.")

    def release(self, partition_id: str) -> bool:
        raise ChronosStubError("Cannot release partition - native library not available.")

    def list(self) -> List[PartitionInfo]:
        raise ChronosStubError("Cannot list partitions - native library not available.")

    def get_available(self, device: int) -> float:
        raise ChronosStubError("Cannot get available memory - native library not available.")

    def show_stats(self):
        raise ChronosStubError("Cannot show stats - native library not available.")

    def get_execution_mode(self) -> str:
        """Returns execution mode - always 'stub' for stub implementation."""
        return "stub"

    def get_backend_name(self) -> str:
        """Returns backend name - always 'Stub' for stub implementation."""
        return "Stub (native library not available)"

    @staticmethod
    def check_concurrent_support() -> bool:
        """Check if concurrent execution is supported."""
        return False


def is_stub_mode() -> bool:
    """Returns True - this is the stub module."""
    return True


def check_concurrent_support() -> dict:
    """
    Check what execution modes are available on this system.

    In stub mode, returns information about why native library failed.
    """
    return {
        'nvidia_mps': False,
        'rocm': False,
        'opencl': False,
        'concurrent_possible': False,
        'recommended': None,
        'stub_mode': True,
        'reason': 'Native library not available'
    }


__all__ = [
    'Partitioner',
    'Partition',
    'PartitionInfo',
    'ChronosError',
    'ChronosStubError',
    'is_stub_mode',
    'check_concurrent_support'
]
