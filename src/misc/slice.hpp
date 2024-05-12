#pragma once

// C++20 span-like slice
// T has to be contigious array-like initialized structure with size of "size * sizeof(T)"
template<typename T>
class Slice {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Slice(T* ref, size_t size) : m_ref(ref), m_size(size) {}
    Slice(T &ref, size_t size) : m_ref(&ref), m_size(size) {}

    template<size_t SIZE>
    Slice(std::array<T, SIZE> &array) : m_ref(array.data()), m_size(SIZE) {}

    Slice(std::vector<T> &vec) : m_ref(vec.data()), m_size(vec.size()) {}

    T& operator[](size_t i) noexcept {
        return m_ref[i];
    }

    const T& operator[](size_t i) const noexcept {
        return m_ref[i];
    }

    constexpr iterator begin() noexcept {
        return iterator(m_ref);
    }

    constexpr const_iterator begin() const noexcept {
        return const_iterator(m_ref);
    }

    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    constexpr iterator end() noexcept {
        return iterator(m_ref + m_size);
    }

    constexpr const_iterator end() const noexcept {
        return const_iterator(m_ref + m_size);
    }

    constexpr const_iterator cend() const noexcept {
        return end();
    }

    constexpr T* data() noexcept {
        return m_ref;
    }

    constexpr const T* data() const noexcept {
        return m_ref;
    }

    constexpr size_t size() const noexcept {
        return m_size;
    }
private:
    T* m_ref;
    const size_t m_size;
};

template<typename T>
class ConstSlice {
public:
    using iterator = const T*;

    ConstSlice(const T* ref, size_t size) : m_ref(ref), m_size(size) {}
    ConstSlice(const T& ref, size_t size) : m_ref(&ref), m_size(size) {}
    ConstSlice(Slice<T> view) : m_ref(view.data()), m_size(view.size()) {}

    template<size_t SIZE>
    ConstSlice(const std::array<T, SIZE> &array) : m_ref(array.data()), m_size(SIZE) {}

    ConstSlice(const std::vector<T> &vec) : m_ref(vec.data()), m_size(vec.size()) {}

    const T& operator[](size_t i) const noexcept {
        return m_ref[i];
    }

    constexpr iterator begin() const noexcept {
        return iterator(m_ref);
    }

    constexpr iterator end() const noexcept {
        return iterator(m_ref + m_size);
    }

    constexpr const T* data() const noexcept {
        return m_ref;
    }

    constexpr size_t size() const noexcept {
        return m_size;
    }
private:
    const T* m_ref;
    const size_t m_size;
};
