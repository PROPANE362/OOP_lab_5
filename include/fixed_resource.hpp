#pragma once
#include <memory_resource>
#include <list>
#include <cstddef>
#include <algorithm>
using namespace std;

class FixedResource : public pmr::memory_resource {
    struct Block { void* p; size_t sz; size_t al; };
    
    void* buf_;
    size_t cap_;
    size_t off_ = 0;
    list<Block> used_;
    list<Block> free_;
    bool own_;

    static size_t align_up(size_t n, size_t a) {
        return (n + a - 1) & ~(a - 1);
    }

protected:
    void* do_allocate(size_t bytes, size_t alignment) override {
        for (auto it = free_.begin(); it != free_.end(); ++it) {
            if (it->sz >= bytes && it->al >= alignment) {
                void* p = it->p;
                used_.push_back(*it);
                free_.erase(it);
                return p;
            }
        }
        size_t aligned_off = align_up(off_, alignment);
        if (aligned_off + bytes > cap_) {
            throw bad_alloc();
        }
        void* p = static_cast<char*>(buf_) + aligned_off;
        off_ = aligned_off + bytes;
        used_.push_back({p, bytes, alignment});
        return p;
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override {
        for (auto it = used_.begin(); it != used_.end(); ++it) {
            if (it->p == p) {
                free_.push_back(*it);
                used_.erase(it);
                return;
            }
        }
    }

    bool do_is_equal(const pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

public:
    explicit FixedResource(size_t size)
        : buf_(::operator new(size)), cap_(size), own_(true) {}

    FixedResource(void* buffer, size_t size)
        : buf_(buffer), cap_(size), own_(false) {}

    ~FixedResource() override {
        used_.clear();
        free_.clear();
        if (own_) {
            ::operator delete(buf_);
        }
    }

    size_t used_count() const { return used_.size(); }
    size_t free_count() const { return free_.size(); }
};
