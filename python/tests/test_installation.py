"""
Tests for installation and stub mode behavior.

These tests verify that the package handles various installation scenarios
correctly, including when the native library is not available.
"""

import unittest
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


class TestImports(unittest.TestCase):
    """Test that all expected imports work."""

    def test_import_main_module(self):
        """Test that chronos module can be imported."""
        import chronos
        self.assertIsNotNone(chronos)

    def test_import_partitioner(self):
        """Test that Partitioner can be imported."""
        from chronos import Partitioner
        self.assertIsNotNone(Partitioner)

    def test_import_partition(self):
        """Test that Partition can be imported."""
        from chronos import Partition
        self.assertIsNotNone(Partition)

    def test_import_partition_info(self):
        """Test that PartitionInfo can be imported."""
        from chronos import PartitionInfo
        self.assertIsNotNone(PartitionInfo)

    def test_import_chronos_error(self):
        """Test that ChronosError can be imported."""
        from chronos import ChronosError
        self.assertIsNotNone(ChronosError)

    def test_import_is_stub_mode(self):
        """Test that is_stub_mode can be imported."""
        from chronos import is_stub_mode
        self.assertIsNotNone(is_stub_mode)

    def test_import_check_concurrent_support(self):
        """Test that check_concurrent_support can be imported."""
        from chronos import check_concurrent_support
        self.assertIsNotNone(check_concurrent_support)

    def test_import_version(self):
        """Test that version can be imported."""
        from chronos import __version__
        self.assertIsNotNone(__version__)
        self.assertIsInstance(__version__, str)


class TestStubModule(unittest.TestCase):
    """Test stub module functionality."""

    def test_stub_chronos_error_exists(self):
        """Test that ChronosStubError exists in stub module."""
        from chronos.stub import ChronosStubError
        self.assertIsNotNone(ChronosStubError)

    def test_stub_error_message_has_instructions(self):
        """Test that stub error message includes installation help."""
        from chronos.stub import ChronosStubError
        try:
            raise ChronosStubError()
        except ChronosStubError as e:
            message = str(e)
            self.assertIn("native library", message.lower())
            self.assertTrue(
                "pip install" in message or
                "reinstall" in message.lower()
            )

    def test_stub_partitioner_raises_error(self):
        """Test that stub Partitioner raises ChronosStubError."""
        from chronos.stub import Partitioner, ChronosStubError
        with self.assertRaises(ChronosStubError):
            Partitioner()

    def test_stub_is_stub_mode(self):
        """Test that stub module is_stub_mode returns True."""
        from chronos.stub import is_stub_mode
        self.assertTrue(is_stub_mode())

    def test_stub_check_concurrent_support(self):
        """Test that stub check_concurrent_support returns correct info."""
        from chronos.stub import check_concurrent_support
        info = check_concurrent_support()
        self.assertIsInstance(info, dict)
        self.assertTrue(info['stub_mode'])
        self.assertFalse(info['concurrent_possible'])
        self.assertFalse(info['nvidia_mps'])
        self.assertFalse(info['rocm'])
        self.assertFalse(info['opencl'])


class TestMainModuleStubMode(unittest.TestCase):
    """Test main module stub mode detection."""

    def test_is_stub_mode_returns_bool(self):
        """Test that is_stub_mode returns a boolean."""
        from chronos import is_stub_mode
        result = is_stub_mode()
        self.assertIsInstance(result, bool)

    def test_check_concurrent_support_returns_dict(self):
        """Test that check_concurrent_support returns a dict."""
        from chronos import check_concurrent_support
        result = check_concurrent_support()
        self.assertIsInstance(result, dict)
        self.assertIn('stub_mode', result)
        self.assertIn('nvidia_mps', result)
        self.assertIn('rocm', result)
        self.assertIn('opencl', result)
        self.assertIn('concurrent_possible', result)
        self.assertIn('recommended', result)


def _gpu_available():
    """Check if GPU is available for testing."""
    from chronos import is_stub_mode, Partitioner, ChronosError
    if is_stub_mode():
        return False
    try:
        p = Partitioner()
        return True
    except ChronosError:
        return False


class TestNativeLibrary(unittest.TestCase):
    """Test native library functionality (when available)."""

    def setUp(self):
        if not _gpu_available():
            self.skipTest("No GPU available")

    def test_partitioner_creation(self):
        """Test that Partitioner can be created."""
        from chronos import Partitioner
        p = Partitioner()
        self.assertIsNotNone(p)

    def test_execution_mode(self):
        """Test that execution mode is valid."""
        from chronos import Partitioner
        p = Partitioner()
        mode = p.get_execution_mode()
        self.assertIn(mode, ['concurrent', 'time_sliced', 'stub'])

    def test_backend_name(self):
        """Test that backend name is returned."""
        from chronos import Partitioner
        p = Partitioner()
        name = p.get_backend_name()
        self.assertIsInstance(name, str)
        self.assertGreater(len(name), 0)

    def test_check_concurrent_support_static(self):
        """Test static concurrent support check."""
        from chronos import Partitioner
        result = Partitioner.check_concurrent_support()
        self.assertIsInstance(result, bool)


class TestMultiprocessingImport(unittest.TestCase):
    """Test multiprocessing module imports."""

    def test_import_fork_safe_partition(self):
        """Test that ForkSafePartition can be imported."""
        try:
            from chronos.multiprocessing import ForkSafePartition
            self.assertIsNotNone(ForkSafePartition)
        except ImportError:
            self.skipTest("multiprocessing module not available")

    def test_import_partition_info_from_multiprocessing(self):
        """Test that PartitionInfo can be imported from multiprocessing."""
        try:
            from chronos.multiprocessing import PartitionInfo
            self.assertIsNotNone(PartitionInfo)
        except ImportError:
            self.skipTest("multiprocessing module not available")


if __name__ == "__main__":
    unittest.main()
