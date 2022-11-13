#include <cstdio>
#include <cstdlib>

#include <utility>

#include <pthread.h>
#include <unistd.h>

#include "mymalloc.h"

#ifdef __APPLE__
// macOS deprecated the usage of `sbrk` syscall, in favor of the `mmap`
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

using HeapBlock = allocator::HeapBlock;

namespace search_strategies {
    static HeapBlock* first_fit(size_t size, HeapBlock* starting) {

        auto block = starting;

        while (block != nullptr) {

            if (!block->used && block->size >= size) {
                return block;
            }

            block = block->next;
        }

        // We looked up all the list finding nothing free
        return nullptr;
    }
}


static const void* OOM_RESULT = reinterpret_cast<void*>(-1);

/***
 * Mutes used to protect memory for concurrent allocations.
 */
pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * A block pointer for the heaps beginning
 */
HeapBlock* heap_start = nullptr;
HeapBlock* heap_top = heap_start;

static void panic(const char* msg) {
    perror(msg);
    std::exit(EXIT_FAILURE);
}

/***
 * Return the greatest near multiplier of size(intptr_t).
 * @param n
 * @return The number of bytes that should be allocated to obtain a good alignment
 */
static inline size_t align(size_t n) {
    return (n + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1);
}

static inline size_t compute_alloc_size(size_t size) {
    // At the end, inside the heap we are requesting space for the header information
    // and the actual user's data:
    // * Size is how many bytes are required for the user's data
    // * sizeof(HeapBlock) is for holding metadata
    // * and, we subtract the sizeof of the HeapBlock::data because at the beginning
    //   we guarantee at least one element
    return size + sizeof(HeapBlock) - sizeof(std::declval<HeapBlock>().data);
}

static HeapBlock* request_memory_from_kernel(size_t size) {

    auto new_block = reinterpret_cast<HeapBlock*>(sbrk(0));

    auto alloc_size = compute_alloc_size(size);

    // Do we run out of memory in expending the heap's segment?
    if (sbrk(static_cast<int>(alloc_size)) == OOM_RESULT) {
        // Yes, we did, return a nullptr
        std::fprintf(stderr, "[error] :: run out of free memory!\n");
        return nullptr;
    }

    return new_block;
}

static HeapBlock* find_free_block(size_t size) {
    // Here we can use several strategies, let's use
    // a naive `first-fit` search.
    return search_strategies::first_fit(size, heap_start);
}

HeapBlock* allocator::get_header(void* data) {
    // Having the pointer of the user's data, we can get the header easily.
    return reinterpret_cast<HeapBlock*>(
            reinterpret_cast<char*>(data) + sizeof(std::declval<HeapBlock>().data) - sizeof(HeapBlock));
}

void* allocator::malloc(size_t size) {

    size_t aligned_size = align(size);

    // Naive implementation of a thread-safe malloc
    if (pthread_mutex_lock(&memory_mutex) != 0) {
        panic("Error while locking the memory mutex");
    }

#ifdef __APPLE__
    std::fprintf(stdout, "[th:#%ld] :: allocating memory of size %ld...\n", reinterpret_cast<long>(pthread_self()), aligned_size);
#endif

    // Find a free block before requesting more memory to the kernel
    if (auto free_block = find_free_block(aligned_size)) {
        // Mark the free block as used
        free_block->used = true;
        if (pthread_mutex_unlock(&memory_mutex) != 0) {
            panic("Error while unlocking the memory mutex");
        }
        return free_block->data;
    }

    auto block = request_memory_from_kernel(aligned_size);

    block->size = aligned_size;
    block->used = true;

    // Append the new memory block on the block list's head
    if (heap_start == nullptr) {
        heap_start = block;
    }

    if (heap_top != nullptr) {
        heap_top->next = block;
    }

    heap_top = block;

    if (pthread_mutex_unlock(&memory_mutex) != 0) {
        panic("Error while unlocking the memory mutex");
    }

    // The user can use the data pointed by the block,
    // without knowing any other info about the header.
    return reinterpret_cast<void*>(block->data);
}

void allocator::free(void* ptr) {

    HeapBlock* block_header = get_header(ptr);

    if (pthread_mutex_lock(&memory_mutex) != 0) {
        panic("Error while locking the memory mutex");
    }
#ifdef __APPLE__
    std::fprintf(stdout, "[th:#%ld] :: freeing memory pointed at %p...\n", reinterpret_cast<long>(pthread_self()), block_header->data);
#endif

    block_header->used = false;

    if (pthread_mutex_unlock(&memory_mutex) != 0) {
        panic("Error while unlocking the memory mutex");
    }
}

bool allocator::are_blocks_freed() {
    if (pthread_mutex_lock(&memory_mutex) != 0) {
        panic("Error while locking the memory mutex");
    }

    auto block = heap_start;
    while (block != nullptr) {
        if (block->used) {
            return false;
        }
        block = block->next;
    }

    if (pthread_mutex_unlock(&memory_mutex) != 0) {
        panic("Error while unlocking the memory mutex");
    }

    return true;
}