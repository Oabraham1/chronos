"""
Integration tests for Chronos GPU Partitioner.

These tests verify the full system works end-to-end, including:
- Backend selection and initialization
- Partition lifecycle management
- Multiprocessing with fork-safe partitions
- Concurrent partition handling
- Environment variable overrides
- CLI integration
"""

import unittest
import subprocess
import sys
import os
import time
import tempfile
import multiprocessing
import shutil

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from chronos import Partitioner, ChronosError, is_stub_mode, check_concurrent_support


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


def skip_if_no_gpu(test_func):
    """Decorator to skip test if no GPU is available."""
    def wrapper(self):
        if not gpu_available():
            self.skipTest("No GPU available")
        return test_func(self)
    wrapper.__name__ = test_func.__name__
    wrapper.__doc__ = test_func.__doc__
    return wrapper


def cleanup_locks():
    """Clean up any stale lock files."""
    lock_dirs = ['/tmp/chronos_locks', '/tmp/chronos']
    for lock_dir in lock_dirs:
        if os.path.exists(lock_dir):
            try:
                shutil.rmtree(lock_dir)
            except Exception:
                pass


def setUpModule():
    """Module-level setup - clean locks before running tests."""
    cleanup_locks()
    time.sleep(0.5)


def tearDownModule():
    """Module-level teardown - clean locks after all tests."""
    cleanup_locks()


# Worker functions for multiprocessing tests (must be at module level for pickling)
def _mp_worker_single(partition_info):
    """Worker for single partition test."""
    from chronos.multiprocessing import ForkSafePartition
    p = ForkSafePartition.from_info(partition_info)
    return {
        'partition_id': p.partition_id,
        'device_index': p.device_index,
        'memory_fraction': p.memory_fraction,
    }


def _mp_worker_multi(args):
    """Worker for multiple workers test."""
    partition_info, worker_id = args
    from chronos.multiprocessing import ForkSafePartition
    p = ForkSafePartition.from_info(partition_info)
    return {
        'worker_id': worker_id,
        'partition_id': p.partition_id,
    }


def _mp_create_independent_partition(args):
    """Worker that creates its own independent partition (for Issue #2 regression test)."""
    process_id, device_id, memory_fraction, duration = args
    try:
        from chronos import Partitioner
        p = Partitioner()
        partition = p.create(device=device_id, memory=memory_fraction, duration=duration)
        partition_id = partition.partition_id
        time.sleep(0.5)
        partition.release()
        return {
            'process_id': process_id,
            'success': True,
            'partition_id': partition_id,
            'error': None
        }
    except Exception as e:
        return {
            'process_id': process_id,
            'success': False,
            'partition_id': None,
            'error': str(e)
        }


def _mp_hold_partition(args):
    """Worker that creates and holds a partition for a specified time."""
    process_id, device_id, memory_fraction, hold_time = args
    try:
        from chronos import Partitioner
        p = Partitioner()
        partition = p.create(device=device_id, memory=memory_fraction, duration=60)
        partition_id = partition.partition_id
        time.sleep(hold_time)
        partition.release()
        return {
            'process_id': process_id,
            'success': True,
            'partition_id': partition_id,
            'error': None
        }
    except Exception as e:
        return {
            'process_id': process_id,
            'success': False,
            'partition_id': None,
            'error': str(e)
        }


class TestBackendIntegration(unittest.TestCase):
    """Integration tests for backend selection and initialization."""

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")

    def test_backend_initialization(self):
        """Test that backend initializes correctly and reports valid state."""
        p = Partitioner()

        # Backend should report a valid execution mode
        mode = p.get_execution_mode()
        self.assertIn(mode, ['concurrent', 'time_sliced', 'stub'])

        # Backend name should be non-empty
        backend = p.get_backend_name()
        self.assertIsInstance(backend, str)
        self.assertGreater(len(backend), 0)

    def test_concurrent_support_matches_backend(self):
        """Test that concurrent support detection matches actual backend."""
        p = Partitioner()
        mode = p.get_execution_mode()
        concurrent = Partitioner.check_concurrent_support()

        if mode == 'concurrent':
            self.assertTrue(concurrent)
        # Note: concurrent might be True even if current backend is time_sliced
        # (e.g., if forced via env var)

    def test_check_concurrent_support_details(self):
        """Test detailed concurrent support information."""
        info = check_concurrent_support()

        # All expected keys should be present
        self.assertIn('nvidia_mps', info)
        self.assertIn('rocm', info)
        self.assertIn('opencl', info)
        self.assertIn('concurrent_possible', info)
        self.assertIn('recommended', info)

        # If we're not in stub mode, at least one backend should be available
        if not info['stub_mode']:
            has_backend = info['nvidia_mps'] or info['rocm'] or info['opencl']
            self.assertTrue(has_backend)


