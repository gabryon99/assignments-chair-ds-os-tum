## Chair of Distributed Systems and Operating Systems Assignments

This repository contains two assignment for an open position at *Chair of Distributed and Operating Systems* at TUM University.

### Assignment 1: Memory Allocator

Implement two (`malloc` and `free`) library calls using the `sbrk` syscall and implement a 
simple allocation policy to do thread-safe allocation and free. 
Please also write a short paragraph explaining the policy you implemented, and how it works.

- [x] Write `malloc` implementation
- [x] Write `free` implementation
- [x] Write a small paragraph about the implementation (read below)

#### Allocation Policy

The implemented memory allocator uses a `pthread_mutex_t` to protect the heap's access for concurrent threads.
The locking procedure is coarse-grained, a thread lock all the list before access and modify it. At the end, the list will be unlocked, allowing other threads to work on it.

For the sake of simplicity, the memory policy chosen for the allocator is the **first-fit approach**. When a free block is found, and its size allows storing another new one, then the block is taken.

The runtime of the allocator is not the most-efficient one, it could be improved introducing the splitting and the coalescing of free blocks.
Moreover, a free list can be added to traverse only the free blocks, to speed up the allocation process.

#### Building process

The project requires `CMake` and a C++ compiler that supports the standard `C++20` version.

```bash
$ cd assignment_1
$ mkdir build/ && cd build/
$ cmake ..
$ cmake -build .
```

### Assignment 2: Shared-Memory Communication

Implement two programs in C/C++/Rust.

#### Server program:

- [x] Initialize a hash table of given size (command line)

- [x] Support insertion of items in the hash table

- [x] Hash table collisions are resolved by maintaining a linked list for each bucket/entry in the hash table

- [x] Supports concurrent operations (multithreading) to perform (insert, read, delete operations on the hash table)

- [x] Use readers-writer lock to ensure safety of concurrent operations, try to optimize the granularity. *Personal note*: implemented fine granularity using the lock-striping technique.

- [x] Communicates with the client program using shared memory buffer (POSIX `shm`)

#### Client program

- [x] Enqueue requests/operations (insert, read a bucket, delete) to the server (that will operate on the hash table) via shared memory buffer (POSIX `shm`)