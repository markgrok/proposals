// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/memory/resource.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>

namespace js = webcpp::json_schema;

// A size whose slots, and whose room for an extended alignment, overflow std::size_t is refused as
// memory_resource::allocate refuses what it cannot allocate, never wrapped into a small block.
int main() {
    js::detail::resource_of<std::allocator<std::byte>> adapted{std::allocator<std::byte>{}};
    // Read through a volatile, so that no compiler knows the size: GCC refuses a call whose size it
    // knows exceeds the largest object (-Walloc-size-larger-than), which is this test's point.
    const volatile std::size_t unknown = std::numeric_limits<std::size_t>::max() - 3;
    const std::size_t huge = unknown;
    int refused = 0;
    for (const std::size_t alignment : std::array<std::size_t, 2>{alignof(std::max_align_t), 64}) {
        try {
            void* p = adapted.get().allocate(huge, alignment);
            BOOST_ERROR("an allocation of SIZE_MAX - 3 bytes returned");
            adapted.get().deallocate(p, huge, alignment);
        } catch (const std::bad_alloc&) {
            ++refused;
        }
    }
    BOOST_TEST_EQ(refused, 2);
    return boost::report_errors();
}
