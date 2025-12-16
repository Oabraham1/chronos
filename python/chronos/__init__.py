"""
Chronos GPU Partitioner

Time-based GPU partitioning for fair resource sharing.

Execution Modes:
    - concurrent: True parallel execution (NVIDIA MPS)
    - time_sliced: Context switching between partitions (OpenCL)
    - stub: Native library not available

Example:
    from chronos import Partitioner

    p = Partitioner()
    print(f"Backend: {p.get_backend_name()}")
    print(f"Mode: {p.get_execution_mode()}")

    with p.create(device=0, memory=0.5, duration=3600) as partition:
        # Your GPU code here
        train_model()
"""

import ctypes
import ctypes.util
import os
import sys
import warnings
from typing import Optional, List
from dataclasses import dataclass

__version__ = "1.1.0"

# Flag to track if we're in stub mode
_STUB_MODE = False
_STUB_REASON = None
_lib = None


def _find_library():
    """Find the Chronos native library."""
    if sys.platform == "darwin":
        lib_names = ["libchronos.dylib"]
    elif sys.platform == "win32":
        lib_names = ["chronos.dll", "libchronos.dll"]
    else:
        lib_names = ["libchronos.so"]

    # Search paths in order of preference
    search_paths = [
        # Installed alongside the package
        os.path.dirname(__file__),
        # Development build directory
        os.path.join(os.path.dirname(__file__), "..", "..", "build", "lib"),
        os.path.join(os.path.dirname(__file__), "..", "..", "lib"),
        # System paths
        "/usr/local/lib",
        "/usr/lib",
    ]

    for path in search_paths:
        for name in lib_names:
            full_path = os.path.join(path, name)
            if os.path.exists(full_path):
                return full_path

    # Try ctypes find_library as last resort
    for name in lib_names:
        try:
            lib_base = name.replace("lib", "").replace(".so", "").replace(".dylib", "").replace(".dll", "")
            found = ctypes.util.find_library(lib_base)
            if found:
                return found
        except Exception:
            pass

    return None


def _load_native_library():
    """Attempt to load the native library."""
    global _STUB_MODE, _STUB_REASON, _lib

    lib_path = _find_library()
    if lib_path is None:
        _STUB_MODE = True
        _STUB_REASON = "Native library not found"
        return None

    try:
        lib = ctypes.CDLL(lib_path)

        # Set up function signatures
        lib.chronos_partitioner_create.restype = ctypes.c_void_p
        lib.chronos_partitioner_create.argtypes = []

        lib.chronos_partitioner_destroy.restype = None
        lib.chronos_partitioner_destroy.argtypes = [ctypes.c_void_p]

        lib.chronos_create_partition.restype = ctypes.c_int
        lib.chronos_create_partition.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_float,
            ctypes.c_int,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        lib.chronos_release_partition.restype = ctypes.c_int
        lib.chronos_release_partition.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        lib.chronos_list_partitions.restype = ctypes.c_int
        lib.chronos_list_partitions.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(_ChronosPartitionInfo),
            ctypes.POINTER(ctypes.c_size_t),
        ]

        lib.chronos_get_available_percentage.restype = ctypes.c_float
        lib.chronos_get_available_percentage.argtypes = [ctypes.c_void_p, ctypes.c_int]

        lib.chronos_show_device_stats.restype = None
        lib.chronos_show_device_stats.argtypes = [ctypes.c_void_p]

        lib.chronos_get_last_error.restype = ctypes.c_char_p
        lib.chronos_get_last_error.argtypes = []

        # Check for new backend API functions (may not exist in older builds)
        try:
            lib.chronos_get_execution_mode.restype = ctypes.c_int
            lib.chronos_get_execution_mode.argtypes = [ctypes.c_void_p]

            lib.chronos_get_backend_name.restype = ctypes.c_char_p
            lib.chronos_get_backend_name.argtypes = [ctypes.c_void_p]

            lib.chronos_check_concurrent_support.restype = ctypes.c_int
            lib.chronos_check_concurrent_support.argtypes = []
        except AttributeError:
            # Older library version without backend API
            pass

        return lib

    except OSError as e:
        _STUB_MODE = True
        _STUB_REASON = f"Failed to load native library: {e}"
        return None
    except Exception as e:
        _STUB_MODE = True
        _STUB_REASON = f"Error initializing native library: {e}"
        return None


class _ChronosPartitionInfo(ctypes.Structure):
    """C structure for partition information."""
    _fields_ = [
        ("partition_id", ctypes.c_char * 64),
        ("device_index", ctypes.c_int),
        ("memory_fraction", ctypes.c_float),
        ("duration_seconds", ctypes.c_int),
        ("time_remaining_seconds", ctypes.c_int),
        ("username", ctypes.c_char * 256),
        ("process_id", ctypes.c_int),
        ("active", ctypes.c_int),
    ]


# Try to load native library
_lib = _load_native_library()


