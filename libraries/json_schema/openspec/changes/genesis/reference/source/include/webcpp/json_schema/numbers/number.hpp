// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The number type of a schema: the concept a type models to be one, the customization point it
 models it through, and the exact decimal form every comparison and `multipleOf` work on.
*/
#ifndef WEBCPP_JSON_SCHEMA_NUMBERS_NUMBER_HPP
#define WEBCPP_JSON_SCHEMA_NUMBERS_NUMBER_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/system/result.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace webcpp::json_schema {

/**
 An unsigned integer of 128 bits, as two halves, which every compiler has.

 MSVC has no 128-bit integer type, so the decomposition of a number carries its coefficient this
 way.

 @see "Numbers", in the guide.
*/
struct uint128 {
    /** The upper 64 bits. */
    std::uint64_t high = 0;

    /** The lower 64 bits. */
    std::uint64_t low = 0;

    /**
     Whether two values are equal.

     @return `true` when both halves are equal.
    */
    friend constexpr bool operator==(const uint128&, const uint128&) noexcept = default;
};

/**
 A finite number as a sign, a coefficient and a power of ten: its magnitude is `coefficient *
 10^exponent`.

 One number has many decompositions, `{false, 1, 0}` and `{false, 10, -1}` among them, so two are
 compared as numbers with the library's comparison, never member by member. A `decimal64_t`'s
 coefficient fits in `low`; a `decimal128_t`'s needs 113 bits.

 @see "Numbers", in the guide.
*/
struct decomposed {
    /** Whether the number is negative; a zero may carry either sign, and is zero either way. */
    bool negative = false;

    /** The coefficient, an unsigned integer. */
    uint128 coefficient{};

    /** The power of ten the coefficient is multiplied by. */
    std::int32_t exponent = 0;

    /**
     Whether two decompositions are the same, member by member.

     @return `true` when the sign, the coefficient and the exponent are all equal; two equal
     numbers may still differ here.
    */
    friend constexpr bool operator==(const decomposed&, const decomposed&) noexcept = default;
};

/**
 The customization point through which a type is a schema's number.

 A specialization provides four static member functions: `from_chars`, which reads a JSON
 number's text into a `boost::system::result<N>`, refusing a number the type cannot hold;
 `decompose`, which gives the value's @ref decomposed form exactly; `is_integer`; and `to_chars`,
 which writes the value into a `std::span<char>` and returns how many characters it wrote, or 0
 when they do not fit. `numbers/decimal.hpp` specializes it for `boost::decimal::decimal64_t` and
 `decimal128_t`. No floating-point specialization ships: a binary type cannot hold `0.1`.

 @tparam N The type.
 @see "Numbers", in the guide.
*/
template <class N>
struct number_traits;

/**
 A type that can hold a schema's numbers: regular, totally ordered, constructible from the JSON
 integers, and with a specialization of @ref number_traits.

 @tparam N The type.
 @see "Numbers", in the guide.
*/
template <class N>
concept number =
    std::regular<N> && std::totally_ordered<N> && std::constructible_from<N, std::int64_t> &&
    std::constructible_from<N, std::uint64_t> && requires(N n, std::string_view text) {
        { number_traits<N>::from_chars(text) } -> std::same_as<boost::system::result<N>>;
        { number_traits<N>::decompose(n) } -> std::same_as<decomposed>;
        { number_traits<N>::is_integer(n) } -> std::same_as<bool>;
        { number_traits<N>::to_chars(std::span<char>{}, n) } -> std::same_as<std::size_t>;
    };

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_NUMBERS_NUMBER_HPP
