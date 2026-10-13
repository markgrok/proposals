// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The whole library: include this one header to use json_schema.

 It includes every public header but `<webcpp/json_schema/output/locale.hpp>`,
 the messages of Boost.Locale, which a program includes itself, since it links
 the compiled Boost.Locale and needs exceptions.

 @see "Introduction", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_HPP
#define WEBCPP_JSON_SCHEMA_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <webcpp/json_schema/dialects/dialect.hpp>

#include <webcpp/json_schema/formats/format_set.hpp>

#include <webcpp/json_schema/ir/document.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>
#include <webcpp/json_schema/ir/number.hpp>

#include <webcpp/json_schema/json/parse.hpp>

#include <webcpp/json_schema/numbers/decimal.hpp>
#include <webcpp/json_schema/numbers/number.hpp>

#include <webcpp/json_schema/output/error.hpp>
#include <webcpp/json_schema/output/message_hook.hpp>

#include <webcpp/json_schema/regex/regex_policy.hpp>
#include <webcpp/json_schema/regex/webcpp_regex.hpp>

#include <webcpp/json_schema/registry/registry.hpp>
#include <webcpp/json_schema/registry/resolver.hpp>

#include <webcpp/json_schema/validator/options.hpp>
#include <webcpp/json_schema/validator/validator.hpp>

#include <webcpp/json_schema/vocabulary/vocabulary.hpp>

#endif  // WEBCPP_JSON_SCHEMA_HPP
