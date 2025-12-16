"""
Tests for execution mode and backend queries.
"""

import unittest
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from chronos import Partitioner, ChronosError, is_stub_mode, check_concurrent_support


class TestExecutionMode(unittest.TestCase):
    """Test execution mode functionality."""

    def setUp(self):
        if is_stub_mode():
            self.skipTest("Running in stub mode")
        self.partitioner = Partitioner()

    def test_get_execution_mode_returns_string(self):
        """Test that get_execution_mode returns a valid string."""
        mode = self.partitioner.get_execution_mode()
        self.assertIsInstance(mode, str)
        self.assertIn(mode, ['concurrent', 'time_sliced', 'stub'])

    def test_get_backend_name_returns_string(self):
        """Test that get_backend_name returns a non-empty string."""
        name = self.partitioner.get_backend_name()
        self.assertIsInstance(name, str)
        self.assertGreater(len(name), 0)

    def test_check_concurrent_support_static_method(self):
        """Test that check_concurrent_support static method works."""
        result = Partitioner.check_concurrent_support()
        self.assertIsInstance(result, bool)


class TestConcurrentSupport(unittest.TestCase):
    """Test concurrent support detection."""

    def test_check_concurrent_support_returns_dict(self):
        """Test that check_concurrent_support returns expected dict."""
        result = check_concurrent_support()

        self.assertIsInstance(result, dict)
        self.assertIn('nvidia_mps', result)
        self.assertIn('rocm', result)
        self.assertIn('opencl', result)
        self.assertIn('concurrent_possible', result)
        self.assertIn('recommended', result)
        self.assertIn('stub_mode', result)

    def test_check_concurrent_support_types(self):
        """Test that check_concurrent_support values have correct types."""
        result = check_concurrent_support()

        self.assertIsInstance(result['nvidia_mps'], bool)
        self.assertIsInstance(result['rocm'], bool)
        self.assertIsInstance(result['opencl'], bool)
        self.assertIsInstance(result['concurrent_possible'], bool)
        self.assertIsInstance(result['stub_mode'], bool)
        # recommended can be None or string
        if result['recommended'] is not None:
            self.assertIsInstance(result['recommended'], str)

    def test_stub_mode_consistency(self):
        """Test that stub_mode flag is consistent."""
        result = check_concurrent_support()
        self.assertEqual(result['stub_mode'], is_stub_mode())


class TestBackendSelection(unittest.TestCase):
    """Test backend selection behavior."""

    def setUp(self):
        if is_stub_mode():
            self.skipTest("Running in stub mode")
        self.partitioner = Partitioner()

    def test_execution_mode_matches_backend(self):
        """Test that execution mode is consistent with backend name."""
        mode = self.partitioner.get_execution_mode()
        backend = self.partitioner.get_backend_name()

        # MPS should be concurrent
        if 'MPS' in backend:
            self.assertEqual(mode, 'concurrent')

        # ROCm should be concurrent
        if 'ROCm' in backend:
            self.assertEqual(mode, 'concurrent')

        # OpenCL should be time_sliced
        if 'OpenCL' in backend:
            self.assertEqual(mode, 'time_sliced')

        # Stub should be stub
        if 'Stub' in backend:
            self.assertEqual(mode, 'stub')

    def test_backend_name_is_known(self):
        """Test that backend name is one of the known backends."""
        backend = self.partitioner.get_backend_name()
        known_backends = [
            'NVIDIA MPS',
            'ROCm',
            'OpenCL',
            'Stub',
            'Unknown',
        ]
        # Backend should contain at least part of a known name
        found = False
        for known in known_backends:
            if known in backend:
                found = True
                break
        self.assertTrue(found, f"Unknown backend: {backend}")


if __name__ == "__main__":
    unittest.main()
