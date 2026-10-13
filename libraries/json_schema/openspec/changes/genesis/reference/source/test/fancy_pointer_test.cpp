// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Tests that the adapter refuses an allocator whose pointer is a class, as Boost.Interprocess's
// offset_ptr is, with its own message, which clang's -verify finds, and not deep inside the
// template. The expectation is longer than a line, which clang-format would otherwise break.

#include <webcpp/json_schema/detail/memory/resource.hpp>

#include <cstddef>
#include <memory>

namespace {

/** A pointer that is a class, which the adapter cannot return as a void*. */
template <class T>
class fancy_pointer {
public:
    using element_type = T;

    explicit fancy_pointer(T* p) noexcept : p_(p) {}

private:
    T* p_;
};

/** An allocator whose pointer is a fancy_pointer. */
template <class T>
class fancy_allocator {
public:
    using value_type = T;
    using pointer = fancy_pointer<T>;

    fancy_allocator() noexcept = default;

    template <class U>
    fancy_allocator(const fancy_allocator<U>& /*other*/) noexcept {}

    pointer allocate(std::size_t n) { return pointer(std::allocator<T>{}.allocate(n)); }

    void deallocate(pointer /*p*/, std::size_t /*n*/) noexcept {}
};

}  // namespace

// clang-format off
// expected-error@webcpp/json_schema/detail/memory/resource.hpp:* {{json_schema adapts only an allocator whose pointer is a raw pointer}}
// clang-format on
int main() {
    webcpp::json_schema::detail::resource_of<fancy_allocator<std::byte>> adapted{
        fancy_allocator<std::byte>{}};
    (void)adapted.get();
}
