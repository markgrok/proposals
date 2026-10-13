// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The traits of a validator: the number type of its schemas, its allocator and its regex policy,
 the defaults, and the concept a traits type models.

 @see "Customization", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_TRAITS_HPP
#define WEBCPP_JSON_SCHEMA_TRAITS_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/numbers/decimal.hpp>
#include <webcpp/json_schema/numbers/number.hpp>
#include <webcpp/json_schema/regex/regex_policy.hpp>
#include <webcpp/json_schema/regex/webcpp_regex.hpp>

#include <boost/decimal/decimal64_t.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <string>

namespace webcpp::json_schema {

/**
 The default traits: `decimal64_t` numbers, the polymorphic allocator of `std::byte`, and
 webcpp::regex.

 A program changes one of them by deriving from it and naming the member again, as in
 `struct wide : js::default_traits { using number_type = boost::decimal::decimal128_t; };`.

 @see "Customization", in the guide.
*/
struct default_traits {
    /** The number type of a schema's numbers that no `std::int64_t` or `std::uint64_t` holds. */
    using number_type = boost::decimal::decimal64_t;
    /** The allocator of everything a schema and a validation allocate. */
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    /** The regex policy of `pattern` and `patternProperties`. */
    using regex_policy = webcpp_regex;
};

/**
 A traits type: a `number_type` that models @ref number, an `allocator_type` of `std::byte`, and a
 `regex_policy` that compiles and searches in that allocator.

 The allocator is checked first, so that a type which is no allocator of `std::byte` is refused
 before the regex policy rebinds it.

 @tparam T The traits type.
 @see "Customization", in the guide.
*/
template <class T>
concept traits =
    std::same_as<typename T::allocator_type::value_type, std::byte> &&
    number<typename T::number_type> &&
    regex_policy<typename T::regex_policy, typename T::allocator_type, typename T::allocator_type>;

/**
 The string of the traits' allocator: `std::pmr::string` with the default traits.

 @tparam Traits The traits.
*/
template <class Traits>
using basic_string = std::basic_string<
    char, std::char_traits<char>,
    typename std::allocator_traits<typename Traits::allocator_type>::template rebind_alloc<char>>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_TRAITS_HPP
