#ifndef ASSIGNMENT_2_SERVER_HPP
#define ASSIGNMENT_2_SERVER_HPP

#include <thread>
#include <type_traits>

#include "HashTable.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename Key, typename Value>
class Server {


public:

    Server(std::size_t workers, std::size_t initial_capacity) : m_hashtable(initial_capacity) {
        m_threads.resize(workers);
    }

    ~Server() {
    }

   void start() {

        // The server MUST initialize the shared memory area...

        std::fprintf(stdout, "[server][info] :: starting server with %lu workers...\n", this->m_threads.size());

        unsigned worker_id = 0;

        for (auto& th: this->m_threads) {
            th = std::thread{[this, worker_id]() { this->loop(worker_id); }};
            worker_id++;
        }

        for (auto& th: this->m_threads) {
            th.join();
        }
   }

private:

    std::vector<std::thread> m_threads{};

    HashTable<Key, Value> m_hashtable;

    [[noreturn]]
    void loop(unsigned worker_id) {

        std::fprintf(stdout, "[server][info] :: worker#{%u}: starting main loop...\n", worker_id);

        while (true) {
            // auto incoming_message = this->read_next_message();
            // TODO: serve the request
        }
    }

};

#endif //ASSIGNMENT_2_SERVER_HPP
