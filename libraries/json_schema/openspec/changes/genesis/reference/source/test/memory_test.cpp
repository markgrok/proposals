// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/memory/inline_stack.hpp>
#include <webcpp/json_schema/detail/memory/resource.hpp>

#include "counting_resource.hpp"
#include "require.hpp"

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <memory_resource>

namespace js = webcpp::json_schema;

namespace {

/**
 The storage of a tight_allocator, aligned to 4096, with the block it gave last, the block it was
 given back last, and how many blocks are live.
*/
struct alignas(4096) arena {
    std::array<unsigned char, 65536> bytes{};
    std::size_t used = 0;
    unsigned char* given = nullptr;
    std::size_t given_bytes = 0;
    unsigned char* returned = nullptr;
    std::size_t returned_bytes = 0;
    std::size_t live = 0;
};

/**
 An allocator that aligns its blocks to alignof(T) and never to more: each starts at an odd
 multiple of alignof(T) in its arena, which it never reuses.
*/
template <class T>
class tight_allocator {
public:
    using value_type = T;

    explicit tight_allocator(arena* storage) noexcept : arena_(storage) {}

    template <class U>
    explicit tight_allocator(const tight_allocator<U>& other) noexcept : arena_(other.arena_) {}

    T* allocate(std::size_t n) {
        std::size_t offset = (arena_->used + alignof(T) - 1) / alignof(T) * alignof(T);
        if ((offset / alignof(T)) % 2 == 0) {
            offset += alignof(T);
        }
        const std::size_t bytes = n * sizeof(T);
        webcpp::test::require(BOOST_TEST_LE(offset + bytes, arena_->bytes.size()));
        arena_->used = offset + bytes;
        arena_->given = arena_->bytes.data() + offset;
        arena_->given_bytes = bytes;
        ++arena_->live;
        return reinterpret_cast<T*>(arena_->given);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        arena_->returned = reinterpret_cast<unsigned char*>(p);
        arena_->returned_bytes = n * sizeof(T);
        --arena_->live;
    }

    template <class U>
    bool operator==(const tight_allocator<U>& other) const noexcept {
        return arena_ == other.arena_;
    }

private:
    template <class U>
    friend class tight_allocator;

    arena* arena_;
};

void a_pmr_allocator_gives_its_own_resource() {
    webcpp::test::counting_resource counted;
    const std::pmr::polymorphic_allocator<std::byte> pmr(&counted);
    js::detail::resource_of<std::pmr::polymorphic_allocator<std::byte>> direct(pmr);
    BOOST_TEST(&direct.get() == &counted);
}

// Every block the adapter returns is aligned as asked, lies within the block it took from the
// allocator, and goes back to the allocator whole, whatever the alignment.
void an_adapter_meets_every_alignment_within_its_block() {
    const auto storage = std::make_unique<arena>();
    js::detail::resource_of<tight_allocator<std::byte>> adapted{
        tight_allocator<std::byte>(storage.get())};
    constexpr std::size_t fundamental = alignof(std::max_align_t);
    constexpr std::array<std::size_t, 9> alignments{1, 2, 4, 8, fundamental, 16, 32, 64, 4096};
    constexpr std::array<std::size_t, 6> sizes{1, 7, 8, 9, 64, 65};
    for (const std::size_t alignment : alignments) {
        for (const std::size_t size : sizes) {
            void* p = adapted.get().allocate(size, alignment);
            const auto* first = static_cast<const unsigned char*>(p);
            BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0U);
            BOOST_TEST_GE(storage->given_bytes, size);
            BOOST_TEST(first >= storage->given);
            BOOST_TEST(first + size <= storage->given + storage->given_bytes);
            std::memset(p, 0xA5, size);
            adapted.get().deallocate(p, size, alignment);
            BOOST_TEST(storage->returned == storage->given);
            BOOST_TEST_EQ(storage->returned_bytes, storage->given_bytes);
        }
    }
    BOOST_TEST_EQ(storage->live, 0U);
}

void std_allocator_meets_an_extended_alignment() {
    js::detail::resource_of<std::allocator<std::byte>> adapted{std::allocator<std::byte>{}};
    for (const std::size_t alignment : std::array<std::size_t, 2>{64, 4096}) {
        std::array<void*, 32> blocks{};
        for (void*& p : blocks) {
            p = adapted.get().allocate(40, alignment);
            BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0U);
        }
        for (void* p : blocks) {
            adapted.get().deallocate(p, 40, alignment);
        }
    }
}

void an_inline_stack_spills_into_its_resource() {
    webcpp::test::counting_resource counted;
    js::detail::inline_stack<int, 4> stack{std::pmr::polymorphic_allocator<int>(&counted)};
    for (int i = 0; i < 4; ++i) {
        stack.push_back(i);
    }
    BOOST_TEST_EQ(counted.allocations, 0U);
    stack.push_back(4);
    BOOST_TEST_EQ(counted.allocations, 1U);
}

}  // namespace

int main() {
    a_pmr_allocator_gives_its_own_resource();
    an_adapter_meets_every_alignment_within_its_block();
    std_allocator_meets_an_extended_alignment();
    an_inline_stack_spills_into_its_resource();
    return boost::report_errors();
}
