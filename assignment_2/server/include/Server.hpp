#ifndef ASSIGNMENT_2_SERVER_HPP
#define ASSIGNMENT_2_SERVER_HPP

#include <thread>
#include <type_traits>

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#include "HashTable.hpp"
#include "Common.hpp"
#include "Protocol.hpp"

template<typename Key, typename Value>
class Server {

    using ShmQueue = protocol::SharedMessageQueue<Key, Value>;

public:
    Server(std::size_t workers, size_t initial_capacity)
            : m_hashtable(initial_capacity) {
        m_threads.resize(workers);
    }

    void start() {

        // The server MUST initialize the shared memory area...
        init_shared_queue();

        std::fprintf(stdout, "[server][info] :: starting server with %lu workers...\n", this->m_threads.size());

        unsigned worker_id = 0;

        for (auto &th: this->m_threads) {
            th = std::thread{[this, worker_id]() { this->loop(worker_id); }};
            worker_id++;
        }

        for (auto &th: this->m_threads) {
            th.join();
        }
    }

private:
    std::vector<std::thread> m_threads{};
    ShmQueue *m_shared_queue{nullptr};

    HashTable<Key, Value> m_hashtable;

    using ReqMessage = typename ShmQueue::ReqMessage;
    using ResMessage = typename ShmQueue::ResMessage;

    inline ReqMessage read_next_message() {
        return this->m_shared_queue->receive_request();
    }

    void init_shared_queue() {

        int fd;
        char *addr;

        // unlink previous shared object
        if (shm_unlink(protocol::SHM_FILENAME) == -1) {
            panic("[server] :: error while invoking shm_unlink");
        }

        if ((fd = shm_open(protocol::SHM_FILENAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1) {
            panic("[server] :: error while invoking shm_open, probably a server instance is runnign?");
        }

        if (ftruncate(fd, sizeof(ShmQueue)) == -1) {
            panic("[server] :: error while invoking ftruncate");
        }

        void* mmap_addr = mmap(nullptr, sizeof(ShmQueue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mmap_addr == MAP_FAILED) {
            panic("[server] :: error while invoking mmap");
        }

        close(fd);

        addr = reinterpret_cast<char*>(mmap_addr);

        this->m_shared_queue = new(addr) ShmQueue();
    }

    [[noreturn]] void loop(unsigned worker_id) {

        std::fprintf(stdout, "[server][info] :: worker#{%u}: starting main loop...\n", worker_id);

        while (true) {

            std::fprintf(stdout, "[server][info] :: worker#{%u}: ready to read next message...\n", worker_id);
            auto incoming_message = this->read_next_message();

            switch (incoming_message.m_type) {
                case ReqMessage::Type::Read: {

                    ResMessage answer(incoming_message.m_from_client_id);
                    std::fprintf(stdout, "[server][info] :: worker#{%u}: read key{%s}\n", worker_id, incoming_message.m_key.data);

                    if (auto val = m_hashtable.get(incoming_message.m_key)) {
                        std::fprintf(stdout, "[server][info] :: worker#{%u}: read key{%s}: success value{%s}!\n", worker_id, incoming_message.m_key.data, val.value()->data);
                        std::memcpy(&answer.m_value, &val.value()->data, sizeof(Value));
                        answer.m_type = ResMessage::Type::SuccessfulRead;
                    }
                    else {
                        std::fprintf(stdout, "[server][info] :: worker#{%u}: read key{%s}: fail!\n", worker_id, incoming_message.m_key.data);
                        answer.m_type = ResMessage::Type::FailedRead;
                    }

                    m_shared_queue->answer_pending_request(answer);

                    break;
                }
                case ReqMessage::Type::Insert: {

                    if (auto prev = m_hashtable.insert(incoming_message.m_key, incoming_message.m_value)) {
                        std::fprintf(stdout, "[server][info] :: worker#{%u}: insert key{%s}: popped out value{%s}\n", worker_id, incoming_message.m_key.data, prev.value().data);
                    }
                    else {
                        std::fprintf(stdout, "[server][info] :: worker#{%u}: insert operation key{%s}: new value{%s} registered!\n", worker_id, incoming_message.m_key.data, incoming_message.m_value.data);
                    }

                    if (!incoming_message.m_async) {
                        send_acknowledgement(incoming_message);
                    }

                    break;
                }
                case ReqMessage::Type::Remove: {

                    if (auto _ = m_hashtable.remove(incoming_message.m_key)) {
                        std::fprintf(stdout, "[server][info] :: worker#{%u}: remove key{%s} => success!\n", worker_id, incoming_message.m_key.data);
                    }
                    else {
                        std::fprintf(stdout, "[server][info] :: worker#{%u}: remove key{%s} => missing key\n", worker_id, incoming_message.m_key.data);
                    }

                    if (!incoming_message.m_async) {
                        send_acknowledgement(incoming_message);
                    }

                    break;
                }
            }

        }
    }

    void send_acknowledgement(const ReqMessage &incoming_message) {
        ResMessage response(incoming_message.m_from_client_id);
        m_shared_queue->answer_pending_request(response);
    }
};

#endif // ASSIGNMENT_2_SERVER_HPP
