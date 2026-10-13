// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_JSON_SCHEMA_RECORDING_RESOURCE_HPP
#define WEBCPP_TEST_JSON_SCHEMA_RECORDING_RESOURCE_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory_resource>

namespace webcpp::test {

// Keeps every block it gives, with its size and alignment, and counts what a caller gets wrong: a
// block that is not aligned as asked, and a deallocation that names no live block, or names one
// with another size or alignment than it was allocated with. A wrong deallocation goes upstream
// with the block's own size and alignment, so that the test itself stays defined.
class recording_resource final : public std::pmr::memory_resource {
public:
    explicit recording_resource(
        std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
        : upstream_(upstream) {}

    std::size_t allocations = 0;
    std::size_t bytes = 0;
    std::size_t misaligned = 0;
    std::size_t mismatched = 0;

    [[nodiscard]] std::size_t live_blocks() const noexcept { return blocks_.size(); }

private:
    struct block {
        std::size_t bytes;
        std::size_t alignment;
    };

    void* do_allocate(std::size_t size, std::size_t alignment) override {
        void* p = upstream_->allocate(size, alignment);
        ++allocations;
        bytes += size;
        misaligned += reinterpret_cast<std::uintptr_t>(p) % alignment == 0 ? 0 : 1;
        blocks_[p] = block{.bytes = size, .alignment = alignment};
        return p;
    }

    void do_deallocate(void* p, std::size_t size, std::size_t alignment) override {
        const auto found = blocks_.find(p);
        if (found == blocks_.end()) {
            ++mismatched;
            return;
        }
        const block given = found->second;
        blocks_.erase(found);
        mismatched += given.bytes == size && given.alignment == alignment ? 0 : 1;
        upstream_->deallocate(p, given.bytes, given.alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::pmr::memory_resource* upstream_;
    std::map<const void*, block> blocks_;
};

// An allocator that is not a pmr one, over a recording_resource: what json_schema adapts through
// its allocator_resource.
template <class T>
class recording_allocator {
public:
    using value_type = T;

    explicit recording_allocator(recording_resource* resource) noexcept : resource_(resource) {}

    template <class U>
    explicit recording_allocator(const recording_allocator<U>& other) noexcept
        : resource_(other.resource_) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(resource_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        resource_->deallocate(static_cast<void*>(p), n * sizeof(T), alignof(T));
    }

    template <class U>
    bool operator==(const recording_allocator<U>& other) const noexcept {
        return resource_ == other.resource_;
    }

private:
    template <class U>
    friend class recording_allocator;

    recording_resource* resource_;
};

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_JSON_SCHEMA_RECORDING_RESOURCE_HPP
