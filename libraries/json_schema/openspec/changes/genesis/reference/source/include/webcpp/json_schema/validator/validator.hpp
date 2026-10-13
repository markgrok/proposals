// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The validator: a set of dialects, as a type, which loads a schema into the IR.

 @see "Loading a schema", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_VALIDATOR_VALIDATOR_HPP
#define WEBCPP_JSON_SCHEMA_VALIDATOR_VALIDATOR_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/dialects/draft2020_12.hpp>
#include <webcpp/json_schema/detail/json/exact_handler.hpp>
#include <webcpp/json_schema/detail/loader/walk.hpp>
#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/ir/document.hpp>
#include <webcpp/json_schema/json/parse.hpp>
#include <webcpp/json_schema/traits.hpp>
#include <webcpp/json_schema/validator/options.hpp>

#include <boost/json/value.hpp>

#include <array>
#include <cstddef>

namespace webcpp::json_schema {

/**
 A validator of a set of dialects: it loads a schema written in any of them into the IR.

 It holds no state: it is the set of dialects, as a type, and every member is `const`, so one
 validator serves any number of loads, on any number of threads. The first dialect of the set is
 the one a schema without `$schema` is read in, unless the options name another.

 A load reads the schema iteratively, over one copy of it that the document holds, so a schema
 nested however deep is read without recursion up to `max_schema_depth`, and refused past it.

 @tparam Traits The traits of the documents it builds.
 @tparam Dialects The tags of its dialects, at least one, each a @ref dialect_policy.
 @see "Loading a schema", in the guide.
*/
template <class Traits, class... Dialects>
    requires traits<Traits> && (sizeof...(Dialects) > 0) && (dialect_policy<Dialects> && ...)
class basic_validator final {
    static_assert((requires { detail::dialect_machinery<Dialects>::keywords(); } && ...),
                  "a dialect that has_machinery names has its machinery included here");

public:
    /** The traits. */
    using traits_type = Traits;

    /** The allocator of the documents it builds. */
    using allocator_type = Traits::allocator_type;

    /** The document a load builds. */
    using document_type = ir::basic_document<Traits>;

    /** The options of a load. */
    using compile_options = basic_compile_options<Traits>;

    /** Its dialects, in the order of its parameters; the first is the default. */
    static constexpr std::array<dialect, sizeof...(Dialects)> dialects{Dialects::id...};

    /**
     Loads a schema into a document of the IR.

     The root's dialect is its `$schema`, else the options' `default_dialect`, else the first of
     @ref dialects. Every keyword's value is checked for the shape its dialect gives it; a load
     never checks the schema against its meta-schema. A number is read from its double: the
     shortest text that reads back as it.

     Options a load cannot honour yet are refused rather than ignored: a `registry`, a `resolver`,
     a `base_uri`, `format_assertion`, `formats`, `content_assertion`, `vocabularies`, and an
     `unknown` other than `dialect_default` or `annotate`.
     Two defaults are not honoured yet, since a default cannot be refused: with
     `compatibility_keywords` on, `definitions` and `dependencies` still load as unknown keywords,
     as with it off; and `validate_schema` is a compile's, which a load never runs.

     @param schema The schema.
     @param options The options.
     @param allocator The allocator of the document and of the load's scratch.
     @return The document, or the diagnostic of the first fault, a node's keywords read before its
     subschemas and those in document order: among others
     @ref errc::not_a_schema, @ref errc::invalid_keyword_value, @ref errc::unknown_dialect,
     @ref errc::invalid_number, @ref errc::schema_too_deep, and
     @ref errc::unresolvable_reference for a `$ref`, a `$dynamicRef` and an `$id` but the root's
     absolute one, which no load resolves yet; a refused option's `where` is `"options"`.
     @throws std::bad_alloc When the allocator throws it; without exceptions, the program ends.
     @see "Loading a schema", in the guide.
    */
    [[nodiscard]] result<document_type> load(const boost::json::value& schema,
                                             const compile_options& options = compile_options(),
                                             allocator_type allocator = {}) const {
        return walk(schema, nullptr, options, allocator);
    }

    /**
     Loads a schema read by @ref parse, whose numbers are read from their text.

     It does what the other overload does, but for a number whose double denotes another decimal
     than its text: the number is the one its text writes, as @ref parse recorded it, and one that
     the number type cannot hold is @ref errc::invalid_number.

     @param schema The schema, as @ref parse read it.
     @param options The options.
     @param allocator The allocator of the document and of the load's scratch.
     @return The document, or the diagnostic of the first fault.
     @throws std::bad_alloc When the allocator throws it; without exceptions, the program ends.
     @see "Loading a schema", in the guide.
    */
    [[nodiscard]] result<document_type> load(const basic_parsed_json<Traits>& schema,
                                             const compile_options& options = compile_options(),
                                             allocator_type allocator = {}) const {
        return walk(schema.value, &schema.exact_numbers, options, allocator);
    }

private:
    /** Walks `schema`, with the exact numbers of `exact` when there are some. */
    [[nodiscard]] static result<document_type> walk(const boost::json::value& schema,
                                                    const detail::exact_table<Traits>* exact,
                                                    const compile_options& options,
                                                    allocator_type allocator) {
        const std::array<detail::dialect_keywords, sizeof...(Dialects)> tables{
            detail::dialect_keywords{
                .id = Dialects::id,
                .keywords = detail::dialect_machinery<Dialects>::keywords(),
            }...,
        };
        detail::walker<Traits> walker(tables, options, allocator);
        return walker.run(schema, exact);
    }
};

/**
 The validator of a set of dialects, with the default traits.

 @tparam Dialects The tags of its dialects; the first is the default.
 @see "Loading a schema", in the guide.
*/
template <class... Dialects>
using validator_for = basic_validator<default_traits, Dialects...>;

/**
 The validator of every dialect whose keywords json_schema loads, with the default traits: draft
 2020-12.

 @see "Loading a schema", in the guide.
*/
using validator = validator_for<draft2020_12>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_VALIDATOR_VALIDATOR_HPP
