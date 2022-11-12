#pragma once

#include <cstddef>
#include <vector>
#include <optional>
#include <functional>
#include <shared_mutex>
#include <atomic>

template <typename Key, typename Value>
class HashTable {
    
public:

    HashTable(std::size_t initial_capacity = 1 << 3) : m_capacity{initial_capacity}, m_init_capacity{initial_capacity} {
        this->m_table.resize(this->m_capacity);
        this->m_locks.resize(this->m_init_capacity);
        for (auto& ptr: this->m_locks) {
            ptr = std::make_unique<std::shared_timed_mutex>();
        }
    }

    std::size_t size() const {
        return this->m_size;
    }

    std::optional<Value> insert(Key key, Value value) noexcept {

        resize();

        auto hashed = std::hash<Key>{}(key);

        {
            std::lock_guard<std::shared_timed_mutex> writer_lock{*this->m_locks[hashed % this->m_locks.size()]};

            // Does the element exists already?
            Buckets& buckets = this->m_table[hashed % this->m_capacity];
            auto existing = std::find_if(buckets.begin(), buckets.end(), [&key](const Bucket& b) {
                return b.m_key == key;
            });

            if (existing != buckets.end()) {
                // We find an existing entry, we replace the bucket key and value
                auto old_value = std::move(existing->m_value);
                existing->m_value = value;
                existing->m_key = key;

                return std::optional{old_value};
            }

            // Let's append a new bucket to the bucket list, looking for a free one
            auto free_bucket = std::find_if(buckets.begin(), buckets.end(), [](const Bucket& b) {
                return b.m_status == Bucket::Status::Free;
            });

            if (free_bucket != buckets.end()) {
                // Let us reuse the existing bucket, instead creating a new one
                free_bucket->m_status = Bucket::Status::Occupied;
                free_bucket->m_key = std::move(key);
                free_bucket->m_value = std::move(value);
            }
            else {
                buckets.push_back(Bucket{std::move(key), std::move(value)});
            }

        }

        this->m_size++;

        return {};
    }

    /***
     * Get the value indexed by the key, if contained. (Read-Only operation)
     * @param key
     * @return A pointer to the value stored inside the hashtable, otherwise
     * a None option.
     */
    std::optional<Value*> get(Key key) noexcept {

        auto hashed = std::hash<Key>{}(key);
        {
            std::shared_lock<std::shared_timed_mutex> reader_lock{*this->m_locks[hashed % this->m_locks.size()]};

            Buckets& buckets = this->m_table[hashed % this->m_capacity];

            for (auto &bucket: buckets) {
                if (bucket.m_key == key) {
                    return std::optional{&bucket.m_value};
                }
            }
        }

        return {};
    }

    std::optional<std::pair<Key, Value>> remove(Key &key) noexcept {

        auto hashed = std::hash<Key>{}(key);
        {
            std::lock_guard<std::shared_timed_mutex> writer_lock{*this->m_locks[hashed % this->m_locks.size()]};

            Buckets& buckets = this->m_table[hashed % this->m_capacity];
            auto existing = std::find_if(buckets.begin(), buckets.end(), [&key](const Bucket& b) {
                return b.m_key == key;
            });

            if (existing != buckets.end()) {

                auto val = std::move(existing->m_value);
                auto k = std::move(existing->m_key);

                existing->m_status = Bucket::Status::Free;

                return std::optional{std::pair{key, val}};
            }
        }

        this->m_size--;

        return {};
    }

    /***
     * Check if the value indexed by key is contained inside the map. (Read-Only operation)
     * @param key
     * @return True if the values is contained, false otherwise.
     */
    bool has(Key& key) noexcept {

        auto hashed = std::hash<Key>{}(key);
        std::shared_lock<std::shared_timed_mutex> reader_lock{*(this->m_locks[hashed % this->m_locks.size()])};

        Buckets& buckets = this->m_table[hashed % this->m_capacity];

        auto found = std::find_if(buckets.begin(), buckets.end(), [=](const Bucket& b) {
            return b.m_key == key;
        });

        return (found != buckets.end());
    }

private:

    struct Bucket {

        // Status of the current Bucket
        enum class Status {
            // The current bucket contains a valid (key, value) couple.
            Occupied,
            // The bucket does not contain a valid couple anymore, it could be re-used to spare memory.
            Free
        };

        Key m_key;
        Value m_value;
        Status m_status;

        Bucket(Key&& key, Value&& value) : m_key{std::move(key)}, m_value{std::move(value)}, m_status{Status::Occupied} {}
    };

    using Buckets = std::vector<Bucket>;
    using Mutex = std::unique_ptr<std::shared_timed_mutex>;

    std::vector<Buckets> m_table{};
    std::vector<Mutex> m_locks{};

    std::atomic_size_t m_size{0};
    std::size_t m_capacity{0};
    std::size_t m_init_capacity{0};

    static constexpr float MAX_LOAD = 0.75f;

    /***
     * Resize the current hashtable capacity if we exceed the MAX_LOAD factor.
     */
    void resize() {
        // Should we resize the hash-table? Let's start locking all the locks
        // before going on.
        for (Mutex& mutex: this->m_locks) {
            mutex->lock();
        }

        // If the new size is going to be more than the actual capacity
        // scaled by the MAX_LOAD factor, then we should resize the hashtable.
        if (this->m_size + 1 > this->m_capacity * MAX_LOAD) {
            // Double the capacity
            this->m_capacity *= 2;
            this->m_table.resize(this->m_capacity);
        }

        // Release all the locks in the same order.
        for (Mutex& mutex: this->m_locks) {
            mutex->unlock();
        }
    }

};