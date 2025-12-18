#pragma once
#include <memory_resource>
#include <iterator>
#include <cstddef>
#include <utility>
#include <stdexcept>

template<typename T>
class DynArray {
public:
    using allocator_type = std::pmr::polymorphic_allocator<T>;

    class ConstIterator;

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() : p_(nullptr) {}
        explicit Iterator(T* p) : p_(p) {}

        reference operator*() const { return *p_; }
        pointer operator->() const { return p_; }
        Iterator& operator++() { ++p_; return *this; }
        Iterator operator++(int) { Iterator t = *this; ++p_; return t; }
        bool operator==(const Iterator& o) const { return p_ == o.p_; }
        bool operator!=(const Iterator& o) const { return p_ != o.p_; }
        pointer base() const { return p_; }

    private:
        T* p_;
    };

    class ConstIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        ConstIterator() : p_(nullptr) {}
        explicit ConstIterator(const T* p) : p_(p) {}
        ConstIterator(const Iterator& it) : p_(it.base()) {}

        reference operator*() const { return *p_; }
        pointer operator->() const { return p_; }
        ConstIterator& operator++() { ++p_; return *this; }
        ConstIterator operator++(int) { ConstIterator t = *this; ++p_; return t; }
        bool operator==(const ConstIterator& o) const { return p_ == o.p_; }
        bool operator!=(const ConstIterator& o) const { return p_ != o.p_; }

    private:
        const T* p_;
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;

    explicit DynArray(allocator_type alloc = {})
        : alloc_(alloc), data_(nullptr), sz_(0), cap_(0) {}

    explicit DynArray(std::size_t n, allocator_type alloc = {})
        : alloc_(alloc), data_(nullptr), sz_(0), cap_(0) {
        reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            push_back(T{});
        }
    }

    DynArray(const DynArray& o)
        : alloc_(o.alloc_), data_(nullptr), sz_(0), cap_(0) {
        reserve(o.sz_);
        for (std::size_t i = 0; i < o.sz_; ++i) {
            push_back(o.data_[i]);
        }
    }

    DynArray(DynArray&& o) noexcept
        : alloc_(o.alloc_), data_(o.data_), sz_(o.sz_), cap_(o.cap_) {
        o.data_ = nullptr;
        o.sz_ = 0;
        o.cap_ = 0;
    }

    ~DynArray() { clear(); dealloc(); }

    DynArray& operator=(const DynArray& o) {
        if (this != &o) {
            clear();
            dealloc();
            alloc_ = o.alloc_;
            reserve(o.sz_);
            for (std::size_t i = 0; i < o.sz_; ++i) {
                push_back(o.data_[i]);
            }
        }
        return *this;
    }

    DynArray& operator=(DynArray&& o) noexcept {
        if (this != &o) {
            clear();
            dealloc();
            alloc_ = o.alloc_;
            data_ = o.data_;
            sz_ = o.sz_;
            cap_ = o.cap_;
            o.data_ = nullptr;
            o.sz_ = 0;
            o.cap_ = 0;
        }
        return *this;
    }

    void push_back(const T& v) {
        if (sz_ >= cap_) grow();
        std::allocator_traits<allocator_type>::construct(alloc_, data_ + sz_, v);
        ++sz_;
    }

    void push_back(T&& v) {
        if (sz_ >= cap_) grow();
        std::allocator_traits<allocator_type>::construct(alloc_, data_ + sz_, std::move(v));
        ++sz_;
    }

    template<typename... Args>
    T& emplace_back(Args&&... args) {
        if (sz_ >= cap_) grow();
        std::allocator_traits<allocator_type>::construct(alloc_, data_ + sz_, std::forward<Args>(args)...);
        return data_[sz_++];
    }

    void pop_back() {
        if (sz_ > 0) {
            --sz_;
            std::allocator_traits<allocator_type>::destroy(alloc_, data_ + sz_);
        }
    }

    void clear() {
        for (std::size_t i = 0; i < sz_; ++i) {
            std::allocator_traits<allocator_type>::destroy(alloc_, data_ + i);
        }
        sz_ = 0;
    }

    void reserve(std::size_t n) {
        if (n > cap_) {
            T* nd = alloc_.allocate(n);
            for (std::size_t i = 0; i < sz_; ++i) {
                std::allocator_traits<allocator_type>::construct(alloc_, nd + i, std::move(data_[i]));
                std::allocator_traits<allocator_type>::destroy(alloc_, data_ + i);
            }
            if (data_) alloc_.deallocate(data_, cap_);
            data_ = nd;
            cap_ = n;
        }
    }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    T& at(std::size_t i) {
        if (i >= sz_) throw std::out_of_range("index");
        return data_[i];
    }

    const T& at(std::size_t i) const {
        if (i >= sz_) throw std::out_of_range("index");
        return data_[i];
    }

    T& front() { return data_[0]; }
    const T& front() const { return data_[0]; }
    T& back() { return data_[sz_ - 1]; }
    const T& back() const { return data_[sz_ - 1]; }
    T* data() { return data_; }
    const T* data() const { return data_; }

    std::size_t size() const { return sz_; }
    std::size_t capacity() const { return cap_; }
    bool empty() const { return sz_ == 0; }

    iterator begin() { return iterator(data_); }
    iterator end() { return iterator(data_ + sz_); }
    const_iterator begin() const { return const_iterator(static_cast<const T*>(data_)); }
    const_iterator end() const { return const_iterator(static_cast<const T*>(data_ + sz_)); }
    const_iterator cbegin() const { return const_iterator(static_cast<const T*>(data_)); }
    const_iterator cend() const { return const_iterator(static_cast<const T*>(data_ + sz_)); }

    allocator_type get_allocator() const { return alloc_; }

private:
    void grow() {
        reserve(cap_ == 0 ? 1 : cap_ * 2);
    }

    void dealloc() {
        if (data_) {
            alloc_.deallocate(data_, cap_);
            data_ = nullptr;
            cap_ = 0;
        }
    }

    allocator_type alloc_;
    T* data_;
    std::size_t sz_;
    std::size_t cap_;
};
