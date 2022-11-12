#ifndef ASSIGNMENT_2_PROTOCOL_HPP
#define ASSIGNMENT_2_PROTOCOL_HPP

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

namespace protocol {

    static constexpr const char* SHM_FILENAME = "/tmp/shared-mem-queue";

    template <typename Key, typename Value>
    struct RequestMessage {

        enum class Type { Read, Insert, Remove };

        // Which client sent the message to the server
        int m_from_client_id;
        bool m_want_answer = true;

        Type m_type;

        Key m_key;
        Value m_value;

    };

    template <typename Value>
    struct ResponseMessage {

        enum class Type {
            Acknowledgment, SuccessfulRead, FailedRead
        };

        // To which client is the message referred to
        int m_to_client_id;

        Type m_type;
        Value m_value;
    };

    template <typename Key, typename Value, size_t QueueSize = 64>
    struct SharedMessageQueue {

        using ReqMessage = RequestMessage<Key, Value>;
        using ResMessage = ResponseMessage<Value>;

        struct WrapperResMessage {
            bool m_valid{false};
            ResMessage m_msg;
        };

        //region Requests
        ReqMessage m_requests[QueueSize];

        size_t m_head_request{0};
        size_t m_tail_request{0};

        pthread_mutex_t m_request_mutex;
        pthread_mutexattr_t m_request_attr;

        pthread_cond_t m_request_cond_full;
        pthread_condattr_t m_request_cond_full_attr;

        pthread_cond_t m_request_cond_empty;
        pthread_condattr_t m_request_cond_empty_attr;
        //endregion

        //region Responses
        WrapperResMessage m_responses[QueueSize];
        size_t m_head_response{0};
        size_t m_tail_response{0};

        pthread_mutex_t m_response_mutex;
        pthread_mutexattr_t m_response_attr;

        pthread_cond_t m_response_cond_full;
        pthread_condattr_t m_response_cond_full_attr;

        pthread_cond_t m_response_cond_empty;
        pthread_condattr_t m_response_cond_empty_attr;

        //endregion

        std::atomic_int m_curr_id{0};

        SharedMessageQueue() {

            // Initialize mutexes and condition variables

            pthread_mutexattr_init(&m_request_attr);
            pthread_mutexattr_setpshared(&m_request_attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&m_request_mutex, &m_request_attr);

            pthread_mutexattr_init(&m_response_attr);
            pthread_mutexattr_setpshared(&m_response_attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&m_response_mutex, &m_response_attr);

            pthread_condattr_init(&m_request_cond_empty_attr);
            pthread_condattr_setpshared(&m_request_cond_empty_attr, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&m_request_cond_empty, &m_request_cond_empty_attr);

            pthread_condattr_init(&m_request_cond_full_attr);
            pthread_condattr_setpshared(&m_request_cond_full_attr, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&m_request_cond_full, &m_request_cond_full_attr);

            pthread_condattr_init(&m_response_cond_empty_attr);
            pthread_condattr_setpshared(&m_response_cond_empty_attr, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&m_response_cond_empty, &m_response_cond_empty_attr);

            pthread_condattr_init(&m_response_cond_full_attr);
            pthread_condattr_setpshared(&m_response_cond_full_attr, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&m_response_cond_full, &m_response_cond_full_attr);
        }

        ~SharedMessageQueue() {

            pthread_mutex_destroy(&m_request_mutex);
            pthread_mutexattr_destroy(&m_request_attr);

            pthread_mutex_destroy(&m_response_mutex);
            pthread_mutexattr_destroy(&m_response_attr);

            pthread_cond_destroy(&m_request_cond_empty);
            pthread_cond_destroy(&m_request_cond_full);
            pthread_condattr_destroy(&m_request_cond_empty_attr);
            pthread_condattr_destroy(&m_request_cond_full_attr);

            pthread_cond_destroy(&m_response_cond_empty);
            pthread_cond_destroy(&m_response_cond_full);
            pthread_condattr_destroy(&m_response_cond_empty_attr);
            pthread_condattr_destroy(&m_response_cond_full_attr);
        }

        /***
         * Function used by a client to register itself to the shared queue.
         * @return
         */
        int get_client_id() { return m_curr_id++; }

        /***
         * Send a request message for the server (for Messages that
         * don't need an answer).
         * @param msg
         */
        void send_request(ReqMessage* msg) noexcept {

            pthread_mutex_lock(&m_request_mutex);

            while ((m_tail_request + 1) % QueueSize == (m_head_request)) {
                // The buffer for requests is full, wait for a free slot.
                pthread_cond_wait(&m_request_cond_full, &m_request_mutex);
            }

            // Instead of assigning, we just copy byte per byte

            //region Critical Section

            std::memcpy(&m_requests[m_tail_request], msg, sizeof(ReqMessage));
            m_tail_request = (m_tail_request + 1) % QueueSize;

            //endregion

            // An element has been inserted inside the buffer,
            // signal the waiting consumer.
            pthread_cond_signal(&m_request_cond_empty);

            pthread_mutex_unlock(&m_request_mutex);
        }

        void receive_request(ReqMessage* dest) noexcept {

            pthread_mutex_lock(&m_request_mutex);

            while (m_head_request == m_tail_request) {
                // The buffer is empty, wait for an element to be inserted.
                pthread_cond_wait(&m_request_cond_empty, &m_request_mutex);
            }

            //region Critical Section

            // Copy the head message to dest
            std::memcpy(dest, &m_requests[m_head_request], sizeof(ReqMessage));
            m_head_request = (m_head_request + 1) % QueueSize;
            //endregion

            // An element has been consumed, the waiting
            // producers can finally write.
            pthread_cond_signal(&m_request_cond_full);
            pthread_mutex_unlock(&m_request_mutex);
        }

        ResMessage send_waiting_request(ReqMessage* snd) noexcept {

            ResMessage incoming_msg;
            int client_id = snd->m_from_client_id;

            // Send the normal request to the server
            send_request(snd);

            // Now we should wait for the answer, we use the response queue.

            pthread_mutex_lock(&m_response_mutex);

            while (m_tail_response == m_head_response ||
                   (!m_responses[m_head_response].m_valid && m_responses[m_head_response].m_msg.m_to_client_id != client_id)) {
                pthread_cond_wait(&m_response_cond_empty, &m_response_mutex);
            }

            //region Critical section
            std::memcpy(&incoming_msg, &m_responses[m_head_response].m_msg, sizeof(ResMessage));
            m_responses[m_head_response].m_valid = false;
            m_head_response = (m_head_response + 1) % QueueSize;
            //endregion

            pthread_cond_signal(&m_response_cond_full);
            pthread_mutex_unlock(&m_response_mutex);

            return incoming_msg;
        }

        void answer_pending_request(ResMessage* msg) noexcept {

            pthread_mutex_lock(&m_response_mutex);

            while ((m_tail_response + 1) % QueueSize == (m_head_response)) {
                // The buffer for requests is full, wait for a free slot.
                pthread_cond_wait(&m_response_cond_full, &m_response_mutex);
            }

            //region Critical section
            std::memcpy(&m_responses[m_tail_response].m_msg, msg, sizeof(ResMessage));
            m_responses[m_tail_response].m_valid = true;
            m_tail_response = (m_tail_response + 1) % QueueSize;
            //endregion

            pthread_cond_signal(&m_response_cond_empty);
            pthread_mutex_unlock(&m_response_mutex);
        }

    };

}

#endif //ASSIGNMENT_2_PROTOCOL_HPP
