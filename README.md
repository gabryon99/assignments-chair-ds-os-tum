## Chair of Distributed Systems and Operating Systems Assignments

This repository contains two assignment for an open position at *Chair of Distributed and Operating Systems* at TUM University.

### Assignment 1: Memory Allocator

Implement two (`malloc` and `free`) library calls using the `sbrk` syscall and implement a simple allocation policy to do thread-safe allocation and free. 
Please also write a short paragraph explaining the policy you implemented, and how it works.

- [x] Write malloc implementation
- [x] Write free implementation
- [ ] Write a small paragraph about the implementation

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

- [ ] Communicates with the client program using shared memory buffer (POSIX `shm`)

#### Client program

- [ ] Enqueue requests/operations (insert, read a bucket, delete) to the server (that will operate on the hash table) via shared memory buffer (POSIX `shm`)