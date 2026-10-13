// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 What a regex policy provides: a pattern compiled once, in the schema's allocator, with a step
 budget, and searched in the validation's.

 @see "Regular expressions", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_REGEX_REGEX_POLICY_HPP
#define WEBCPP_JSON_SCHEMA_REGEX_REGEX_POLICY_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/system/result.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace webcpp::json_schema {

/**
 What a search answers: found, not found, or stopped by the step budget or the backtracking stack.

 @see "Regular expressions", in the guide.
*/
enum class match_result : std::uint8_t {
    /** The pattern matches somewhere in the text. */
    found,
    /** It matches nowhere. */
    not_found,
    /** A runaway search, stopped: the keyword fails with the limit's error. */
    limit,
};

/**
 A regex policy: `R::compile(text, schema_allocator, step_budget)` compiles an ECMA-262 pattern in
 `u` mode, unanchored, over UTF-8, into the schema's allocator, and `R::search(re, text,
 validation_allocator)` searches well-formed UTF-8, its state in the validation's allocator.

 @tparam R The policy.
 @tparam SchemaAlloc The schema's allocator.
 @tparam ValidationAlloc The validation's allocator.
 @see "Regular expressions", in the guide.
*/
template <class R, class SchemaAlloc, class ValidationAlloc>
concept regex_policy =
    requires(std::string_view text, const SchemaAlloc& schema_allocator,
             const ValidationAlloc& validation_allocator, std::size_t step_budget,
             const R::template compiled<SchemaAlloc>& re) {
        {
            R::compile(text, schema_allocator, step_budget)
        } -> std::same_as<boost::system::result<typename R::template compiled<SchemaAlloc>>>;
        { R::search(re, text, validation_allocator) } -> std::same_as<match_result>;
    };

namespace detail {

/** The bytes of a search's state kept on the stack before it reaches the validation's allocator;
    measured at its definition (Limits). */
inline constexpr std::size_t regex_inline_bytes = 4096;

}  // namespace detail

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_REGEX_REGEX_POLICY_HPP