class ChronosError(Exception):
    """Exception raised for Chronos-related errors."""
    pass


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
    """
    A GPU partition handle.

    Use as a context manager for automatic cleanup:

        with partitioner.create(device=0, memory=0.5, duration=3600) as p:
            # GPU work here
            pass

    Or manually manage the lifecycle:

        p = partitioner.create(device=0, memory=0.5, duration=3600)
        try:
            # GPU work here
            pass
        finally:
            p.release()
    """

    def __init__(self, partitioner, partition_id: str, device: int, memory: float, duration: int):
        self._partitioner = partitioner
        self.partition_id = partition_id
        self.device = device
        self.memory_fraction = memory
        self.duration = duration

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.release()
        return False

    def release(self):
        """Release this partition."""
        if self.partition_id:
            self._partitioner.release(self.partition_id)
            self.partition_id = None

    @property
    def time_remaining(self) -> int:
        """Get remaining time in seconds."""
        partitions = self._partitioner.list()
        for p in partitions:
            if p.partition_id == self.partition_id:
                return p.time_remaining_seconds
        return 0


class Partitioner:
    """
    GPU Partition Manager.

    Creates time-bounded GPU partitions with memory limits.

    Execution Model:
        - NVIDIA GPUs with MPS: True concurrent execution
        - AMD GPUs with ROCm: GPU-level partitioning
        - Other GPUs (OpenCL): Time-sliced execution

    Use `get_execution_mode()` to check which mode is active.

    Example:
        p = Partitioner()
        print(p.get_execution_mode())  # "concurrent" or "time_sliced"

        with p.create(device=0, memory=0.5, duration=3600) as partition:
            # Your GPU code here
            train_model()
    """

    def __init__(self):
        if _STUB_MODE:
            # Import and raise stub error
            from .stub import ChronosStubError
            raise ChronosStubError(_STUB_REASON)

        self._handle = _lib.chronos_partitioner_create()
        if not self._handle:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to create partitioner: {error.decode() if error else 'unknown error'}"
            )

    def __del__(self):
        if hasattr(self, "_handle") and self._handle and _lib:
            _lib.chronos_partitioner_destroy(self._handle)

    def create(
        self, device: int, memory: float, duration: int, user: Optional[str] = None
    ) -> Partition:
        """
        Create a GPU partition.

        Args:
            device: GPU device index (0, 1, ...)
            memory: Fraction of GPU memory (0.0 to 1.0)
            duration: Duration in seconds
            user: Target user (requires admin privileges)

        Returns:
            Partition context manager

        Raises:
            ChronosError: If partition creation fails
        """
        partition_id = ctypes.create_string_buffer(64)
        user_bytes = user.encode() if user else None

        result = _lib.chronos_create_partition(
            self._handle,
            device,
            memory,
            duration,
            user_bytes,
            partition_id,
            len(partition_id),
        )

        if result != 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to create partition: {error.decode() if error else 'unknown error'}"
            )

        return Partition(self, partition_id.value.decode(), device, memory, duration)

    def create_raw(
        self, device: int, memory: float, duration: int, user: Optional[str] = None
    ) -> str:
        """
        Create a GPU partition and return just the partition ID.

        This is useful for fork-safe multiprocessing where you need to pass
        the partition ID to worker processes. Use the multiprocessing module
        for a higher-level API.

        Args:
            device: GPU device index (0, 1, ...)
            memory: Fraction of GPU memory (0.0 to 1.0)
            duration: Duration in seconds
            user: Target user (requires admin privileges)

        Returns:
            Partition ID string

        Raises:
            ChronosError: If partition creation fails
        """
        partition_id = ctypes.create_string_buffer(64)
        user_bytes = user.encode() if user else None

        result = _lib.chronos_create_partition(
            self._handle,
            device,
            memory,
            duration,
            user_bytes,
            partition_id,
            len(partition_id),
        )

        if result != 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to create partition: {error.decode() if error else 'unknown error'}"
            )

        return partition_id.value.decode()

    def release(self, partition_id: str) -> bool:
        """
        Release a partition by ID.

        Args:
            partition_id: The partition ID to release

        Returns:
            True if successful

        Raises:
            ChronosError: If release fails
        """
        result = _lib.chronos_release_partition(self._handle, partition_id.encode())
        if result != 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to release partition: {error.decode() if error else 'unknown error'}"
            )
        return True

    def list(self) -> List[PartitionInfo]:
        """
        List all active partitions.

        Returns:
            List of PartitionInfo objects
        """
        count = ctypes.c_size_t(0)
        _lib.chronos_list_partitions(self._handle, None, ctypes.byref(count))

        if count.value == 0:
            return []

        partitions = (_ChronosPartitionInfo * count.value)()
        result = _lib.chronos_list_partitions(self._handle, partitions, ctypes.byref(count))

        if result != 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to list partitions: {error.decode() if error else 'unknown error'}"
            )

        return [
            PartitionInfo(
                partition_id=p.partition_id.decode(),
                device_index=p.device_index,
                memory_fraction=p.memory_fraction,
                duration_seconds=p.duration_seconds,
                time_remaining_seconds=p.time_remaining_seconds,
                username=p.username.decode(),
                process_id=p.process_id,
                active=bool(p.active),
            )
            for p in partitions[: count.value]
        ]

    def get_available(self, device: int) -> float:
        """
        Get available memory fraction for a device.

        Args:
            device: GPU device index

        Returns:
            Available memory as fraction (0.0 to 1.0)
        """
        result = _lib.chronos_get_available_percentage(self._handle, device)
        if result < 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to get available percentage: {error.decode() if error else 'unknown error'}"
            )
        return result

    def show_stats(self):
        """Display device statistics to stdout."""
        _lib.chronos_show_device_stats(self._handle)

    def get_execution_mode(self) -> str:
        """
        Get the current execution mode.

        Returns:
            "concurrent" - Multiple partitions run in parallel (MPS/ROCm)
            "time_sliced" - Multiple partitions share GPU via time-slicing
            "stub" - Native library not available
        """
        try:
            mode = _lib.chronos_get_execution_mode(self._handle)
            return ["concurrent", "time_sliced", "stub"][mode]
        except AttributeError:
            # Older library version - assume time_sliced (OpenCL only)
            return "time_sliced"

    def get_backend_name(self) -> str:
        """
        Get the name of the active backend.

        Returns:
            Backend name (e.g., 'NVIDIA MPS', 'ROCm', 'OpenCL')
        """
        try:
            name = _lib.chronos_get_backend_name(self._handle)
            return name.decode() if name else "OpenCL"
        except AttributeError:
            # Older library version - assume OpenCL
            return "OpenCL"

    @staticmethod
    def check_concurrent_support() -> bool:
        """
        Check if concurrent execution is supported on this system.

        Returns:
            True if NVIDIA MPS or ROCm is available
        """
        if _STUB_MODE:
            return False
        try:
            return bool(_lib.chronos_check_concurrent_support())
        except AttributeError:
            # Older library version
            return False


