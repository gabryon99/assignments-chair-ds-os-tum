#ifndef ASSIGNMENT_1_MEMALLOC_MYMALLOC_H
#define ASSIGNMENT_1_MEMALLOC_MYMALLOC_H

namespace allocator {
    /***
     * Structure representing a block of memory inside the heap.
     */
    struct HeapBlock {
        // How big is the block inside the memory
        size_t size;
        // Is the current block in use?
        bool used;
        // A pointer to the next block in memory
        HeapBlock* next;
        // The data hold by the block where the user writes on.
        intptr_t data[1];
    };

    /***
     * Allocate memory using a custom memory allocator,
     * using a next-fit strategy.
     * @param size
     * @return
     */
    void* malloc(size_t size);

    /***
     * Get block's header for debugging information.
     * @param data
     * @return
     */
    HeapBlock* get_header(void* data);

    /***
     * Free the memory pointed by the ptr.
     * @param ptr
     */
    void free(void* ptr);

    /***
     * Check if all the block have been freed.
     * @return
     */
    bool are_blocks_freed();
}

#endif