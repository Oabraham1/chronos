"""
Fork-safe multiprocessing utilities for Chronos.

This module provides ForkSafePartition, a partition wrapper that can be safely
used with multiprocessing.Pool and other fork-based parallelism.

Example:
    from chronos.multiprocessing import ForkSafePartition
    import multiprocessing

    def worker(partition_info):
        with ForkSafePartition.from_info(partition_info) as p:
            # Use GPU here
            return do_work()

    # Create partition in main process
    partition = ForkSafePartition(device=0, memory=0.5, duration=3600)
    info = partition.get_info()

    # Pass info to workers (pickle-safe)
    with multiprocessing.Pool(4) as pool:
        results = pool.map(worker, [info] * 4)

    partition.release()
"""

import os
import pickle
from dataclasses import dataclass
from typing import Optional

from . import Partitioner, _STUB_MODE


@dataclass
class PartitionInfo:
    """Pickle-safe partition information for passing to worker processes."""

    partition_id: str
    device_index: int
    memory_fraction: float
    duration_seconds: int
    username: str
    pid: int

    def __getstate__(self):
        return self.__dict__

    def __setstate__(self, state):
        self.__dict__.update(state)


class ForkSafePartition:
    """
    A fork-safe wrapper around Chronos partitions.

    Unlike the regular Partition object, ForkSafePartition can be safely used
    with multiprocessing.Pool because it doesn't hold native handles that
    become invalid after fork().

    The partition is created in the main process and workers reconnect using
    the partition ID stored in PartitionInfo.

    Usage:
        # Main process
        partition = ForkSafePartition(device=0, memory=0.5, duration=3600)
        info = partition.get_info()

        # Worker process
        with ForkSafePartition.from_info(info) as p:
            # GPU work here
            pass

        # Main process cleanup
        partition.release()
    """

    def __init__(
        self,
        device: int = 0,
        memory: float = 0.5,
        duration: int = 3600,
        user: str = "",
        _info: Optional[PartitionInfo] = None,
    ):
        """
        Create a new fork-safe partition.

        Args:
            device: GPU device index
            memory: Memory fraction (0.0 to 1.0)
            duration: Duration in seconds
            user: Target username (optional)
            _info: Internal use only - for reconnecting in worker processes
        """
        self._partitioner: Optional[Partitioner] = None
        self._info: Optional[PartitionInfo] = None
        self._is_owner = False
        self._released = False

        if _info is not None:
            # Reconnecting in worker process
            self._info = _info
            self._is_owner = False
        else:
            # Creating new partition in main process
            self._partitioner = Partitioner()
            partition_id = self._partitioner.create_raw(device, memory, duration, user)

            if not partition_id:
                raise RuntimeError("Failed to create partition")

            self._info = PartitionInfo(
                partition_id=partition_id,
                device_index=device,
                memory_fraction=memory,
                duration_seconds=duration,
                username=user or os.environ.get("USER", "unknown"),
                pid=os.getpid(),
            )
            self._is_owner = True

    @classmethod
    def from_info(cls, info: PartitionInfo) -> "ForkSafePartition":
        """
        Reconnect to an existing partition in a worker process.

        Args:
            info: PartitionInfo obtained from get_info() in the main process

        Returns:
            ForkSafePartition connected to the existing partition
        """
        return cls(_info=info)

    def get_info(self) -> PartitionInfo:
        """
        Get pickle-safe partition info for passing to worker processes.

        Returns:
            PartitionInfo that can be pickled and passed to workers
        """
        if self._info is None:
            raise RuntimeError("Partition not initialized")
        return self._info

    @property
    def partition_id(self) -> str:
        """Get the partition ID."""
        if self._info is None:
            raise RuntimeError("Partition not initialized")
        return self._info.partition_id

    @property
    def device_index(self) -> int:
        """Get the device index."""
        if self._info is None:
            raise RuntimeError("Partition not initialized")
        return self._info.device_index

    @property
    def memory_fraction(self) -> float:
        """Get the memory fraction."""
        if self._info is None:
            raise RuntimeError("Partition not initialized")
        return self._info.memory_fraction

    def release(self) -> bool:
        """
        Release the partition.

        Only the owner process (the one that created the partition) should
        release it. Worker processes should not call release().

        Returns:
            True if released successfully, False otherwise
        """
        if self._released:
            return True

        if not self._is_owner:
            # Workers don't release - they just disconnect
            self._released = True
            return True

        if self._partitioner is None:
            self._partitioner = Partitioner()

        if self._info is None:
            return False

        result = self._partitioner.release(self._info.partition_id)
        self._released = True
        return result

    def __enter__(self) -> "ForkSafePartition":
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        # Only owner releases on context exit
        if self._is_owner:
            self.release()

    def __del__(self):
        # Clean up if not already released (owner only)
        if self._is_owner and not self._released:
            try:
                self.release()
            except Exception:
                pass

    def __reduce__(self):
        """
        Make the partition pickle-safe by only pickling the info.
        When unpickled, it will reconnect using from_info().
        """
        return (self.__class__.from_info, (self._info,))


def create_shared_partition(
    device: int = 0, memory: float = 0.5, duration: int = 3600, user: str = ""
) -> ForkSafePartition:
    """
    Convenience function to create a fork-safe partition.

    Args:
        device: GPU device index
        memory: Memory fraction (0.0 to 1.0)
        duration: Duration in seconds
        user: Target username (optional)

    Returns:
        ForkSafePartition that can be used with multiprocessing
    """
    return ForkSafePartition(device=device, memory=memory, duration=duration, user=user)
