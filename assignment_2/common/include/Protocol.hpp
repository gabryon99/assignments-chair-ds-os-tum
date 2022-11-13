#ifndef ASSIGNMENT_2_PROTOCOL_HPP
#define ASSIGNMENT_2_PROTOCOL_HPP

#include <fcntl.h>
#include <unistd.h>
#include "RingBuffer.hpp"

namespace protocol {

    static constexpr const char* SHM_FILENAME = "/tmp/shm-queue";

    template <typename Key, typename Value>
    struct RequestMessage {

        enum class Type { Read, Insert, Remove };

        // Which client sent the message to the server
        int m_from_client_id{-1};
        bool m_async{false};

        Type m_type;

        Key m_key;
        Value m_value;

        RequestMessage() {}

        RequestMessage(int client_id, Type type, Key key, Value val, bool async = false) : m_from_client_id{client_id}, m_type{type},
            m_key{key}, m_value{val}, m_async{async} {}

        RequestMessage(int client_id, Type type, Key key, bool async = false) : m_from_client_id{client_id},
            m_type{type}, m_key{key}, m_async{async} {}

    };

    template <typename Value>
    struct ResponseMessage {

        enum class Type {
            Acknowledgment, SuccessfulRead, FailedRead
        };

        // To which client is the message referred to
        int m_dest_client{-1};

        Type m_type;
        Value m_value;

        ResponseMessage() {}

        ResponseMessage(int dest_client_id, Type type = Type::Acknowledgment) : m_dest_client{dest_client_id}, m_type{type} {}

        ResponseMessage(int dest_client_id, Type type, Value val) : m_dest_client{dest_client_id},
            m_type{type}, m_value{val} {}
    };

    template <typename Key, typename Value, size_t QueueSize = 64>
    struct SharedMessageQueue {

        using ReqMessage = RequestMessage<Key, Value>;
        using ResMessage = ResponseMessage<Value>;

        struct WrapperResMessage {
            bool m_valid{false};
            ResMessage m_msg;
        };

        RingBuffer<ReqMessage, QueueSize> m_requests{};
        RingBuffer<WrapperResMessage , QueueSize> m_responses{};

        std::atomic_int m_curr_id{0};

        SharedMessageQueue() { }

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
        void send_request(ReqMessage msg) noexcept {
            m_requests.put(msg);
        }

        ReqMessage receive_request() noexcept {
            return m_requests.pop();
        }

        ResMessage send_waiting_request(ReqMessage snd) noexcept {

            int client_id = snd.m_from_client_id;

            // Send the normal request to the server
            send_request(snd);

            // Now we should wait for the answer, we use the response queue.
            auto incoming_msg = m_responses.conditional_pop([client_id](const WrapperResMessage& head) -> bool {
                return client_id != head.m_msg.m_dest_client && !head.m_valid;
            }, [](WrapperResMessage& head) {
                head.m_valid = false;
            });

            return incoming_msg.m_msg;
        }

        void answer_pending_request(ResMessage msg) noexcept {
            m_responses.put(WrapperResMessage {
                    .m_valid = true,
                    .m_msg = msg
            });
        }

    };

}

#endif //ASSIGNMENT_2_PROTOCOL_HPP
