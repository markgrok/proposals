// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The configuration of json_schema, which each of its headers includes first:
 whether json_schema is built without exceptions.

 @see "Errors and exceptions", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_CONFIG_HPP
#define WEBCPP_JSON_SCHEMA_CONFIG_HPP

#include <boost/config.hpp>

// MrDocs parses json_schema as a build with exceptions does, where the macro is
// not defined: this definition, undone at once, is the one the reference lists,
// and the parse goes on with every API that throws in it.
#ifdef __MRDOCS__
/**
 Defined when json_schema is built without exceptions: automatically when the
 compiler has none (`BOOST_NO_EXCEPTIONS`), or by the developer to disable them
 in a build that has them.

 While it is defined, `basic_validator::compile_or_throw` does not exist and
 `<webcpp/json_schema/output/locale.hpp>` refuses to compile, naming
 Boost.Locale. Every other operation reports its failure as a value, in both
 modes.

 @see "Errors and exceptions", in the guide.
*/
#define WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
#undef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
#endif

#if defined(BOOST_NO_EXCEPTIONS) && !defined(WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS)
#define WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
#endif

#endif  // WEBCPP_JSON_SCHEMA_CONFIG_HPP
