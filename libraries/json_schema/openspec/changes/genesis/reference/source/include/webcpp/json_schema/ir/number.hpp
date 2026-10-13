// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 A number operand of the IR, held exactly: an integer stays an integer.
*/
#ifndef WEBCPP_JSON_SCHEMA_IR_NUMBER_HPP
#define WEBCPP_JSON_SCHEMA_IR_NUMBER_HPP

#include <webcpp/json_schema/config.hpp>

#include <cstdint>
#include <variant>

namespace webcpp::json_schema::ir {

/**
 A number operand, exact: an integer that fits stays `std::int64_t` or `std::uint64_t`, and any
 other number is a `Number`.

 A bound, `multipleOf`'s divisor and a count are each one. Comparisons and `multipleOf` read it
 through its exact decomposition, so an integer is never rounded to `Number`.

 @tparam Number The schema's number type, a model of @ref webcpp::json_schema::number.
 @see "Numbers", in the guide.
*/
template <class Number>
using number = std::variant<std::int64_t, std::uint64_t, Number>;

}  // namespace webcpp::json_schema::ir

#endif  // WEBCPP_JSON_SCHEMA_IR_NUMBER_HPP
