"""
Tests for fork-safe multiprocessing functionality.
"""

import unittest
import sys
import os
import pickle
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from chronos import Partitioner, ChronosError, is_stub_mode


def gpu_available():
    """Check if GPU is available for testing."""
    if is_stub_mode():
        return False
    try:
        p = Partitioner()
        # Actually try to create a partition to verify GPU works
        partition = p.create(device=0, memory=0.01, duration=5)
        partition.release()
        return True
    except ChronosError:
        return False


class TestPartitionInfo(unittest.TestCase):
    """Test PartitionInfo dataclass."""

    def test_import_partition_info(self):
        """Test that PartitionInfo can be imported from multiprocessing module."""
        from chronos.multiprocessing import PartitionInfo
        self.assertIsNotNone(PartitionInfo)

    def test_partition_info_creation(self):
        """Test that PartitionInfo can be created."""
        from chronos.multiprocessing import PartitionInfo

        info = PartitionInfo(
            partition_id="test-123",
            device_index=0,
            memory_fraction=0.5,
            duration_seconds=3600,
            username="testuser",
            pid=12345,
        )

        self.assertEqual(info.partition_id, "test-123")
        self.assertEqual(info.device_index, 0)
        self.assertEqual(info.memory_fraction, 0.5)
        self.assertEqual(info.duration_seconds, 3600)
        self.assertEqual(info.username, "testuser")
        self.assertEqual(info.pid, 12345)

    def test_partition_info_pickle(self):
        """Test that PartitionInfo can be pickled."""
        from chronos.multiprocessing import PartitionInfo

        info = PartitionInfo(
            partition_id="test-456",
            device_index=1,
            memory_fraction=0.25,
            duration_seconds=1800,
            username="pickletest",
            pid=99999,
        )

        # Pickle and unpickle
        pickled = pickle.dumps(info)
        unpickled = pickle.loads(pickled)

        self.assertEqual(unpickled.partition_id, info.partition_id)
        self.assertEqual(unpickled.device_index, info.device_index)
        self.assertEqual(unpickled.memory_fraction, info.memory_fraction)
        self.assertEqual(unpickled.duration_seconds, info.duration_seconds)
        self.assertEqual(unpickled.username, info.username)
        self.assertEqual(unpickled.pid, info.pid)


class TestForkSafePartition(unittest.TestCase):
    """Test ForkSafePartition class."""

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")

    def test_import_fork_safe_partition(self):
        """Test that ForkSafePartition can be imported."""
        from chronos.multiprocessing import ForkSafePartition
        self.assertIsNotNone(ForkSafePartition)

    def test_create_fork_safe_partition(self):
        """Test that ForkSafePartition can be created."""
        from chronos.multiprocessing import ForkSafePartition

        try:
            partition = ForkSafePartition(device=0, memory=0.1, duration=5)
            self.assertIsNotNone(partition.partition_id)
            self.assertEqual(partition.device_index, 0)
            self.assertEqual(partition.memory_fraction, 0.1)
            partition.release()
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_get_info(self):
        """Test that get_info returns PartitionInfo."""
        from chronos.multiprocessing import ForkSafePartition, PartitionInfo

        try:
            partition = ForkSafePartition(device=0, memory=0.1, duration=5)
            info = partition.get_info()

            self.assertIsInstance(info, PartitionInfo)
            self.assertEqual(info.partition_id, partition.partition_id)
            self.assertEqual(info.device_index, 0)
            self.assertEqual(info.memory_fraction, 0.1)
            self.assertEqual(info.duration_seconds, 5)

            partition.release()
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_from_info(self):
        """Test that from_info creates partition from info."""
        from chronos.multiprocessing import ForkSafePartition

        try:
            # Create original partition
            original = ForkSafePartition(device=0, memory=0.1, duration=5)
            info = original.get_info()

            # Create from info (simulating worker process)
            reconnected = ForkSafePartition.from_info(info)

            self.assertEqual(reconnected.partition_id, original.partition_id)
            self.assertEqual(reconnected.device_index, original.device_index)
            self.assertEqual(reconnected.memory_fraction, original.memory_fraction)

            # Original should release, not reconnected
            original.release()
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_context_manager(self):
        """Test ForkSafePartition as context manager."""
        from chronos.multiprocessing import ForkSafePartition

        try:
            with ForkSafePartition(device=0, memory=0.1, duration=5) as partition:
                self.assertIsNotNone(partition.partition_id)

            # Partition should be released after context
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_pickle_fork_safe_partition(self):
        """Test that ForkSafePartition can be pickled."""
        from chronos.multiprocessing import ForkSafePartition

        try:
            partition = ForkSafePartition(device=0, memory=0.1, duration=5)

            # Pickle and unpickle
            pickled = pickle.dumps(partition)
            unpickled = pickle.loads(pickled)

            # Unpickled should have same partition info
            self.assertEqual(unpickled.partition_id, partition.partition_id)
            self.assertEqual(unpickled.device_index, partition.device_index)
            self.assertEqual(unpickled.memory_fraction, partition.memory_fraction)

            # Original should release
            partition.release()
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


class TestCreateSharedPartition(unittest.TestCase):
    """Test create_shared_partition convenience function."""

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")

    def test_import_create_shared_partition(self):
        """Test that create_shared_partition can be imported."""
        from chronos.multiprocessing import create_shared_partition
        self.assertTrue(callable(create_shared_partition))

    def test_create_shared_partition(self):
        """Test that create_shared_partition works."""
        from chronos.multiprocessing import create_shared_partition, ForkSafePartition

        try:
            partition = create_shared_partition(device=0, memory=0.1, duration=5)
            self.assertIsInstance(partition, ForkSafePartition)
            self.assertIsNotNone(partition.partition_id)
            partition.release()
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


class TestCreateRaw(unittest.TestCase):
    """Test create_raw method on Partitioner."""

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")
        try:
            self.partitioner = Partitioner()
        except ChronosError:
            self.skipTest("No GPU available")

    def tearDown(self):
        if hasattr(self, 'partitioner'):
            try:
                partitions = self.partitioner.list()
                for p in partitions:
                    try:
                        self.partitioner.release(p.partition_id)
                    except:
                        pass
            except:
                pass
            time.sleep(0.5)

    def test_create_raw_returns_string(self):
        """Test that create_raw returns a partition ID string."""
        try:
            partition_id = self.partitioner.create_raw(device=0, memory=0.1, duration=5)
            self.assertIsInstance(partition_id, str)
            self.assertGreater(len(partition_id), 0)
            self.partitioner.release(partition_id)
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_create_raw_creates_valid_partition(self):
        """Test that create_raw creates a valid partition."""
        try:
            partition_id = self.partitioner.create_raw(device=0, memory=0.1, duration=5)

            # Partition should appear in list
            partitions = self.partitioner.list()
            partition_ids = [p.partition_id for p in partitions]
            self.assertIn(partition_id, partition_ids)

            self.partitioner.release(partition_id)
        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


if __name__ == "__main__":
    unittest.main()
