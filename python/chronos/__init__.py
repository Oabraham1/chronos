import ctypes
import os
import sys
from typing import Optional, List
from dataclasses import dataclass


def _find_library():
    if sys.platform == "darwin":
        lib_names = ["libchronos.dylib"]
    elif sys.platform == "win32":
        lib_names = ["chronos.dll", "libchronos.dll"]
    else:
        lib_names = ["libchronos.so"]

    search_paths = [
        os.path.join(os.path.dirname(__file__), "..", "..", "build", "lib"),
        os.path.join(os.path.dirname(__file__), "..", "..", "lib"),
        "/usr/local/lib",
        "/usr/lib",
    ]

    for path in search_paths:
        for name in lib_names:
            full_path = os.path.join(path, name)
            if os.path.exists(full_path):
                return full_path

    for name in lib_names:
        try:
            return ctypes.util.find_library(
                name.replace("lib", "").replace(".so", "").replace(".dylib", "").replace(".dll", "")
            )
        except:
            pass

    raise RuntimeError(f"Could not find Chronos library. Searched: {search_paths}")


_lib = ctypes.CDLL(_find_library())


class _ChronosPartitionInfo(ctypes.Structure):
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


_lib.chronos_partitioner_create.restype = ctypes.c_void_p
_lib.chronos_partitioner_create.argtypes = []

_lib.chronos_partitioner_destroy.restype = None
_lib.chronos_partitioner_destroy.argtypes = [ctypes.c_void_p]

_lib.chronos_create_partition.restype = ctypes.c_int
_lib.chronos_create_partition.argtypes = [
    ctypes.c_void_p,
    ctypes.c_int,
    ctypes.c_float,
    ctypes.c_int,
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_size_t,
]

_lib.chronos_release_partition.restype = ctypes.c_int
_lib.chronos_release_partition.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

_lib.chronos_list_partitions.restype = ctypes.c_int
_lib.chronos_list_partitions.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(_ChronosPartitionInfo),
    ctypes.POINTER(ctypes.c_size_t),
]

_lib.chronos_get_available_percentage.restype = ctypes.c_float
_lib.chronos_get_available_percentage.argtypes = [ctypes.c_void_p, ctypes.c_int]

_lib.chronos_show_device_stats.restype = None
_lib.chronos_show_device_stats.argtypes = [ctypes.c_void_p]

_lib.chronos_get_last_error.restype = ctypes.c_char_p
_lib.chronos_get_last_error.argtypes = []


class ChronosError(Exception):
    pass


@dataclass
class PartitionInfo:
    partition_id: str
    device_index: int
    memory_fraction: float
    duration_seconds: int
    time_remaining_seconds: int
    username: str
    process_id: int
    active: bool


class Partition:
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
        if self.partition_id:
            self._partitioner.release(self.partition_id)
            self.partition_id = None

    @property
    def time_remaining(self) -> int:
        partitions = self._partitioner.list()
        for p in partitions:
            if p.partition_id == self.partition_id:
                return p.time_remaining_seconds
        return 0


class Partitioner:
    def __init__(self):
        self._handle = _lib.chronos_partitioner_create()
        if not self._handle:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to create partitioner: {error.decode() if error else 'unknown error'}"
            )

    def __del__(self):
        if hasattr(self, "_handle") and self._handle:
            _lib.chronos_partitioner_destroy(self._handle)

    def create(
        self, device: int, memory: float, duration: int, user: Optional[str] = None
    ) -> Partition:
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

    def release(self, partition_id: str) -> bool:
        result = _lib.chronos_release_partition(self._handle, partition_id.encode())
        if result != 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to release partition: {error.decode() if error else 'unknown error'}"
            )
        return True

    def list(self) -> List[PartitionInfo]:
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
        result = _lib.chronos_get_available_percentage(self._handle, device)
        if result < 0:
            error = _lib.chronos_get_last_error()
            raise ChronosError(
                f"Failed to get available percentage: {error.decode() if error else 'unknown error'}"
            )
        return result

    def show_stats(self):
        _lib.chronos_show_device_stats(self._handle)


__all__ = ["Partitioner", "Partition", "PartitionInfo", "ChronosError"]
