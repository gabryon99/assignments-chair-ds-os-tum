## Chair of Distributed Systems Assignments

This repository contains two assignment for an open position at *Chair of Distributed and Operating Systems* at TUM University.

### Assignment 1: Memory Allocator

Implement two (`malloc` and `free`) library calls using the `sbrk` syscall and implement a simple allocation policy to do thread-safe allocation and free. 
Please also write a short paragraph explaining the policy you implemented, and how it works.

- [x] Write malloc implementation
- [x] Write free implementation
- [ ] Write a small paragraph about the implementation

### Assignment 2: Shared-Memory Communication

Implement two programs in C/C++/Rust.

#### Server program:

- Initialize a hash table of given size (command line)

- Support insertion of items in the hash table

- Hash table collisions are resolved by maintaining a linked list for each bucket/entry in the hash table

- Supports concurrent operations (multithreading) to perform (insert, read, delete operations on the hash table)

- Use readers-writer lock to ensure safety of concurrent operations, try to optimize the granularity

- Communicates with the client program using shared memory buffer (POSIX `shm`)

#### Client program

- Enqueue requests/operations (insert, read a bucket, delete) to the server (that will operate on the hash table) via shared memory buffer (POSIX `shm`)