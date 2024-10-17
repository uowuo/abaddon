#pragma once

#include "thread"
#include "variant"

#include "channel.hpp"

using Terminate = std::monostate;

template<typename ThreadData, typename Callable>
class ThreadPool {
public:
    using ThreadMessage = std::variant<ThreadData, Terminate>;

    ThreadPool(Callable callable, size_t channel_capacity) noexcept :
        m_callable(std::move(callable)),
        m_channel(Channel<ThreadMessage>(channel_capacity))
    {
        auto concurrency = std::thread::hardware_concurrency() / 2;
        if (concurrency == 0) {
            concurrency = 2;
        }

        MaxThreads = concurrency;
    }

    ~ThreadPool() noexcept {
        Clear();
    }

    void AddThread() noexcept {
        if (GetThreadCount() == MaxThreads) {
            return;
        }

        m_threads.emplace(m_threads.end(), m_callable, std::ref(m_channel));
        m_threads.back().detach();
    }

    void RemoveThread() {
        if (m_threads.empty()) {
            return;
        }

        m_channel.Send(Terminate());
        m_threads.pop_back();
    }

    void Clear() {
        const auto thread_count = GetThreadCount();

        // Clear the queue and send termination messages to remaining threads
        m_channel.ClearQueue();
        for (int i = 0; i < thread_count; i++) {
            m_channel.Send(Terminate());
        }

        m_threads.clear();
    }

    size_t GetThreadCount() const noexcept {
        return m_threads.size();
    }

    void SendToPool(ThreadData &&data) {
        m_channel.Send(std::move(data));
    }

    size_t MaxThreads;

private:
    std::vector<std::thread> m_threads;
    Channel<ThreadMessage> m_channel;
    Callable m_callable;
};