def is_stub_mode() -> bool:
    """
    Check if Chronos is running in stub mode.

    Returns:
        True if native library is not available
    """
    return _STUB_MODE


def check_concurrent_support() -> dict:
    """
    Check what execution modes are available on this system.

    Returns:
        Dict with:
            - 'nvidia_mps': bool - NVIDIA MPS available
            - 'rocm': bool - AMD ROCm available
            - 'opencl': bool - OpenCL available
            - 'recommended': str - Best backend to use
            - 'concurrent_possible': bool - True concurrent execution available
            - 'stub_mode': bool - Running in stub mode
    """
    import subprocess
    import shutil

    info = {
        'nvidia_mps': False,
        'rocm': False,
        'opencl': False,
        'concurrent_possible': False,
        'recommended': None,
        'stub_mode': _STUB_MODE,
    }

    if _STUB_MODE:
        info['reason'] = _STUB_REASON
        return info

    # Check NVIDIA MPS (look for nvidia-smi)
    if shutil.which('nvidia-smi'):
        try:
            result = subprocess.run(
                ['nvidia-smi', '--query-gpu=name', '--format=csv,noheader'],
                capture_output=True, text=True, timeout=5
            )
            if result.returncode == 0 and result.stdout.strip():
                info['nvidia_mps'] = True
                info['concurrent_possible'] = True
                info['recommended'] = 'nvidia_mps'
        except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
            pass

    # Check ROCm (look for rocm-smi)
    if shutil.which('rocm-smi'):
        try:
            result = subprocess.run(
                ['rocm-smi', '--showid'],
                capture_output=True, timeout=5
            )
            if result.returncode == 0:
                info['rocm'] = True
                if not info['recommended']:
                    info['recommended'] = 'rocm'
        except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
            pass

    # OpenCL is available if we're not in stub mode
    info['opencl'] = not _STUB_MODE
    if not info['recommended'] and info['opencl']:
        info['recommended'] = 'opencl'

    return info


# Import multiprocessing utilities
try:
    from .multiprocessing import ForkSafePartition, PartitionInfo as MPPartitionInfo, create_shared_partition
    _HAS_MULTIPROCESSING = True
except ImportError:
    _HAS_MULTIPROCESSING = False

# Import from stub if needed (for consistent API)
if _STUB_MODE:
    from .stub import ChronosStubError
    __all__ = [
        'Partitioner',
        'Partition',
        'PartitionInfo',
        'ChronosError',
        'ChronosStubError',
        'is_stub_mode',
        'check_concurrent_support',
        '__version__',
    ]
else:
    __all__ = [
        'Partitioner',
        'Partition',
        'PartitionInfo',
        'ChronosError',
        'is_stub_mode',
        'check_concurrent_support',
        '__version__',
    ]

# Add multiprocessing exports if available
if _HAS_MULTIPROCESSING:
    __all__.extend(['ForkSafePartition', 'create_shared_partition'])
