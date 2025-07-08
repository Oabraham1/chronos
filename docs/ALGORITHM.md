Chronos Partitioning Algorithm
Formal Algorithm Description
Algorithm 1: CreatePartition
Input: deviceIndex d, memoryFraction f, duration t

Output: partitionId or ∅

1: if d∈
/
[0,∣Devices∣) or f∈
/
(0,1] or t≤0 then 2:     return ∅

3: end if 4:

5: ACQUIRE(partitionMutex)

6:

7: // Check for conflicting locks 8: lockPath ← GenerateLockPath(d, f)

9: if EXISTS(lockPath) then 10:     owner ← ReadLockOwner(lockPath)

11:     if owner

= currentUser then 12:         RELEASE(partitionMutex)

13:         return ∅

14:     end if 15: end if 16:

17: // Admission control 18: device ← Devices[d]

19: requestedMemory ← device.totalMemory × f

20: if requestedMemory > device.availableMemory then 21:     RELEASE(partitionMutex)

22:     return ∅

23: end if 24:

25: // Create partition 26: partitionId ← GenerateUniqueId()

27: if not CreateLockFile(lockPath, partitionId) then 28:     RELEASE(partitionMutex)

29:     return ∅

30: end if 31:

32: // Update state 33: device.availableMemory ← device.availableMemory - requestedMemory

34: partition ← {

35:     id: partitionId,

36:     deviceId: d,

37:     memoryFraction: f,

38:     duration: t,

39:     startTime: NOW(),

40:     active: true,

41:     owner: currentUser,

42:     pid: currentPid

43: }

44: Partitions.add(partition)

45:

46: RELEASE(partitionMutex)

47: return partitionId

Algorithm 2: MonitorPartitions (Background Thread)
1: while running do 2:     ACQUIRE(partitionMutex)

3:     currentTime ← NOW()

4:

5:     for each partition p in Partitions do 6:         if p.active then 7:             elapsed ← currentTime - p.startTime

8:             if elapsed ≥ p.duration then 9:                 ReleasePartitionResources(p)

10:                p.active ← false 11:            end if 12:        end if 13:     end for 14:

15:     // Remove inactive partitions 16:     Partitions.removeIf(p→¬p.active)

17:

18:     RELEASE(partitionMutex)

19:     SLEEP(1 second)

20: end while

Algorithm 3: ReleasePartition
Input: partitionId pid

Output: success (boolean)

1: ACQUIRE(partitionMutex)

2:

3: partition ← Partitions.find(p → p.id = pid)

4: if partition = ∅ or not partition.active then 5:     RELEASE(partitionMutex)

6:     return false 7: end if 8:

9: // Verify ownership 10: if partition.owner

= currentUser then 11:     RELEASE(partitionMutex)

12:     return false 13: end if 14:

15: // Release resources 16: ReleasePartitionResources(partition)

17: partition.active ← false 18:

19: RELEASE(partitionMutex)

20: return true

Theoretical Properties
Theorem 1: Mutual Exclusion Statement: No two partitions can allocate overlapping memory regions on the same device.

Proof: By construction, each partition request acquires the global partitionMutex before checking and modifying device memory state. The lock file mechanism ensures inter-process coordination. Therefore, memory allocation decisions are serialized, preventing overlapping allocations. □

Theorem 2: Progress Guarantee Statement: Every partition will eventually be released, ensuring system progress.

Proof: Each partition has a finite duration t. The monitor thread checks all partitions every second. For any partition p with duration t, it will be released within time interval [t,t+1] seconds. This holds even if the creating process crashes, as the monitor thread operates independently. □

Theorem 3: Fairness Statement: Under FIFO scheduling, partition requests are satisfied in order of arrival.

Proof: The partitionMutex ensures requests are processed sequentially. Combined with the OS scheduler's fairness guarantees for mutex acquisition, this provides FIFO ordering for partition creation. □

Complexity Analysis
Operation

Time Complexity

Space Complexity

CreatePartition

O(1)
∗

O(1)

ReleasePartition

O(n)

O(1)

ListPartitions

O(n)

O(n)

MonitorPartitions

O(n)

O(1)

GetAvailableMemory

O(1)

O(1)

\*Amortized, assuming constant number of devices and efficient lock file operations

Where:

n = number of active partitions

d = number of GPU devices

Design Rationale
Why Time-Based Partitioning?
Predictability: Users know exactly how long they have exclusive access.

Fairness: Prevents indefinite resource hoarding.

Simplicity: No complex scheduling algorithms required.

Self-Healing: Automatic cleanup prevents resource leaks.

Why Filesystem-Based Locks?
Portability: Works across all operating systems.

Persistence: Survives process crashes.

Simplicity: No kernel modifications required.

Visibility: Lock files can be inspected for debugging.

Trade-offs
Granularity: 1-second monitoring interval trades precision for efficiency.

Memory Fragmentation: Fraction-based allocation may leave small unusable chunks.

Lock Contention: Global mutex may become a bottleneck at very high request rates.
