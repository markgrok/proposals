// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The `std::pmr::memory_resource` of an allocator, through which json_schema
 allocates whatever allocator a program gives it.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_MEMORY_RESOURCE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_MEMORY_RESOURCE_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/throw_exception.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <memory_resource>
#include <new>
#include <type_traits>

namespace webcpp::json_schema::detail {

/**
 A `std::pmr::memory_resource` that allocates through an allocator whose pointer is a raw pointer.

 It serves every alignment, as Boost.Container's `resource_adaptor_imp` does: an alignment up to
 `alignof(std::max_align_t)` takes the bytes asked for, and a larger one takes `alignment - 1 +
 sizeof(void*)` more, returns the first address within them aligned as asked with room for a
 pointer before it, and keeps there the address of the block the allocator gave, to give it back.

 Tip: it differs from `resource_adaptor_imp` where these rules require. It is a
 `std::pmr::memory_resource`, which json_schema and Boost.JSON's adapter take. It allocates whole
 slots of `std::max_align_t` rather than bytes, since an allocator of `std::byte` aligns only to 1.
 A size whose slots would overflow `std::size_t` raises `std::bad_array_new_length` through
 `boost::throw_exception`, as `allocate` raises what it cannot allocate, where
 `resource_adaptor_imp` would wrap it into a small block. Its allocator's pointer must be a raw
 pointer, which a `void*` can hold; a fancy pointer is refused by name.
*/
template <class Allocator>
class allocator_resource final : public std::pmr::memory_resource {
public:
    /** Allocates through a copy of `a`. */
    explicit allocator_resource(const Allocator& a) noexcept : allocator_(a) {}

private:
    using slots = std::allocator_traits<Allocator>::template rebind_alloc<std::max_align_t>;
    using traits = std::allocator_traits<slots>;

    static_assert(std::is_pointer_v<typename traits::pointer>,
                  "json_schema adapts only an allocator whose pointer is a raw pointer");

    /** The bytes an alignment beyond a slot's adds: room to align, and the block's address. */
    static constexpr std::size_t extra(std::size_t alignment) noexcept {
        return alignment <= alignof(std::max_align_t) ? 0 : alignment - 1 + sizeof(void*);
    }

    /** The slots that hold `size` bytes and `extra` more; refuses a count that overflows. */
    static std::size_t count(std::size_t size, std::size_t extra) {
        constexpr std::size_t slot = sizeof(std::max_align_t);
        if (size > std::numeric_limits<std::size_t>::max() - extra - (slot - 1)) {
            boost::throw_exception(std::bad_array_new_length());
        }
        return (size + extra + slot - 1) / slot;
    }

    void* do_allocate(std::size_t size, std::size_t alignment) override {
        const std::size_t room = extra(alignment);
        const std::size_t n = count(size, room);
        slots a(allocator_);
        void* const block = traits::allocate(a, n);
        if (room == 0) {
            return block;
        }
        const std::size_t past =
            (reinterpret_cast<std::uintptr_t>(block) + sizeof(void*)) % alignment;
        const std::size_t offset = sizeof(void*) + (past == 0 ? 0 : alignment - past);
        unsigned char* const aligned = static_cast<unsigned char*>(block) + offset;
        std::memcpy(aligned - sizeof(void*), static_cast<const void*>(&block), sizeof(block));
        return aligned;
    }

    void do_deallocate(void* p, std::size_t size, std::size_t alignment) override {
        const std::size_t room = extra(alignment);
        void* block = p;
        if (room != 0) {
            std::memcpy(static_cast<void*>(&block), static_cast<unsigned char*>(p) - sizeof(void*),
                        sizeof(block));
        }
        slots a(allocator_);
        traits::deallocate(a, static_cast<std::max_align_t*>(block), count(size, room));
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    Allocator allocator_;
};

/**
 A `std::pmr::memory_resource` for an allocator of `std::byte`: a pmr allocator's
 own, or an @ref allocator_resource over any other, whose pointer is a raw pointer.

 It is not copyable: the resource it gives may be its own member.
*/
template <class Allocator>
class resource_of {
public:
    /** Adapts a copy of `a`. */
    explicit resource_of(const Allocator& a) noexcept : adapter_(a) {}

    resource_of(const resource_of&) = delete;
    resource_of& operator=(const resource_of&) = delete;

    /** The resource, which lives as long as this object. */
    std::pmr::memory_resource& get() noexcept { return adapter_; }

private:
    allocator_resource<Allocator> adapter_;
};

/** The resource of a pmr allocator, which is its own: no adapter. */
template <class T>
class resource_of<std::pmr::polymorphic_allocator<T>> {
public:
    /** Takes the resource of `a`. */
    explicit resource_of(const std::pmr::polymorphic_allocator<T>& a) noexcept
        : resource_(a.resource()) {}

    resource_of(const resource_of&) = delete;
    resource_of& operator=(const resource_of&) = delete;

    /** The allocator's resource. */
    std::pmr::memory_resource& get() noexcept { return *resource_; }

private:
    std::pmr::memory_resource* resource_;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_MEMORY_RESOURCE_HPP
