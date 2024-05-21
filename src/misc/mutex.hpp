#pragma once

// RAII style mutex guard
// Unlocks mutex upon destruction
template<typename T>
class MutexGuard {
public:
    MutexGuard(T &object, std::unique_lock<std::mutex> &&lock) :
        m_object(object),
        m_lock(std::move(lock)) {}

    MutexGuard(MutexGuard<T> &&other) noexcept :
        m_lock(std::move(other.m_lock)),
        m_object(other.m_object) {}

    // Only implement arrow operator
    // Dereference operator would expose the inner object
    T* operator->() noexcept {
        return &m_object;
    }

    const T* operator->() const noexcept {
        return &m_object;
    }

    void cond_wait(std::condition_variable &condition_var) noexcept {
        condition_var.wait(m_lock);
    }

    // Provide methods for container types
    auto& operator[](size_t index) noexcept {
        return m_object[index];
    }

    const auto& operator[](size_t index) const noexcept {
        return m_object[index];
    }

    auto begin() noexcept {
        return m_object.begin();
    }

    auto end() noexcept {
        return m_object.end();
    }

    auto begin() const noexcept  {
        return m_object.begin();
    }

    auto end() const noexcept {
        return m_object.end();
    }

private:
    std::unique_lock<std::mutex> m_lock;
    T &m_object;
};

// RAII style mutex
// Owns the provided object
// Use Lock with MutexGuard or LockScope with closure to access protected object
template <typename T>
class Mutex {

public:
    Mutex(T&& object) : m_object(std::forward<T>(object)) {}

    template<typename... Args>
    Mutex(Args&&... args) : m_object( T(std::forward<Args>(args)...) ) {}

    Mutex(const Mutex&) = delete;

    template<typename C>
    auto LockScope(C closure) noexcept {
        std::scoped_lock<std::mutex> lock(m_mutex);
        return closure(m_object);
    }

    template<typename C>
    auto LockScope(C closure) const noexcept {
        std::scoped_lock<std::mutex> lock(m_mutex);
        return closure(m_object);
    }

    [[nodiscard]] MutexGuard<T> Lock() noexcept {
        std::unique_lock<std::mutex> lock(m_mutex);
        return MutexGuard(m_object, std::move(lock));
    }

    // Make sure to return const MutexGuard here
    [[nodiscard]] const MutexGuard<T> Lock() const noexcept {
        std::unique_lock<std::mutex> lock(m_mutex);
        return MutexGuard(m_object, std::move(lock));
    }

private:
    mutable std::mutex m_mutex;
    mutable T m_object; // Needs to be mutable for const to work
};
