import unittest
import time
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from chronos import Partitioner, ChronosError


class TestChronosPartitioner(unittest.TestCase):
    def setUp(self):
        self.partitioner = Partitioner()
        time.sleep(0.5)

    def tearDown(self):
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

    def test_create_and_release(self):
        try:
            partition = self.partitioner.create(device=0, memory=0.1, duration=5)
            self.assertIsNotNone(partition.partition_id)

            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 1)
            self.assertAlmostEqual(partitions[0].memory_fraction, 0.1, places=5)

            partition.release()

            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 0)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise

    def test_context_manager(self):
        try:
            with self.partitioner.create(device=0, memory=0.15, duration=5) as partition:
                self.assertIsNotNone(partition.partition_id)

                partitions = self.partitioner.list()
                self.assertEqual(len(partitions), 1)

            time.sleep(0.5)
            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 0)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise

    def test_automatic_expiration(self):
        try:
            partition = self.partitioner.create(device=0, memory=0.08, duration=2)
            self.assertIsNotNone(partition.partition_id)

            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 1)

            time.sleep(3)

            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 0)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise

    def test_multiple_partitions(self):
        try:
            p1 = self.partitioner.create(device=0, memory=0.03, duration=10)
            time.sleep(0.5)
            p2 = self.partitioner.create(
                device=0, memory=0.04, duration=10
            )  # Changed from 0.03 to 0.04

            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 2)

            p1.release()
            time.sleep(0.5)
            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 1)

            p2.release()
            time.sleep(0.5)
            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 0)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise

    def test_get_available(self):
        try:
            available_before = self.partitioner.get_available(device=0)
            self.assertGreater(available_before, 0)
            self.assertLessEqual(available_before, 100)

            partition = self.partitioner.create(device=0, memory=0.12, duration=5)
            time.sleep(0.5)

            available_after = self.partitioner.get_available(device=0)
            self.assertLess(available_after, available_before)

            partition.release()
            time.sleep(0.5)

            available_final = self.partitioner.get_available(device=0)
            self.assertAlmostEqual(available_final, available_before, delta=1.0)

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise

    def test_invalid_parameters(self):
        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=1.5, duration=10)

        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=0.0, duration=10)

        with self.assertRaises(ChronosError):
            self.partitioner.create(device=0, memory=0.5, duration=0)

    def test_time_remaining(self):
        try:
            partition = self.partitioner.create(device=0, memory=0.06, duration=10)

            time_left = partition.time_remaining
            self.assertGreater(time_left, 0)
            self.assertLessEqual(time_left, 10)

            time.sleep(2)

            time_left_after = partition.time_remaining
            self.assertLess(time_left_after, time_left)

            partition.release()

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise

    def test_partition_info(self):
        try:
            partition = self.partitioner.create(device=0, memory=0.18, duration=10)
            time.sleep(0.5)

            partitions = self.partitioner.list()
            self.assertEqual(len(partitions), 1)

            info = partitions[0]
            self.assertEqual(info.partition_id, partition.partition_id)
            self.assertAlmostEqual(info.memory_fraction, 0.18, places=5)
            self.assertEqual(info.duration_seconds, 10)
            self.assertTrue(info.active)
            self.assertIsNotNone(info.username)
            self.assertGreater(info.process_id, 0)

            partition.release()

        except ChronosError as e:
            if "No OpenCL" in str(e) or "Invalid device" in str(e):
                self.skipTest("No OpenCL devices available")
            raise


if __name__ == "__main__":
    unittest.main()
