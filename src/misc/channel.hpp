#pragma once

#include "mutex.hpp"

// Channel with bounded queue size
// Discards the data if the queue is full
template<typename T>
class Channel {
public:
    Channel(size_t capacity) noexcept : m_capacity(capacity) {}

    [[nodiscard]] T Recv() noexcept {
        auto queue = m_queue.Lock();

        if (queue->empty()) {
            queue.cond_wait(m_condition_var);
        }

        auto data = std::move(queue->front());
        queue->pop_front();

        return data;
    };

    void Send(T &&data) noexcept {
        auto queue = m_queue.Lock();

        if (queue->size() == m_capacity) {
            return;
        }

        queue->push_back(std::forward<T>(data));
        m_condition_var.notify_one();
    }

    void ClearQueue() {
        m_queue.Lock()->clear();
    }

private:
    Mutex<std::deque<T>> m_queue;
    size_t m_capacity;

    std::condition_variable m_condition_var;
};
