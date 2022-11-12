#ifndef ASSIGNMENT2_CLIENT_HPP
#define ASSIGNMENT2_CLIENT_HPP

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <optional>

#include "Common.hpp"
#include "Protocol.hpp"

template <typename Key, typename Value>
class Client {

    using ShmQueue = protocol::SharedMessageQueue<Key, Value>;
    using ReqMessage = typename ShmQueue::ReqMessage;
    using ResMessage = typename ShmQueue::ResMessage;

public:

    void start() noexcept {
        connect_to_server();
    }

    void send_async_insert_request(Key key, Value value) {
        ReqMessage msg;
        msg.m_type = ReqMessage::Type::Insert;
        msg.m_from_client_id = m_client_id;
        msg.m_key = key;
        msg.m_value = value;
        msg.m_want_answer = false;
        m_shared_queue->send_request(&msg);
    }

    void send_insert_request(Key key, Value value) {
        ReqMessage msg;
        msg.m_type = ReqMessage::Type::Insert;
        msg.m_from_client_id = m_client_id;
        msg.m_key = key;
        msg.m_value = value;
        m_shared_queue->send_waiting_request(&msg);
    }

    void send_async_remove_request(Key key) {
        ReqMessage msg;
        msg.m_type = ReqMessage::Type::Remove;
        msg.m_from_client_id = m_client_id;
        msg.m_key = key;
        m_shared_queue->send_request(&msg);
    }

    void send_remove_request(Key key) {
        ReqMessage msg;
        msg.m_type = ReqMessage::Type::Remove;
        msg.m_from_client_id = m_client_id;
        msg.m_key = key;
        m_shared_queue->send_waiting_request(&msg);
    }

    std::optional<Value> send_read_request(Key key) {
        ReqMessage msg;
        msg.m_type = ReqMessage::Type::Read;
        msg.m_from_client_id = m_client_id;
        msg.m_key = key;

        auto answer = m_shared_queue->send_waiting_request(&msg);
        if (answer.m_type == ResMessage::Type::FailedRead) {
            return {};
        }

        return answer.m_value;
    }

private:

    size_t m_client_id{0};
    ShmQueue* m_shared_queue{nullptr};

    void connect_to_server() noexcept {

        int fd;

        if ((fd = shm_open(protocol::SHM_FILENAME, O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
            perror("[client] Error during `shm_open`. Did you start the server?");
            std::exit(EXIT_FAILURE);
        }

        m_shared_queue = reinterpret_cast<ShmQueue*>(mmap(nullptr, sizeof(ShmQueue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

        close(fd);

        m_client_id = m_shared_queue->get_client_id();

        std::fprintf(stdout, "[client] :: registered as client #%ld...\n", m_client_id);
    }

};

#endif