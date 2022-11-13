#ifndef ASSIGNMENT_2_RINGBUFFER_HPP
#define ASSIGNMENT_2_RINGBUFFER_HPP

#include <array>

#include <pthread.h>
#include "Common.hpp"

template<typename T, size_t BuffSize = 64>
class RingBuffer {

public:

    RingBuffer() {

        //region Initialize mutex
        pthread_mutexattr_init(&m_mutex_attr);
        pthread_mutexattr_setpshared(&m_mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&m_mutex, &m_mutex_attr);
        //endregion

        //region Initialize `full` condition variable
        pthread_condattr_init(&m_cond_attr_full);
        pthread_condattr_setpshared(&m_cond_attr_full, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&m_cond_full, &m_cond_attr_full);
        //endregion

        //region Initialize `empty` condition variable
        pthread_condattr_init(&m_cond_attr_empty);
        pthread_condattr_setpshared(&m_cond_attr_empty, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&m_cond_empty, &m_cond_attr_empty);
        //endregion

    }

    ~RingBuffer() {

        //region Destroy mutex
        pthread_mutex_destroy(&m_mutex);
        pthread_mutexattr_destroy(&m_mutex_attr);
        //endregion

        //region Destroy `full` condition variable
        pthread_cond_destroy(&m_cond_full);
        pthread_condattr_destroy(&m_cond_attr_full);
        //endregion

        //region Destroy `empty` condition variable
        pthread_cond_destroy(&m_cond_empty);
        pthread_condattr_destroy(&m_cond_attr_empty);
        //endregion
    }

    void put(T element) {

        if (pthread_mutex_lock(&m_mutex) != 0) {
            panic("Error while locking the RingBuffer's mutex");
        }

        while (is_full()) {
            if (pthread_cond_wait(&m_cond_empty, &m_mutex) != 0) {
                if (errno != ETIMEDOUT) {
                    panic("Error while waiting for `empty` condition variable");
                }
                else {
                    if (pthread_mutex_unlock(&m_mutex) != 0) {
                        panic("Error while unlocking the RingBuffer's mutex");
                    }
                    if (pthread_mutex_lock(&m_mutex) != 0) {
                        panic("Error while locking the RingBuffer's mutex");
                    }

                }
            }
        }

        //region Critical Section
        m_buffer[m_tail] = element;
        m_tail = (m_tail + 1) % BuffSize;
        m_count++;
        //endregion

        if (pthread_cond_signal(&m_cond_full) != 0) {
            panic("Error while sending signal for `empty` condition variable");
        }

        if (pthread_mutex_unlock(&m_mutex) != 0) {
            panic("Error while locking the RingBuffer's mutex");
        }
    }

    template <typename Predicate, typename PostEffect>
    T conditional_pop(Predicate predicate, PostEffect effect) {
        T elem;

        if (pthread_mutex_lock(&m_mutex) != 0) {
            panic("Error while locking the RingBuffer's mutex");
        }

        while (is_empty() || predicate(m_buffer[m_head])) {
            if (pthread_cond_wait(&m_cond_full, &m_mutex) != 0) {
                if (errno != ETIMEDOUT) {
                    panic("Error while waiting for `full` condition variable");
                }
                else {
                    if (pthread_mutex_unlock(&m_mutex) != 0) {
                        panic("Error while locking the RingBuffer's mutex");
                    }
                    if (pthread_mutex_lock(&m_mutex) != 0) {
                        panic("Error while locking the RingBuffer's mutex");
                    }
                }
            }
        }

        //region Critical section
        effect(m_buffer[m_head]);

        elem = std::move(m_buffer[m_head]);
        m_head = (m_head + 1) % BuffSize;
        m_count--;
        //endregion

        if (pthread_cond_signal(&m_cond_empty) != 0) {
            panic("Error while sending signal for `full` condition variable");
        }

        if (pthread_mutex_unlock(&m_mutex) != 0) {
            panic("Error while locking the RingBuffer's mutex");
        }

        return elem;
    }

    T pop() {
        return conditional_pop([](const T& head) -> bool {return false;}, [](const T& head){});
    }


private:
    std::array<T, BuffSize> m_buffer;

    size_t m_head{0};
    size_t m_tail{0};

    size_t m_count{0};

    pthread_mutex_t m_mutex;
    pthread_mutexattr_t m_mutex_attr;

    pthread_cond_t m_cond_full;
    pthread_condattr_t m_cond_attr_full;

    pthread_cond_t m_cond_empty;
    pthread_condattr_t m_cond_attr_empty;

    inline bool is_empty() { return m_count == 0; }

    inline bool is_full() {  return (m_count == BuffSize); }

};

#endif //ASSIGNMENT_2_RINGBUFFER_HPP
