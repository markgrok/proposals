// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The default regex policy, over webcpp::regex: the only header of json_schema that includes it.

 @see "Regular expressions", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_REGEX_WEBCPP_REGEX_HPP
#define WEBCPP_JSON_SCHEMA_REGEX_WEBCPP_REGEX_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/memory/resource.hpp>
#include <webcpp/json_schema/regex/regex_policy.hpp>

#include <webcpp/regex.hpp>

#include <boost/system/result.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <string_view>

namespace webcpp::json_schema {

/**
 The default regex policy: webcpp::regex, whose default backend is SRELL, compiling in the schema's
 allocator and searching in the validation's.

 @see "Regular expressions", in the guide.
*/
struct webcpp_regex {
    /**
     A compiled pattern: webcpp::regex's `basic_regex` over the schema's allocator rebound to
     `char`; with the default allocator, `webcpp::regex::regex` itself.

     SRELL takes its program's memory from a resource: a polymorphic allocator's, `operator new`
     for `std::allocator`, and the default resource for any other allocator, as
     `webcpp::regex::honours` says of the rebound allocator.

     @tparam Allocator The schema's allocator.
    */
    template <class Allocator>
    using compiled = webcpp::regex::basic_regex<
        char, typename std::allocator_traits<Allocator>::template rebind_alloc<char>>;

    /**
     Compiles `text` once, in the schema's allocator, with `step_budget`.

     @tparam Allocator The schema's allocator.
     @param text The pattern.
     @param allocator The schema's allocator.
     @param step_budget The step budget of its searches, 0 for the backend's default.
     @return The pattern, or webcpp::regex's error, which the compiler reports as
     `errc::invalid_regex` at the keyword.
    */
    template <class Allocator>
    static boost::system::result<compiled<Allocator>> compile(std::string_view text,
                                                              const Allocator& allocator,
                                                              std::size_t step_budget) {
        using rebound = std::allocator_traits<Allocator>::template rebind_alloc<char>;
        return compiled<Allocator>::compile(
            text, webcpp::regex::options{.step_budget = step_budget}, rebound(allocator));
    }

    /**
     Searches `text`, which Boost.JSON has validated as UTF-8, its state on the stack up to
     `detail::regex_inline_bytes`, then in the validation's allocator.

     Both allocators are deduced from the arguments: `compiled<SchemaAlloc>` names the schema's
     allocator only inside `rebind_alloc`, where it cannot be deduced, so the regex is taken as
     the `basic_regex` it is.

     @tparam RegexAlloc The regex's allocator, the schema's rebound to `char`.
     @tparam ValidationAlloc The validation's allocator.
     @param re The pattern.
     @param text The text, well-formed UTF-8.
     @param allocator The validation's allocator.
     @return Found, not found, or a limit for a runaway search.
    */
    template <class RegexAlloc, class ValidationAlloc>
    static match_result search(const webcpp::regex::basic_regex<char, RegexAlloc>& re,
                               std::string_view text, const ValidationAlloc& allocator) {
        detail::resource_of<ValidationAlloc> upstream(allocator);
        // Uninitialized on purpose: the monotonic resource writes before it reads.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        std::array<std::byte, detail::regex_inline_bytes> buffer;
        std::pmr::monotonic_buffer_resource local(buffer.data(), buffer.size(), &upstream.get());
        webcpp::regex::cmatch m{std::pmr::polymorphic_allocator<webcpp::regex::csub_match>(&local)};
        const auto found =
            webcpp::regex::search(text, m, re, webcpp::regex::regex_constants::match_valid_utf8);
        if (found.has_error()) {
            // error::complexity or error::stack: a search over valid UTF-8 has no other error.
            return match_result::limit;
        }
        return *found ? match_result::found : match_result::not_found;
    }
};

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_REGEX_WEBCPP_REGEX_HPP
