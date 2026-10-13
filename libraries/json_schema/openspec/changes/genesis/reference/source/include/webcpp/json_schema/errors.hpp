// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The errors json_schema reports, as `boost::system::error_code` values of a
 category of its own, the diagnostic that names where each happened, and the
 result type of every operation that can fail.

 @see "Errors and exceptions", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_ERRORS_HPP
#define WEBCPP_JSON_SCHEMA_ERRORS_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/assert/source_location.hpp>
#include <boost/static_string/static_string.hpp>
#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace webcpp::json_schema {

/**
 Why a load, a compile, an emit or a rendering failed.

 The values start at 1, so an `error_code` of value 0 is no error. An `errc`
 converts to an `error_code` and compares with one:
 `compiled.error().code == js::errc::unknown_dialect`. A verdict of validation
 is never an error.

 @note The values are fixed: they are never reused for a different meaning,
 so a stored value keeps its meaning across versions.
 @see "Errors and exceptions", in the guide.
*/
enum class errc : int {
    /**
     A value where a schema must be is neither an object nor a boolean, or is a boolean in draft 4
     or OpenAPI 3.0.
    */
    not_a_schema = 1,

    /** A known keyword's value has the wrong shape. */
    invalid_keyword_value = 2,

    /** The schema fails its meta-schema, which a compile checks by default. */
    meta_schema_violation = 3,

    /** `$schema` names a dialect outside the validator's set. */
    unknown_dialect = 4,

    /** A required vocabulary is one the validator does not know. */
    unknown_vocabulary = 5,

    /** A keyword is refused: by the option that rejects unknown keywords, or by OpenAPI 3.0. */
    unknown_keyword = 6,

    /** An `$id`, a `$ref` or a base URI is not a URI reference. */
    invalid_uri = 7,

    /** No resource answers a reference's URI. */
    unresolvable_reference = 8,

    /** The resolver returned an error for a URI. */
    resolver_failed = 9,

    /** A fragment is not a JSON Pointer, or points at nothing. */
    invalid_json_pointer = 10,

    /** Two resources have one URI. */
    duplicate_resource = 11,

    /** Two anchors have one name in one resource. */
    duplicate_anchor = 12,

    /** A cycle of references consumes no part of the instance. */
    reference_cycle = 13,

    /** A pattern is one the regex policy refuses. */
    invalid_regex = 14,

    /** The schema nests past the compile's depth limit. */
    schema_too_deep = 15,

    /** A number is one the number type cannot hold. */
    invalid_number = 16,

    /** An IR built by hand breaks one of its invariants. */
    invalid_ir = 17,

    /** The target dialect of an emit cannot express a node. */
    unrepresentable = 18,

    /** An output format needs more than the validation collected. */
    output_not_collected = 19,

    /** A user keyword's name is taken in a resource. */
    vocabulary_conflict = 20,
};

namespace detail {

/**
 The human message of an @ref errc value, or the unknown message for a value it does not define.
*/
inline const char* message_of(int value) noexcept {
    switch (static_cast<errc>(value)) {
        case errc::not_a_schema: return "the value is not a schema";
        case errc::invalid_keyword_value: return "a keyword's value has the wrong shape";
        case errc::meta_schema_violation: return "the schema fails its meta-schema";
        case errc::unknown_dialect: return "the dialect is outside the validator's set";
        case errc::unknown_vocabulary: return "a required vocabulary is unknown";
        case errc::unknown_keyword: return "the keyword is refused";
        case errc::invalid_uri: return "the URI is not a URI reference";
        case errc::unresolvable_reference: return "no resource for the reference";
        case errc::resolver_failed: return "the resolver failed";
        case errc::invalid_json_pointer: return "the JSON Pointer is invalid or points at nothing";
        case errc::duplicate_resource: return "two resources have one URI";
        case errc::duplicate_anchor: return "two anchors have one name";
        case errc::reference_cycle: return "a cycle of references consumes no instance";
        case errc::invalid_regex: return "the pattern is not a valid regular expression";
        case errc::schema_too_deep: return "the schema nests too deeply";
        case errc::invalid_number: return "the number cannot be represented";
        case errc::invalid_ir: return "the IR breaks an invariant";
        case errc::unrepresentable: return "the target dialect cannot express the schema";
        case errc::output_not_collected: return "the output format needs more than was collected";
        case errc::vocabulary_conflict: return "a user keyword's name is taken";
    }
    return "unknown webcpp.json_schema error";
}

/**
 A constant category with a fixed identity: it keeps no state of its own; Boost.System
 initializes its `std::error_category` view once, atomically.

 @note The identity is the ASCII of "jschema" followed by one zero byte.
*/
class category final : public boost::system::error_category {
public:
    /** Constructs the category with its fixed identity. */
    constexpr category() noexcept : boost::system::error_category(0x6a736368656d6100ULL) {}

    /** The category's name, "webcpp.json_schema". */
    const char* name() const noexcept override { return "webcpp.json_schema"; }

    /** The human message of the error `value`, as @ref message_of spells it. */
    std::string message(int value) const override { return message_of(value); }

    /**
     Copies the human message of the error `value` into `buffer`, cut to fit `size` bytes with
     its terminating zero, and returns the whole message.
    */
    const char* message(int value, char* buffer, std::size_t size) const noexcept override {
        const char* text = message_of(value);
        if (buffer != nullptr && size != 0) {
            const std::string_view spelling(text);
            const std::size_t limit = size - 1;
            const std::size_t count = spelling.size() < limit ? spelling.size() : limit;
            std::memcpy(buffer, spelling.data(), count);
            buffer[count] = '\0';
        }
        return text;
    }
};

/** The one category of json_schema's errors, which @ref error_category returns. */
inline constexpr category the_category{};

}  // namespace detail

