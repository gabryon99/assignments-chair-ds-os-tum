#include <cassert>
#include <cstdlib>

#include <iostream>
#include <new>
#include <thread>
#include <vector>

#include "mymalloc.h"

/***
 * Custom simple allocator based on Microsoft's example:
 * https://learn.microsoft.com/en-us/cpp/standard-library/allocators?view=msvc-170
 * @tparam T
 */
template <class T>
struct CustomAllocator {
    typedef T value_type;

    CustomAllocator() noexcept {} //default ctor not required by C++ Standard Library

    // A converting copy constructor:
    template<class U> CustomAllocator(const CustomAllocator<U>&) noexcept {}
    template<class U> bool operator==(const CustomAllocator<U>&) const noexcept {
        return true;
    }
    template<class U> bool operator!=(const CustomAllocator<U>&) const noexcept {
        return false;
    }

    T* allocate(const size_t n) const {
        if (n == 0) {
            return nullptr;
        }
        if (n > static_cast<size_t>(-1) / sizeof(T)) {
            throw std::bad_array_new_length();
        }

        // Use custom memory allocator
        void* const pv = allocator::malloc(n * sizeof(T));
        if (!pv) {
            throw std::bad_alloc();
        }

        return static_cast<T*>(pv);
    }

    void deallocate(T* const p, size_t) const noexcept {
        allocator::free(p);
    }
};


int main(int argc, char** argv) {

    // Runs some tests to check the library function's.

    // 1 - Allocate memory: allocate 3 bytes and check if they are aligned
    auto t1 = allocator::malloc(2);
    auto t1_header = allocator::get_header(t1);

    assert(t1_header->size == sizeof(intptr_t));

    // 2 - Allocate memory aligned to the machine's word
    auto t2 = allocator::malloc(sizeof(intptr_t));
    auto t2_header = allocator::get_header(t2);

    assert(t2_header->size == sizeof(intptr_t));

    // 3 - Free a memory block
    allocator::free(t1);
    assert(t1_header->used == false);

    // 4 - Request a new block from memory and see if this has been re-used
    auto t3 = allocator::malloc(2);
    assert(t3 == t1); // The memory pointed by t1 and now free is being reused!

    // Free all the previous memory before moving to the next test
    allocator::free(t2);
    allocator::free(t3);

    // 5 - Spawn some threads that perform allocations

    constexpr size_t MEM_ALLOCS = 256;
    constexpr size_t THREADS = 4;

    std::array<void*, MEM_ALLOCS> pointers{};
    std::array<std::thread, THREADS> threads{};

    size_t delta = pointers.size() / threads.size();

    for (size_t i = 0; i < threads.size(); i++) {
        threads[i] = std::thread([&pointers](size_t from, size_t to) -> void {
            for (size_t j = from; j < to; j++) {
                pointers[j] = allocator::malloc(8);
            }
        }, i * delta, (i * delta) + delta);
    }

    // Wait for thread to joining...
    for (auto& t: threads) {
        t.join();
    }

    delta = pointers.size() / threads.size();

    for (size_t i = 0; i < threads.size(); i++) {
        threads[i] = std::thread([&pointers](size_t from, size_t to) -> void {
            for (size_t j = from; j < to; j++) {
                allocator::free(pointers[j]);
            }
        }, i * delta, (i * delta) + delta);
    }

    // Wait for thread to joining...
    for (auto& t: threads) {
        t.join();
    }

    assert(allocator::are_blocks_freed());

    // 6 - Final test, implement a custom C++ allocator
    // and use it on STL vector
    std::vector<int, CustomAllocator<int>> numbers{};

    // Push 1000 numbers
    for (std::size_t i = 0; i < 1000; i++) {
        numbers.push_back(static_cast<int>(i));
    }

    std::cout << "All tests completed, no assertion raised up!\n";

    return 0;
}