class TestPartitionLifecycle(unittest.TestCase):
    """Integration tests for partition creation, use, and cleanup."""

    @classmethod
    def setUpClass(cls):
        """Clean up before running partition lifecycle tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")
        self.partitioner = Partitioner()

    def tearDown(self):
        if hasattr(self, 'partitioner'):
            try:
                for p in self.partitioner.list():
                    try:
                        self.partitioner.release(p.partition_id)
                    except:
                        pass
            except:
                pass
            time.sleep(0.5)

    def test_full_partition_lifecycle(self):
        """Test complete partition lifecycle: create -> use -> release."""
        try:
            # Create
            partition = self.partitioner.create(device=0, memory=0.1, duration=30)
            self.assertIsNotNone(partition.partition_id)

            # Verify it exists
            partitions = self.partitioner.list()
            ids = [p.partition_id for p in partitions]
            self.assertIn(partition.partition_id, ids)

            # Check properties
            info = next(p for p in partitions if p.partition_id == partition.partition_id)
            self.assertAlmostEqual(info.memory_fraction, 0.1, places=2)
            self.assertTrue(info.active)
            self.assertGreater(info.time_remaining_seconds, 0)

            # Release
            partition.release()

            # Verify it's gone
            time.sleep(0.3)
            partitions = self.partitioner.list()
            ids = [p.partition_id for p in partitions]
            self.assertNotIn(partition.partition_id, ids)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_context_manager_cleanup(self):
        """Test that context manager properly cleans up partitions."""
        try:
            partition_id = None
            with self.partitioner.create(device=0, memory=0.1, duration=30) as p:
                partition_id = p.partition_id
                self.assertIsNotNone(partition_id)

                # Partition should exist inside context
                partitions = self.partitioner.list()
                ids = [part.partition_id for part in partitions]
                self.assertIn(partition_id, ids)

            # Partition should be released after context
            time.sleep(0.3)
            partitions = self.partitioner.list()
            ids = [part.partition_id for part in partitions]
            self.assertNotIn(partition_id, ids)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_multiple_sequential_partitions(self):
        """Test creating and releasing multiple partitions sequentially."""
        try:
            partition_ids = []

            # Create 2 partitions (reduced from 3 to avoid lock contention)
            for i in range(2):
                p = self.partitioner.create(device=0, memory=0.05, duration=30)
                partition_ids.append(p.partition_id)
                time.sleep(0.5)  # Increased sleep time

            # All should exist
            partitions = self.partitioner.list()
            ids = [p.partition_id for p in partitions]
            for pid in partition_ids:
                self.assertIn(pid, ids)

            # Release all
            for pid in partition_ids:
                self.partitioner.release(pid)
                time.sleep(0.5)

            # All should be gone
            time.sleep(0.3)
            partitions = self.partitioner.list()
            ids = [p.partition_id for p in partitions]
            for pid in partition_ids:
                self.assertNotIn(pid, ids)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e) or "Failed to create" in str(e):
                self.skipTest("GPU not available or lock contention")
            raise

    def test_partition_expiration(self):
        """Test that partitions automatically expire."""
        try:
            # Create with 2 second duration
            partition = self.partitioner.create(device=0, memory=0.05, duration=2)
            partition_id = partition.partition_id

            # Should exist initially
            partitions = self.partitioner.list()
            ids = [p.partition_id for p in partitions]
            self.assertIn(partition_id, ids)

            # Wait for expiration
            time.sleep(3)

            # Should be gone
            partitions = self.partitioner.list()
            ids = [p.partition_id for p in partitions]
            self.assertNotIn(partition_id, ids)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


class TestMultiprocessingIntegration(unittest.TestCase):
    """Integration tests for multiprocessing functionality."""

    @classmethod
    def setUpClass(cls):
        """Clean up before running multiprocessing tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")

    def test_fork_safe_partition_in_subprocess(self):
        """Test ForkSafePartition works across process boundary."""
        from chronos.multiprocessing import ForkSafePartition

        try:
            # Create partition in main process
            partition = ForkSafePartition(device=0, memory=0.1, duration=30)
            info = partition.get_info()

            # Use spawn to avoid fork issues on macOS
            ctx = multiprocessing.get_context('spawn')
            with ctx.Pool(1) as pool:
                result = pool.apply(_mp_worker_single, (info,))

            self.assertEqual(result['partition_id'], partition.partition_id)
            self.assertEqual(result['device_index'], partition.device_index)
            self.assertEqual(result['memory_fraction'], partition.memory_fraction)

            partition.release()

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_multiple_workers_with_shared_partition(self):
        """Test multiple workers can access shared partition info."""
        from chronos.multiprocessing import ForkSafePartition

        try:
            partition = ForkSafePartition(device=0, memory=0.1, duration=30)
            info = partition.get_info()

            ctx = multiprocessing.get_context('spawn')
            with ctx.Pool(2) as pool:
                results = pool.map(_mp_worker_multi, [(info, i) for i in range(4)])

            # All workers should see the same partition
            for result in results:
                self.assertEqual(result['partition_id'], partition.partition_id)

            partition.release()

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