/**
 The category of every @ref errc.

 Its `name()` is "webcpp.json_schema", and its `message(value)` the human
 message of the error, "a cycle of references consumes no instance" for
 @ref errc::reference_cycle, or "unknown webcpp.json_schema error" for a value
 @ref errc does not define.

 @return The category, a constant with a fixed identity.
 @see "Errors and exceptions", in the guide.
*/
inline const boost::system::error_category& error_category() noexcept {
    return detail::the_category;
}

/**
 Makes the `error_code` of an @ref errc, as its implicit conversion does.

 @param e The error.
 @return `e` as an `error_code` of the category @ref error_category.
*/
inline boost::system::error_code make_error_code(errc e) noexcept {
    return {static_cast<int>(e), error_category()};
}

/**
 A failure with the place it happened: an error code, where, and what.

 It holds its text inline, so a failure is reported without allocating, under
 an exhausted allocator too. A text longer than 256 bytes keeps at most its
 first 253, never part of a UTF-8 sequence, and ends with "...", so that valid
 UTF-8 stays valid.

 @see "Errors and exceptions", in the guide.
*/
struct diagnostic {
    /** Why it failed: an @ref errc, or the code of the component that refused. */
    boost::system::error_code code;

    /** Where: a keyword location or a URI, cut to 256 bytes on a code point boundary, with "...".
     */
    boost::static_strings::static_string<256> where;

    /** What: the dialect, keyword or detail named, cut likewise. */
    boost::static_strings::static_string<256> what;
};

/**
 What every operation that can fail returns: a `T`, or the @ref diagnostic of
 the failure.

 It is Boost.System's `result` with a @ref diagnostic as its error. Its
 `value()` of an error raises it, through @ref throw_exception_from_error.

 @tparam T The type of the value; `void` for an operation that returns none.
 @see "Errors and exceptions", in the guide.
*/
template <class T>
using result = boost::system::result<T, diagnostic>;

namespace detail {

/** The bytes of the UTF-8 sequence whose first byte is `byte`: 1 for any byte that leads none. */
constexpr std::size_t utf8_length(unsigned char byte) noexcept {
    if ((byte & 0xE0U) == 0xC0U) {
        return 2;
    }
    if ((byte & 0xF0U) == 0xE0U) {
        return 3;
    }
    if ((byte & 0xF8U) == 0xF0U) {
        return 4;
    }
    return 1;
}

/**
 Copies `text` into `out`, cut on a code point boundary and ended by "..." when it does not fit.

 The cut keeps the first `max_size() - 3` bytes, less the sequence the cut would split: when one
 of the last three kept bytes is a UTF-8 lead byte whose sequence runs past them, the cut moves
 before the lowest such byte, so at most three bytes go. A well-formed sequence is never split,
 and a lead byte is never left at the cut without the bytes it announces, so valid UTF-8 stays
 valid. What the text already holds wrongly before the cut, such as continuation bytes with no
 lead byte, is kept as it is: the cut neither moves for it nor repairs it.

 Tip: `assign` and `append` throw only for a text longer than `max_size()`, which the cut rules out,
 hence the exemption from bugprone-exception-escape.
*/
// NOLINTNEXTLINE(bugprone-exception-escape)
inline void assign_cut(boost::static_strings::static_string<256>& out,
                       std::string_view text) noexcept {
    if (text.size() <= out.max_size()) {
        out.assign(text.data(), text.size());
        return;
    }
    const std::size_t boundary = out.max_size() - 3;
    std::size_t keep = boundary;
    for (std::size_t lead = boundary - 1; lead >= boundary - 3; --lead) {
        if (lead + utf8_length(static_cast<unsigned char>(text[lead])) > boundary) {
            keep = lead;
        }
    }
    const std::string_view kept = text.substr(0, keep);
    out.assign(kept.data(), kept.size());
    out.append("...");
}

/** A diagnostic of `code`, `where` and `what`, without allocating. */
inline diagnostic make_diagnostic(boost::system::error_code code, std::string_view where,
                                  std::string_view what) noexcept {
    diagnostic d;
    d.code = code;
    assign_cut(d.where, where);
    assign_cut(d.what, what);
    return d;
}

}  // namespace detail

/**
 Raises a diagnostic as a `boost::system::system_error`, for `result::value()`.

 The exception's code is the diagnostic's, and its text is `where`, then
 `": "` and `what` when there is one. `result::value()` of an error finds this
 function by argument-dependent lookup. Without exceptions,
 `boost::throw_exception` calls the program's handler.

 @param d The diagnostic raised.
 @param loc Where `value()` was called.
 @throws boost::system::system_error Always.
 @see "Errors and exceptions", in the guide.
*/
[[noreturn]] inline void throw_exception_from_error(const diagnostic& d,
                                                    const boost::source_location& loc) {
    std::string text(d.where.data(), d.where.size());
    if (!d.what.empty()) {
        text.append(": ").append(d.what.data(), d.what.size());
    }
    boost::throw_exception(boost::system::system_error(d.code, text), loc);
}

}  // namespace webcpp::json_schema

namespace boost::system {
template <>
struct is_error_code_enum<webcpp::json_schema::errc> : std::true_type {};
}  // namespace boost::system

#endif  // WEBCPP_JSON_SCHEMA_ERRORS_HPP
