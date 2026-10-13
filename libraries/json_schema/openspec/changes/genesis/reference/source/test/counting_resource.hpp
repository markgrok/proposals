// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#ifndef WEBCPP_TEST_JSON_SCHEMA_COUNTING_RESOURCE_HPP
#define WEBCPP_TEST_JSON_SCHEMA_COUNTING_RESOURCE_HPP

#include <cstddef>
#include <memory_resource>

namespace webcpp::test {

// Counts what it allocates, frees and holds at its peak, and passes each request upstream.
class counting_resource final : public std::pmr::memory_resource {
public:
    explicit counting_resource(
        std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
        : upstream_(upstream) {}

    std::size_t allocations = 0;
    std::size_t deallocations = 0;
    std::size_t live = 0;
    std::size_t peak = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++allocations;
        live += bytes;
        peak = live > peak ? live : peak;
        return upstream_->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        ++deallocations;
        live -= bytes;
        upstream_->deallocate(p, bytes, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::pmr::memory_resource* upstream_;
};

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_JSON_SCHEMA_COUNTING_RESOURCE_HPP
