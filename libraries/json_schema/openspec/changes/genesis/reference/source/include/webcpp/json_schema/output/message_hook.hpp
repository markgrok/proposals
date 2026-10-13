// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The message hook: what a validation calls to write an error's message, in place of the English
 templates.

 @see "Output", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_OUTPUT_MESSAGE_HOOK_HPP
#define WEBCPP_JSON_SCHEMA_OUTPUT_MESSAGE_HOOK_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/output/error.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <boost/compat/function_ref.hpp>

namespace webcpp::json_schema {

/**
 What a validation calls to write an error's message, in place of the English templates.

 The hook appends the message of `e` to `out`, a string of the traits' allocator, so that the text
 lands in the validation's allocator. It refers to the callable it was made from, which must
 outlive every use of the options that hold it.

 @tparam Traits The traits, whose allocator the string uses.
 @see "Output", in the guide.
*/
template <class Traits>
using basic_message_hook =
    boost::compat::function_ref<void(const error& e, basic_string<Traits>& out)>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_OUTPUT_MESSAGE_HOOK_HPP
