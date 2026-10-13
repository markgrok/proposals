// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Where the JSON values json_schema owns live: an arena that frees nothing until it dies, and that
 Boost.JSON knows frees nothing, so that a value in it, however deep, is destroyed without a visit
 to its elements and so without recursion.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_VALUE_ARENA_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_VALUE_ARENA_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/memory/resource.hpp>

#include <boost/container/pmr/memory_resource.hpp>
#include <boost/json/is_deallocate_trivial.hpp>
#include <boost/json/storage_ptr.hpp>

#include <cstddef>
#include <memory_resource>

namespace webcpp::json_schema::detail {

/**
 Boost.JSON's memory resource over a `std::pmr::monotonic_buffer_resource`: it deallocates nothing,
 and `boost::json::is_deallocate_trivial` says so.

 A `boost::json::storage_ptr` made from its address carries that, and an array, an object or a
 string in that storage then skips its elements' destructors (`array::destroy`, `object::~object`):
 a value dies in constant time, whatever its depth. The monotonic resource gives its blocks back to
 its upstream when it dies, each with the size and alignment it took.

 Tip: Boost.JSON's own `monotonic_resource` would carry the same flag, but Boost 1.92's has two
 defects. It gives each block back `sizeof(block)` bytes shorter than it took
 (`impl/monotonic_resource.ipp`, `release`), which an allocator that reads the size, such as
 json_schema's adapter, cannot take; fixed after 1.92, boostorg/json#1179, commit af039520. And it
 sizes a fresh block from the request's size alone, so an over-aligned request whose size and
 padding do not fit that block is answered with a null pointer (`do_allocate`, behind a
 `BOOST_ASSERT`): `allocate(1000, 64)` and `allocate(4096, 4096)` on a fresh resource; open,
 boostorg/json#1191. The standard's monotonic resource has neither.
*/
class arena_resource final : public boost::container::pmr::memory_resource {
public:
    /** Allocates its blocks from `upstream`, which must outlive this resource. */
    explicit arena_resource(std::pmr::memory_resource& upstream) : monotonic_(&upstream) {}

    arena_resource(const arena_resource&) = delete;
    arena_resource& operator=(const arena_resource&) = delete;

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        return monotonic_.allocate(bytes, alignment);
    }

    void do_deallocate(void* /*p*/, std::size_t /*bytes*/, std::size_t /*alignment*/) override {}

    [[nodiscard]] bool do_is_equal(
        const boost::container::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

private:
    std::pmr::monotonic_buffer_resource monotonic_;
};

}  // namespace webcpp::json_schema::detail

/** Tells Boost.JSON that an @ref webcpp::json_schema::detail::arena_resource frees nothing. */
template <>
struct boost::json::is_deallocate_trivial<webcpp::json_schema::detail::arena_resource> {
    /** True: deallocation does nothing. */
    static constexpr bool value = true;
};

namespace webcpp::json_schema::detail {

/**
 The JSON values json_schema owns: their storage, an @ref arena_resource over the resource of an
 allocator, which frees them all at once when the arena dies.

 It is neither copyable nor movable, since its storage is its own member and a value points to it;
 it is made with `std::allocate_shared` through its allocator, so that its address never moves.
 Every value in it must die before it, as a value whose storage is gone would; its destruction,
 trivial, touches nothing.
*/
template <class Allocator>
class value_arena {
public:
    /** An empty arena, which allocates through a copy of `a`. */
    explicit value_arena(const Allocator& a) : upstream_(a), arena_(upstream_.get()) {}

    value_arena(const value_arena&) = delete;
    value_arena& operator=(const value_arena&) = delete;

    /** The storage of the values: it does not own the arena, and it deallocates trivially. */
    boost::json::storage_ptr storage() noexcept { return boost::json::storage_ptr(&arena_); }

    /** The allocator's resource, which the arena takes its blocks from. */
    std::pmr::memory_resource& resource() noexcept { return upstream_.get(); }

private:
    resource_of<Allocator> upstream_;
    arena_resource arena_;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_VALUE_ARENA_HPP
