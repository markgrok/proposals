// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/json/json_resource.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace js = webcpp::json_schema;

namespace {

void values_allocate_through_the_adapter() {
    webcpp::test::counting_resource counted;
    js::detail::json_resource adapter(counted);
    {
        const auto v = boost::json::parse(R"({"a": [1, 2, 3], "b": "a string long enough"})",
                                          boost::json::storage_ptr(&adapter));
        BOOST_TEST(counted.allocations > 0);
    }
    BOOST_TEST_EQ(counted.allocations, counted.deallocations);
    BOOST_TEST_EQ(counted.live, 0U);
}

// Every request reaches the upstream resource with its own size and alignment, and every block
// comes back with them.
void requests_pass_whole() {
    webcpp::test::recording_resource recorded;
    js::detail::json_resource adapter(recorded);
    {
        boost::json::value v = boost::json::parse(
            R"({"a": [1, 2, 3], "b": "a string long enough", "c": {"d": [[], {}]}})",
            boost::json::storage_ptr(&adapter));
        for (int i = 0; i < 100; ++i) {
            v.as_object()["a"].as_array().push_back(i);
        }
    }
    constexpr std::size_t fundamental = alignof(std::max_align_t);
    constexpr std::array<std::size_t, 7> alignments{1, 2, 8, fundamental, 16, 64, 4096};
    for (const std::size_t alignment : alignments) {
        for (const std::size_t size : {std::size_t{1}, std::size_t{24}, std::size_t{5000}}) {
            const std::size_t before = recorded.bytes;
            void* p = adapter.allocate(size, alignment);
            BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0U);
            BOOST_TEST_EQ(recorded.bytes - before, size);
            adapter.deallocate(p, size, alignment);
        }
    }
    BOOST_TEST(recorded.allocations > 0);
    BOOST_TEST_EQ(recorded.live_blocks(), 0U);
    BOOST_TEST_EQ(recorded.mismatched, 0U);
    BOOST_TEST_EQ(recorded.misaligned, 0U);
}

// An adapter is equal to itself alone: telling another adapter apart from another resource would
// need RTTI.
void an_adapter_is_equal_to_itself() {
    webcpp::test::counting_resource counted;
    const js::detail::json_resource one(counted);
    const js::detail::json_resource two(counted);
    BOOST_TEST(one.is_equal(one));
    BOOST_TEST(!one.is_equal(two));
}

}  // namespace

int main() {
    values_allocate_through_the_adapter();
    requests_pass_whole();
    an_adapter_is_equal_to_itself();
    return boost::report_errors();
}
