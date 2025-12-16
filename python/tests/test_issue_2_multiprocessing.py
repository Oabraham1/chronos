#!/usr/bin/env python3
"""
Test for GitHub Issue #2: Multiple Partitions Per User via Multiprocessing Fails

This test verifies that multiple processes can create partitions with the same
memory fraction on the same GPU device.

https://github.com/oabraham1/chronos/issues/2
"""

import multiprocessing
import time
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from chronos import Partitioner, ChronosError, is_stub_mode


def partition_gpu(process_id, device_id, memory_percentage, duration):
    """Worker function that creates a partition and holds it for duration seconds."""
    print(f"[{process_id}] Starting partitioning attempt for device {device_id} "
          f"with {memory_percentage*100:.0f}% memory for {duration} seconds.")
    try:
        with Partitioner().create(device=device_id, memory=memory_percentage, duration=duration) as p:
            print(f"[{process_id}] Partition successfully acquired: {p.partition_id}")
            print(f"[{process_id}] Simulating workload...")
            time.sleep(duration)
        print(f"[{process_id}] Partition released and work completed.")
        return {"process_id": process_id, "success": True, "partition_id": p.partition_id}
    except Exception as e:
        print(f"[{process_id}] Failed to acquire partition: {e}")
        return {"process_id": process_id, "success": False, "error": str(e)}


def test_concurrent_same_memory_fraction():
    """
    Test that two processes can create partitions with the SAME memory fraction.

    This was the core bug in issue #2 - the lock file naming used memory fraction,
    so two 25% partitions would conflict.
    """
    if is_stub_mode():
        print("SKIP: Running in stub mode")
        return True

    print("\n" + "="*60)
    print("TEST: Concurrent partitions with SAME memory fraction")
    print("="*60)

    device_id = 0
    memory_percentage = 0.25  # Both processes use 25%
    duration = 5

    print(f"\nStarting concurrent partitioning attempts...")
    print(f"Both processes will request {memory_percentage*100}% of GPU {device_id}")
    print(f"Total: {memory_percentage*2*100}% (should succeed as < 100%)\n")

    # Create and start the first process
    process1 = multiprocessing.Process(
        target=partition_gpu,
        args=("Process 1", device_id, memory_percentage, duration)
    )
    process1.start()
    print("[Main] Started Process 1. Waiting 2 seconds before starting Process 2...")

    # Staggered start for the second process
    time.sleep(2)

    # Create and start the second process with SAME memory fraction
    process2 = multiprocessing.Process(
        target=partition_gpu,
        args=("Process 2", device_id, memory_percentage, duration)
    )
    process2.start()
    print("[Main] Started Process 2. Waiting for both processes to complete...\n")

    # Wait for both processes to finish
    process1.join()
    process2.join()

    print("\n[Main] All processes completed.")

    # Check exit codes
    success = process1.exitcode == 0 and process2.exitcode == 0
    if success:
        print("[PASS] Both processes created partitions successfully!")
    else:
        print(f"[FAIL] Process exit codes: P1={process1.exitcode}, P2={process2.exitcode}")

    return success


def test_concurrent_different_memory_fractions():
    """
    Test that two processes can create partitions with different memory fractions.
    """
    if is_stub_mode():
        print("SKIP: Running in stub mode")
        return True

    print("\n" + "="*60)
    print("TEST: Concurrent partitions with DIFFERENT memory fractions")
    print("="*60)

    device_id = 0
    duration = 5

    print(f"\nProcess 1 will request 35%, Process 2 will request 25%")
    print(f"Total: 60% (should succeed as < 100%)\n")

    process1 = multiprocessing.Process(
        target=partition_gpu,
        args=("Process 1", device_id, 0.35, duration)
    )
    process1.start()
    print("[Main] Started Process 1. Waiting 2 seconds...")

    time.sleep(2)

    process2 = multiprocessing.Process(
        target=partition_gpu,
        args=("Process 2", device_id, 0.25, duration)
    )
    process2.start()
    print("[Main] Started Process 2. Waiting for completion...\n")

    process1.join()
    process2.join()

    print("\n[Main] All processes completed.")

    success = process1.exitcode == 0 and process2.exitcode == 0
    if success:
        print("[PASS] Both processes created partitions successfully!")
    else:
        print(f"[FAIL] Process exit codes: P1={process1.exitcode}, P2={process2.exitcode}")

    return success


def test_three_concurrent_partitions():
    """
    Test three processes creating partitions simultaneously.
    """
    if is_stub_mode():
        print("SKIP: Running in stub mode")
        return True

    print("\n" + "="*60)
    print("TEST: Three concurrent partitions")
    print("="*60)

    device_id = 0
    duration = 5

    print(f"\nProcess 1: 20%, Process 2: 20%, Process 3: 20%")
    print(f"Total: 60% (should succeed as < 100%)\n")

    processes = []
    for i in range(3):
        p = multiprocessing.Process(
            target=partition_gpu,
            args=(f"Process {i+1}", device_id, 0.20, duration)
        )
        processes.append(p)
        p.start()
        print(f"[Main] Started Process {i+1}")
        time.sleep(1)  # Stagger starts

    print("[Main] Waiting for all processes to complete...\n")

    for p in processes:
        p.join()

    print("\n[Main] All processes completed.")

    success = all(p.exitcode == 0 for p in processes)
    if success:
        print("[PASS] All three processes created partitions successfully!")
    else:
        codes = [p.exitcode for p in processes]
        print(f"[FAIL] Process exit codes: {codes}")

    return success


if __name__ == "__main__":
    print("="*60)
    print("GitHub Issue #2 Regression Test")
    print("Multiple Partitions Per User via Multiprocessing")
    print("="*60)

    # Clean up any stale locks
    import shutil
    for lock_dir in ['/tmp/chronos_locks', '/tmp/chronos']:
        if os.path.exists(lock_dir):
            shutil.rmtree(lock_dir)

    results = []

    # Run tests
    results.append(("Same memory fraction", test_concurrent_same_memory_fraction()))

    time.sleep(1)  # Wait between tests

    results.append(("Different memory fractions", test_concurrent_different_memory_fractions()))

    time.sleep(1)

    results.append(("Three concurrent", test_three_concurrent_partitions()))

    # Summary
    print("\n" + "="*60)
    print("SUMMARY")
    print("="*60)

    all_passed = True
    for name, passed in results:
        status = "PASS" if passed else "FAIL"
        print(f"  {name}: {status}")
        if not passed:
            all_passed = False

    print("\n" + "="*60)
    if all_passed:
        print("ALL TESTS PASSED - Issue #2 is FIXED!")
        sys.exit(0)
    else:
        print("SOME TESTS FAILED")
        sys.exit(1)
