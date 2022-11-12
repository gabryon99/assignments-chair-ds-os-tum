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

    Client() {}
    ~Client() { free_memory_page(); }

    void start() noexcept {
        connect_to_server();
    }

    void send_insert_request(Key key, Value value, bool async = false) {
        ReqMessage insert_msg(m_client_id, ReqMessage::Type::Insert, key, value, async);
        m_shared_queue->send_waiting_request(insert_msg);
    }


    void send_remove_request(Key key, bool async = false) {
        ReqMessage remove_msg(m_client_id, ReqMessage::Type::Remove, key, async);
        m_shared_queue->send_waiting_request(remove_msg);
    }

    std::optional<Value> send_read_request(Key key) {

        ReqMessage read_msg(m_client_id, ReqMessage::Type::Read, key);

        auto answer = m_shared_queue->send_waiting_request(read_msg);

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
            panic("[client] Error during `shm_open`. Did you start the server?");
        }

        m_shared_queue = reinterpret_cast<ShmQueue*>(mmap(nullptr, sizeof(ShmQueue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        // We don't need to keep the file open
        close(fd);

        // Get a client id from the queue
        m_client_id = m_shared_queue->get_client_id();

        std::fprintf(stdout, "[client] :: registered as client #%ld...\n", m_client_id);
    }

    void free_memory_page() {
        if (munmap(reinterpret_cast<void*>(m_shared_queue), sizeof(ShmQueue)) == -1) {
            panic("[client] :: error while freeing shared memory page");
        }
    }

};

#endif