// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/json/deep_copy.hpp>
#include <webcpp/json_schema/detail/json/equal.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/value.hpp>

#include <memory_resource>
#include <new>

namespace js = webcpp::json_schema;

namespace {

// An array wider than both inline stacks, so that equal and deep_copy must allocate in scratch.
boost::json::value wide() {
    boost::json::array elements;
    for (int i = 0; i < 100; ++i) {
        elements.push_back(i);
    }
    return elements;
}

}  // namespace

// An allocation that scratch cannot make reaches the caller of equal as it reaches the caller of
// deep_copy: as the std::bad_alloc the resource raises, which a build with exceptions catches.
// Built without exceptions, the test calls one of them, which must end the program before main
// returns (the Jamfile runs it as a test that fails): WEBCPP_TEST_JSON_SCHEMA_CALL is 1 for equal,
// 2 for deep_copy.
int main() {
    const boost::json::value v = wide();
    boost::json::monotonic_resource out;
    std::pmr::memory_resource& none = *std::pmr::null_memory_resource();
#ifdef WEBCPP_TEST_JSON_SCHEMA_CALL
    if (WEBCPP_TEST_JSON_SCHEMA_CALL == 1) {
        static_cast<void>(js::detail::equal(v, v, none));
    } else {
        static_cast<void>(js::detail::deep_copy(v, &out, none));
    }
    return 0;
#else
    int refused = 0;
    try {
        static_cast<void>(js::detail::equal(v, v, none));
        BOOST_ERROR("equal returned without the memory it needs");
    } catch (const std::bad_alloc&) {
        ++refused;
    }
    try {
        static_cast<void>(js::detail::deep_copy(v, &out, none));
        BOOST_ERROR("deep_copy returned without the memory it needs");
    } catch (const std::bad_alloc&) {
        ++refused;
    }
    BOOST_TEST_EQ(refused, 2);
    return boost::report_errors();
#endif
}
