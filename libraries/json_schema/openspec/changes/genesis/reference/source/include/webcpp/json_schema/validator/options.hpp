// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The options of a compile and of a validation, and their depth limits.

 @see "Options", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_VALIDATOR_OPTIONS_HPP
#define WEBCPP_JSON_SCHEMA_VALIDATOR_OPTIONS_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/formats/format_set.hpp>
#include <webcpp/json_schema/output/error.hpp>
#include <webcpp/json_schema/output/message_hook.hpp>
#include <webcpp/json_schema/registry/registry.hpp>
#include <webcpp/json_schema/registry/resolver.hpp>
#include <webcpp/json_schema/traits.hpp>
#include <webcpp/json_schema/vocabulary/vocabulary.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace webcpp::json_schema {

/**
 The deepest nesting of schemas a compile reads by default; a schema nested deeper is
 `errc::schema_too_deep`.

 @note A starting value, not measured yet. The measured value is the largest power of two at which
 loading and compiling a schema nested that deep uses at most 1 MiB of memory on wasip2 at -O0.
 @see "Limits", in the guide.
*/
inline constexpr std::size_t default_max_schema_depth = 1024;

/**
 The deepest nesting of an instance a validation descends into by default; a value nested deeper
 is an error of kind `error_kind::depth_limit`.

 @note A starting value, not measured yet. The measured value is the largest power of two at which
 validating an instance nested that deep against the 2020-12 meta-schema uses at most 1 MiB of
 validation memory on wasip2 at -O0.
 @see "Limits", in the guide.
*/
inline constexpr std::size_t default_max_depth = 1024;

/**
 What a compile does with a keyword that no dialect, vocabulary or user vocabulary it reads knows.

 @see "Options", in the guide.
*/
enum class unknown_keywords : std::uint8_t {
    /**
     As its dialect says: an annotation in 2019-09 and 2020-12, inert in drafts 4 to 7, and
     `errc::unknown_keyword` in OpenAPI 3.0, but for a name that starts with `x-`.
    */
    dialect_default,

    /** An annotation of its value, in every dialect. */
    annotate,

    /** Inert, in every dialect: it neither asserts nor annotates. */
    ignore,

    /** `errc::unknown_keyword`, naming the keyword, in every dialect. */
    reject,
};

/**
 How a schema is compiled: where its references are found, which formats and vocabularies it
 knows, and how strictly it is read.

 A program names the members it sets, as in `js::compile_options{.format_assertion = true}`. The
 objects the options refer to (the registry, the resolver's callable, the formats and the
 vocabularies) must outlive the compile; the formats' and the vocabularies' functions must also
 outlive every schema compiled with them.

 @tparam Traits The traits of the validator that compiles.
 @see "Options", in the guide.
*/
template <traits Traits>
struct basic_compile_options {
    /**
     The user's documents, looked up by references after the document's own resources; a compile
     also keeps in it the meta-schemas it compiles, for later compiles.
    */
    basic_registry<Traits>* registry = nullptr;

    /**
     Asked once for each absolute URI that neither the document, the registry nor the bundled
     meta-schemas know; without it such a URI is `errc::unresolvable_reference`.
    */
    std::optional<resolver_ref> resolver{};

    /** The root's retrieval URI, the base of its relative references; else `urn:webcpp:root`. */
    std::string_view base_uri{};

    /** The dialect of a schema whose root has no `$schema`; else the first of the validator's. */
    std::optional<dialect> default_dialect{};

    /**
     Whether `format` asserts in every dialect and resource, not only where the `format-assertion`
     vocabulary makes it.
    */
    bool format_assertion = false;

    /** The formats `format` checks when it asserts, by name. */
    format_set formats = {};

    /**
     Whether `contentEncoding`, `contentMediaType` and `contentSchema` assert: `base64` is decoded,
     `application/json` parsed, and `contentSchema` applied to the result, a failure of kind
     `error_kind::content`; else they annotate only.
    */
    bool content_assertion = false;

    /**
     The step budget of each search of a `pattern` or a `patternProperties` key, 0 for the regex
     policy's default; past it the search stops, an error of kind `error_kind::pattern_limit`.

     @note The step is the configured backend's unit, as webcpp::regex counts it, and no bound on
     time or memory: on SRELL, backtracking failures of groups and alternatives from one start
     position; on Boost.Regex, iterator steps per search; `std::regex` reads no budget.
    */
    std::size_t regex_step_budget = 0;

    /** What a keyword no dialect or vocabulary read knows becomes. */
    unknown_keywords unknown = unknown_keywords::dialect_default;

    /**
     Whether 2019-09 and 2020-12 read `definitions` and `dependencies` as drafts 4 to 7 do, as
     `$defs` and as `dependentRequired` and `dependentSchemas`; else they are unknown keywords.
    */
    bool compatibility_keywords = true;

    /**
     Whether each resource is validated against its meta-schema first; a violation is
     `errc::meta_schema_violation`, naming where the schema breaks it.
    */
    bool validate_schema = true;

    /** The user vocabularies whose keywords the compile reads, as active as each one says. */
    std::span<const basic_vocabulary<Traits>> vocabularies = {};

    /** The deepest nesting of schemas the compile reads; deeper is `errc::schema_too_deep`. */
    std::size_t max_schema_depth = default_max_schema_depth;
};

/** The compile options of the default traits. */
using compile_options = basic_compile_options<default_traits>;

/**
 How an instance is validated: how its messages are written, what is collected, and how deep it
 is read.

 @tparam Traits The traits of the schema that validates.
 @see "Options", in the guide.
*/
template <traits Traits>
struct basic_validate_options {
    /**
     The hook that writes each error's message, in place of the English templates; its callable
     must outlive every outcome that writes a message with it.
    */
    std::optional<basic_message_hook<Traits>> messages{};

    /** What `validate` collects, which decides the output formats the outcome can write. */
    collect what = collect::errors;

    /**
     The deepest nesting of the instance the validation descends into; a value deeper is an error
     of kind `error_kind::depth_limit`, and its branch fails.
    */
    std::size_t max_depth = default_max_depth;
};

/** The validate options of the default traits. */
using validate_options = basic_validate_options<default_traits>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_VALIDATOR_OPTIONS_HPP
