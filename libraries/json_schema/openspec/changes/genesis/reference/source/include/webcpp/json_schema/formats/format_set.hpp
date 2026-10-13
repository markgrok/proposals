// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The format set: the checks `format` calls, each under the name a schema gives it.

 @see "Formats", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_FORMATS_FORMAT_SET_HPP
#define WEBCPP_JSON_SCHEMA_FORMATS_FORMAT_SET_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/compat/function_ref.hpp>

#include <span>
#include <string_view>

namespace webcpp::json_schema {

/**
 One format: its name, as `format` names it, and the check of a string.

 The check must outlive every schema compiled with a set that holds it.

 @see "Formats", in the guide.
*/
struct format_entry {
    /** The format's name, for example `date-time`. */
    std::string_view name{};

    /** Whether a string, well-formed UTF-8, is of the format. */
    boost::compat::function_ref<bool(std::string_view) noexcept> check;
};

/**
 The formats a compile knows, which the user extends or replaces.

 When `format` asserts, a string is checked by the entry of its name, other types pass, and a
 failure is an error of kind `error_kind::format`; a name the set does not hold is an annotation
 only. The entries must outlive every schema compiled with them.

 @see "Formats", in the guide.
*/
using format_set = std::span<const format_entry>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_FORMATS_FORMAT_SET_HPP
