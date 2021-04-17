#include <iostream>
#include <cstddef>
#include <utility>
#include <memory>

template <typename T>
struct RawMemory {
    T* buf = nullptr;
    size_t cp = 0;

    static T* Allocate(size_t n) {
        return static_cast<T*>(operator new(n * sizeof(T)));
    }
    static void Deallocate(T* buf) {
        operator delete (buf);
    }

    RawMemory() = default;

    RawMemory(size_t n) {
        buf = Allocate(n);
        cp =n;
    }

    ~RawMemory() {
        Deallocate(buf);
    }
    T* operator + (size_t i) {
        return buf + i;
    }
    const T* operator + (size_t i) const {
        return buf + i;
    }
    T& operator[] (size_t i) {
        return buf[i];
    }
    const T& operator[] (size_t i) const {
        return buf[i];
    }
    void Swap(RawMemory& other) {
        std::swap(buf, other.buf);
        std::swap(cp, other.cp);
    }
    RawMemory(const RawMemory&) = delete;

    RawMemory(RawMemory&& other) {
        Swap(other);
    }
    RawMemory& operator= (const RawMemory&) = delete;

    RawMemory& operator = (RawMemory&& other) {
        Swap(other);
        return *this;
    }
};



template <typename T>
class Vector {
private:
    RawMemory<T> data;
    size_t sz = 0;

    static void Construct(void * buf) {
        new (buf) T();
    }
    static void Construct(void * buf, const T& elem) {
        new (buf) T(elem);
    }
    static void Construct(void * buf, const T&& elem) {
        new (buf) T(std::move(elem));
    }
    static void Destroy(T* buf) {
        buf->~T();
    }

public:
    Vector() = default;
    size_t size() const {
        return sz;
    }
    size_t capacity() const {
        return data.cp;
    }
    const T& operator[](size_t i) const {
        return data[i];
    }
    T& operator[](size_t i) {
        return data[i];
    }

    Vector(size_t n) : data(n) {
        std::uninitialized_value_construct_n(
                data.buf, n);
        sz = n;
    }
    Vector(const Vector& other) : data(other.sz) {
        std::uninitialized_copy_n(
                other.data.buf, other.sz, data.buf);
        sz = other.sz;
    }
    void swap(Vector& other) {
        data.Swap(other.data);
        std::swap(sz, other.sz);
    }
    Vector(Vector&& other) {
        swap(other);
    }



    ~Vector() {
        std::destroy_n(data.buf, sz);
    }
    Vector& operator=(const Vector& other) {
        if (other.sz > data.cp) {
            Vector tmp(other);
            swap(tmp);
        } else {
            for (size_t i = 0; i < sz && i < other.sz; ++i) {
                data[i] = other[i];
            }
            if (sz < other.sz) {
                std::uninitialized_copy_n(
                        other.data.buf + sz,
                        other.sz - sz,
                        data.buf + sz);
            } else if (sz > other.sz) {
                std::destroy_n(
                        data.buf + other.sz,
                        sz - other.sz);
            }
            sz = other.sz;
        }
        return *this;
    }

    Vector& operator=(Vector&& other)  noexcept {
        swap(other);
        return *this;
    }

    void reserve(size_t n) {
        if (n > data.cp) {
            RawMemory<T> data2(n);
            std::uninitialized_move_n(
                    data.buf, sz, data2.buf);
            std::destroy_n(data.buf, sz);

            data.Swap(data2);
        }
    }

    void resize(size_t n) {
        reserve(n);
        if (sz < n) {
            std::uninitialized_value_construct_n(
                    data + sz, n - sz);
        } else if (sz > n) {
            std::destroy_n(
                    data + n, sz - n);
        }
        sz = n;
    }

    void push_back(const T& elem) {
        if (sz == data.cp) {
            reserve(sz == 0 ? 1 : sz * 2);
        }
        new(data + sz) T(elem);
        ++sz;
    }

    void push_back(T&& elem) {
        if (sz == data.cp) {
            reserve(sz == 0 ? 1 : sz * 2);
        }
        new(data + sz) T(std::move(elem));
        ++sz;
    }

    void pop_back() {
        std::destroy_at(data + sz - 1);
        --sz;
    }
    void clear() {
        while (sz > 0) {
            pop_back();
        }
    }
    T* begin() const {
        return data.buf;
    }
    T* end() const {
        return data.buf + sz;
    }
    template<typename ... Args>
    T& emplace_back(Args&& ... args) {
        if (sz == data.cp) {
            reserve(sz == 0 ? 1 : sz * 2);
        }
        auto elem = new (data + sz) T(std::forward<args>(args)...);
        ++sz;
        return *elem;
    }
};