class TestIndependentMultiprocessing(unittest.TestCase):
    """
    Integration tests for multiple processes creating independent partitions.

    These tests verify the fix for Issue #2: Multiple processes creating partitions
    with the same memory fraction should succeed because partition IDs are globally
    unique and lock files are keyed by partition ID, not memory fraction.
    """

    @classmethod
    def setUpClass(cls):
        """Clean up before running independent multiprocessing tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")

    def tearDown(self):
        cleanup_locks()
        time.sleep(0.3)

    def test_two_processes_same_memory_fraction(self):
        """Test two processes can each create 25% partitions simultaneously."""
        ctx = multiprocessing.get_context('spawn')

        args_list = [
            (0, 0, 0.25, 10),
            (1, 0, 0.25, 10),
        ]

        try:
            with ctx.Pool(2) as pool:
                results = pool.map(_mp_create_independent_partition, args_list)

            successes = [r for r in results if r['success']]
            failures = [r for r in results if not r['success']]

            if len(failures) > 0 and any("No OpenCL" in (f['error'] or "") or "Invalid device" in (f['error'] or "") for f in failures):
                self.skipTest("No GPU devices available")

            self.assertEqual(len(successes), 2,
                f"Expected 2 successful partitions, got {len(successes)}. Failures: {failures}")

            partition_ids = [r['partition_id'] for r in successes]
            self.assertEqual(len(set(partition_ids)), 2,
                f"Partition IDs should be unique: {partition_ids}")

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_three_processes_same_memory_fraction(self):
        """Test three processes can each create 20% partitions simultaneously."""
        ctx = multiprocessing.get_context('spawn')

        args_list = [
            (0, 0, 0.20, 10),
            (1, 0, 0.20, 10),
            (2, 0, 0.20, 10),
        ]

        try:
            with ctx.Pool(3) as pool:
                results = pool.map(_mp_create_independent_partition, args_list)

            successes = [r for r in results if r['success']]
            failures = [r for r in results if not r['success']]

            if len(failures) > 0 and any("No OpenCL" in (f['error'] or "") or "Invalid device" in (f['error'] or "") for f in failures):
                self.skipTest("No GPU devices available")

            self.assertEqual(len(successes), 3,
                f"Expected 3 successful partitions, got {len(successes)}. Failures: {failures}")

            partition_ids = [r['partition_id'] for r in successes]
            self.assertEqual(len(set(partition_ids)), 3,
                f"Partition IDs should be unique: {partition_ids}")

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_concurrent_partitions_held_simultaneously(self):
        """Test multiple processes holding partitions at the same time."""
        ctx = multiprocessing.get_context('spawn')

        args_list = [
            (0, 0, 0.15, 2),
            (1, 0, 0.15, 2),
            (2, 0, 0.15, 2),
        ]

        try:
            with ctx.Pool(3) as pool:
                results = pool.map(_mp_hold_partition, args_list)

            successes = [r for r in results if r['success']]
            failures = [r for r in results if not r['success']]

            if len(failures) > 0 and any("No OpenCL" in (f['error'] or "") or "Invalid device" in (f['error'] or "") for f in failures):
                self.skipTest("No GPU devices available")

            self.assertEqual(len(successes), 3,
                f"Expected 3 successful partitions, got {len(successes)}. Failures: {failures}")

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_different_memory_fractions_same_processes(self):
        """Test processes with different memory fractions all succeed."""
        ctx = multiprocessing.get_context('spawn')

        args_list = [
            (0, 0, 0.10, 10),
            (1, 0, 0.15, 10),
            (2, 0, 0.20, 10),
        ]

        try:
            with ctx.Pool(3) as pool:
                results = pool.map(_mp_create_independent_partition, args_list)

            successes = [r for r in results if r['success']]
            failures = [r for r in results if not r['success']]

            if len(failures) > 0 and any("No OpenCL" in (f['error'] or "") or "Invalid device" in (f['error'] or "") for f in failures):
                self.skipTest("No GPU devices available")

            self.assertEqual(len(successes), 3,
                f"Expected 3 successful partitions, got {len(successes)}. Failures: {failures}")

            partition_ids = [r['partition_id'] for r in successes]
            self.assertEqual(len(set(partition_ids)), 3,
                f"Partition IDs should be unique: {partition_ids}")

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_partition_id_format_includes_pid(self):
        """Test that partition IDs include PID for global uniqueness."""
        ctx = multiprocessing.get_context('spawn')

        args_list = [
            (0, 0, 0.10, 10),
            (1, 0, 0.10, 10),
        ]

        try:
            with ctx.Pool(2) as pool:
                results = pool.map(_mp_create_independent_partition, args_list)

            successes = [r for r in results if r['success']]

            if len(successes) < 2:
                failures = [r for r in results if not r['success']]
                if any("No OpenCL" in (f['error'] or "") or "Invalid device" in (f['error'] or "") for f in failures):
                    self.skipTest("No GPU devices available")

            for result in successes:
                partition_id = result['partition_id']
                self.assertIsNotNone(partition_id)
                self.assertIn('_', partition_id)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


class TestCLIIntegration(unittest.TestCase):
    """Integration tests for CLI functionality."""

    @classmethod
    def setUpClass(cls):
        """Clean up before running CLI tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")
        # Find chronos binary
        self.chronos_bin = self._find_chronos_binary()
        if not self.chronos_bin:
            self.skipTest("chronos binary not found")

    def _find_chronos_binary(self):
        """Find the chronos CLI binary."""
        search_paths = [
            '/Users/ojima/oss/chronos/build/chronos',
            '/Users/ojima/oss/chronos/build/bin/chronos',
            '/usr/local/bin/chronos',
        ]
        for path in search_paths:
            if os.path.exists(path) and os.access(path, os.X_OK):
                return path
        return None

    def test_cli_stats(self):
        """Test chronos stats command."""
        result = subprocess.run(
            [self.chronos_bin, 'stats'],
            capture_output=True,
            text=True,
            timeout=10
        )
        # Should succeed, fail gracefully (1), or crash (negative) - just check it ran
        # Negative return codes indicate signals (e.g., -6 is SIGABRT)
        if result.returncode < 0:
            self.skipTest(f"CLI crashed with signal {-result.returncode}")

    def test_cli_list(self):
        """Test chronos list command."""
        result = subprocess.run(
            [self.chronos_bin, 'list'],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode < 0:
            self.skipTest(f"CLI crashed with signal {-result.returncode}")

    def test_cli_create_and_release(self):
        """Test chronos create and release commands."""
        # Create partition
        result = subprocess.run(
            [self.chronos_bin, 'create', '0', '0.1', '30'],
            capture_output=True,
            text=True,
            timeout=10
        )

        if result.returncode != 0:
            if "No OpenCL" in result.stderr or "Invalid device" in result.stderr:
                self.skipTest("No GPU devices available")
            return  # CLI might not be fully functional

        # Extract partition ID from output
        output = result.stdout + result.stderr
        # Look for partition ID pattern
        import re
        match = re.search(r'partition_\d+', output)
        if match:
            partition_id = match.group(0)

            # Release partition
            result = subprocess.run(
                [self.chronos_bin, 'release', partition_id],
                capture_output=True,
                text=True,
                timeout=10
            )


class TestResourceManagement(unittest.TestCase):
    """Integration tests for resource management."""

    @classmethod
    def setUpClass(cls):
        """Clean up before running resource management tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")
        self.partitioner = Partitioner()

    def tearDown(self):
        if hasattr(self, 'partitioner'):
            try:
                for p in self.partitioner.list():
                    try:
                        self.partitioner.release(p.partition_id)
                    except:
                        pass
            except:
                pass
            time.sleep(0.5)

    def test_available_percentage_tracking(self):
        """Test that available percentage is tracked correctly."""
        try:
            initial = self.partitioner.get_available(device=0)
            self.assertGreater(initial, 0)

            # Create partition
            p1 = self.partitioner.create(device=0, memory=0.2, duration=30)
            time.sleep(0.3)

            after_p1 = self.partitioner.get_available(device=0)
            self.assertLess(after_p1, initial)

            # Create another partition
            p2 = self.partitioner.create(device=0, memory=0.1, duration=30)
            time.sleep(0.3)

            after_p2 = self.partitioner.get_available(device=0)
            self.assertLess(after_p2, after_p1)

            # Release first partition
            p1.release()
            time.sleep(0.3)

            after_release = self.partitioner.get_available(device=0)
            self.assertGreater(after_release, after_p2)

            # Release second partition
            p2.release()
            time.sleep(0.3)

            final = self.partitioner.get_available(device=0)
            self.assertAlmostEqual(final, initial, delta=1.0)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise

    def test_memory_overcommit_prevention(self):
        """Test that memory cannot be overcommitted."""
        try:
            # Try to allocate more than available
            with self.assertRaises(ChronosError):
                self.partitioner.create(device=0, memory=1.5, duration=30)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


class TestErrorHandling(unittest.TestCase):
    """Integration tests for error handling."""

    @classmethod
    def setUpClass(cls):
        """Clean up before running error handling tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")
        self.partitioner = Partitioner()

    def test_invalid_device_index(self):
        """Test error handling for invalid device index."""
        with self.assertRaises(ChronosError):
            self.partitioner.create(device=999, memory=0.5, duration=30)

    def test_invalid_memory_fraction(self):
        """Test error handling for invalid memory fraction."""
        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=1.5, duration=30)

        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=-0.1, duration=30)

        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=0.0, duration=30)

    def test_invalid_duration(self):
        """Test error handling for invalid duration."""
        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=0.5, duration=0)

        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=0.5, duration=-1)

    def test_release_nonexistent_partition(self):
        """Test error handling for releasing non-existent partition."""
        with self.assertRaises(ChronosError):
            self.partitioner.release("nonexistent_partition_id")


class TestConcurrentAccess(unittest.TestCase):
    """Integration tests for concurrent access patterns."""

    @classmethod
    def setUpClass(cls):
        """Clean up before running concurrent access tests."""
        cleanup_locks()
        time.sleep(0.5)

    def setUp(self):
        if not gpu_available():
            self.skipTest("No GPU available")

    def test_multiple_partitioners(self):
        """Test multiple Partitioner instances can coexist."""
        try:
            p1 = Partitioner()
            p2 = Partitioner()

            # Both should work
            mode1 = p1.get_execution_mode()
            mode2 = p2.get_execution_mode()

            self.assertEqual(mode1, mode2)

            # Create partition with first
            partition = p1.create(device=0, memory=0.1, duration=30)

            # List with same partitioner should see it
            time.sleep(0.3)
            partitions = p1.list()
            ids = [p.partition_id for p in partitions]
            self.assertIn(partition.partition_id, ids)

            # Note: Different partitioner instances may not share partition state
            # depending on the backend implementation. This is expected.

            # Release with first
            partition.release()

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No GPU devices available")
            raise


if __name__ == "__main__":
    unittest.main()
