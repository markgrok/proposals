// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The stack of an iterative walk, whose first elements live inline, so that a
 shallow walk allocates nothing.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_MEMORY_INLINE_STACK_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_MEMORY_INLINE_STACK_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/container/small_vector.hpp>

#include <cstddef>
#include <memory_resource>

namespace webcpp::json_schema::detail {

/** A stack whose first N elements live inline, then in a std::pmr resource. */
template <class T, std::size_t N>
using inline_stack = boost::container::small_vector<T, N, std::pmr::polymorphic_allocator<T>>;

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_MEMORY_INLINE_STACK_HPP
