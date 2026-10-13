// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 What a validation reports: the kind of each failed keyword, the error that
 locates it, the annotations that passing keywords produce, and how much of
 that a validation collects.

 @see "Output", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_OUTPUT_ERROR_HPP
#define WEBCPP_JSON_SCHEMA_OUTPUT_ERROR_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/deep_copy.hpp>

#include <boost/assert.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory_resource>
#include <new>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

namespace webcpp::json_schema {

/**
 Which keyword failed, or why a subschema did.

 @note The values are fixed: they are never reused for a different meaning,
 and a new kind takes the next value, so a stored value keeps its meaning
 across versions.
*/
enum class error_kind : std::uint16_t {
    /** The schema is `false`, which no value matches. */
    false_schema = 1,

    /** `type`: the value is of none of the types listed. */
    type = 2,

    /** `enum`: the value equals none of the values listed. */
    enum_value = 3,

    /** `const`: the value differs from the constant. */
    const_value = 4,

    /** `multipleOf`: the number is not a multiple of the divisor. */
    multiple_of = 5,

    /** `maximum`: the number is greater than the bound. */
    maximum = 6,

    /** `exclusiveMaximum`: the number is not less than the bound. */
    exclusive_maximum = 7,

    /** `minimum`: the number is less than the bound. */
    minimum = 8,

    /** `exclusiveMinimum`: the number is not greater than the bound. */
    exclusive_minimum = 9,

    /** `maxLength`: the string has more code points than the bound. */
    max_length = 10,

    /** `minLength`: the string has fewer code points than the bound. */
    min_length = 11,

    /** `pattern`: the regular expression finds no match in the string. */
    pattern = 12,

    /** `pattern`: the regex policy stopped at its limit before it could decide. */
    pattern_limit = 13,

    /** `format`, when it asserts: the string is not of the format. */
    format = 14,

    /** `maxItems`: the array has more items than the bound. */
    max_items = 15,

    /** `minItems`: the array has fewer items than the bound. */
    min_items = 16,

    /** `uniqueItems`: two items of the array are equal. */
    unique_items = 17,

    /** `contains`: no item matches its schema. */
    contains = 18,

    /** `minContains`: fewer items match `contains` than the bound. */
    min_contains = 19,

    /** `maxContains`: more items match `contains` than the bound. */
    max_contains = 20,

    /** `maxProperties`: the object has more members than the bound. */
    max_properties = 21,

    /** `minProperties`: the object has fewer members than the bound. */
    min_properties = 22,

    /** `required`: members the object must have are missing. */
    required = 23,

    /** `dependentRequired`: members a present member requires are missing. */
    dependent_required = 24,

    /** `properties`: a member fails its schema. */
    properties = 25,

    /** `patternProperties`: a member fails the schema of a pattern its name matches. */
    pattern_properties = 26,

    /** `additionalProperties`: a member no other keyword evaluates fails its schema. */
    additional_properties = 27,

    /** `propertyNames`: a member's name fails its schema. */
    property_names = 28,

    /** `dependentSchemas`: the object fails the schema a present member selects. */
    dependent_schemas = 29,

    /** `prefixItems`: an item fails the schema of its position. */
    prefix_items = 30,

    /** `items`: an item fails its schema. */
    items = 31,

    /** `unevaluatedItems`: an item no other keyword evaluates fails its schema. */
    unevaluated_items = 32,

    /** `unevaluatedProperties`: a member no other keyword evaluates fails its schema. */
    unevaluated_properties = 33,

    /** `allOf`: the value fails at least one of the schemas. */
    all_of = 34,

    /** `anyOf`: the value matches none of the schemas. */
    any_of = 35,

    /** `oneOf`: the value matches none of the schemas, or more than one. */
    one_of = 36,

    /** `not`: the value matches the schema. */
    not_schema = 37,

    /** `if`, `then`, `else`: the value fails the schema its `if` selects. */
    if_then_else = 38,

    /** `$ref`, `$dynamicRef` or `$recursiveRef`: the referenced schema had errors. */
    reference = 39,

    /** `contentMediaType`, `contentEncoding` or `contentSchema`, when they assert. */
    content = 40,

    /** A schema object had errors. */
    schema = 41,

    /** A user keyword failed. */
    user = 42,

    /** The instance nests deeper than the validation's depth limit. */
    depth_limit = 43,
};

/**
 What an error has to say beyond its kind and its locations.
*/
struct error_details {
    /**
     One alternative per kind that has more to say, views into the outcome's storage.

     The alternatives are: nothing; the names missing for `required` and
     `dependent_required`, or offending for `additional_properties` and
     `unevaluated_properties`; the two equal indices for `unique_items`, or the
     matching ones for `one_of`; a length, item, property or match count; and a
     user keyword's own error.
    */
    std::variant<std::monostate, std::span<const std::string_view>, std::span<const std::size_t>,
                 std::uint64_t, boost::system::error_code>
        value{};
};

/**
 How much a validation collects: the outcome holds that, and its output formats
 need it.

 @see "Output", in the guide.
*/
enum class collect : std::uint8_t {
    /** The errors that count, and the units that locate them. */
    errors,

    /** The errors and the annotations that count. */
    errors_and_annotations,

    /** Everything, what a passing applicator's failed alternatives recorded included. */
    everything,
};

namespace detail {

/** `text`, copied into `arena`, which an empty text does not touch. */
inline std::string_view copy_text(std::string_view text, std::pmr::memory_resource& arena) {
    if (text.empty()) {
        return {};
    }
    auto* const bytes = static_cast<char*>(arena.allocate(text.size(), 1));
    std::memcpy(bytes, text.data(), text.size());
    return {bytes, text.size()};
}

/** The fields of an error, each named at the call that builds one. */
struct error_fields {
    /** The kind of the keyword that failed. */
    error_kind kind;
    /** The keyword as written. */
    std::string_view keyword{};
    /** The instance location, a JSON Pointer. */
    std::string_view instance_location{};
    /** The evaluation path, a JSON Pointer. */
    std::string_view keyword_location{};
    /** The resource's URI, `#` and the keyword's pointer, percent-encoded as a URI fragment. */
    std::string_view absolute_keyword_location{};
    /** The keyword's value, never null. */
    const boost::json::value* expected = nullptr;
    /** The instance's value, never null. */
    const boost::json::value* received = nullptr;
    /** What the kind has to say more. */
    error_details details{};
};

/** The fields of an annotation, each named at the call that builds one. */
struct annotation_fields {
    /** The keyword as written. */
    std::string_view keyword{};
    /** The instance location, a JSON Pointer. */
    std::string_view instance_location{};
    /** The evaluation path, a JSON Pointer. */
    std::string_view keyword_location{};
    /** The resource's URI, `#` and the keyword's pointer, percent-encoded as a URI fragment. */
    std::string_view absolute_keyword_location{};
    /** The value, never null. */
    const boost::json::value* value = nullptr;
};

struct error_access;

}  // namespace detail

/**
 A keyword that failed: its kind, where it is in the schema, where it looked in
 the instance, and the two values it compared.

 Every view refers to the outcome's storage, to the compiled schema, which the
 outcome keeps alive, or to the instance, which must outlive the outcome.
*/
class error {
public:
    /**
     The kind of the keyword that failed.

     @return The kind.
    */
    [[nodiscard]] error_kind kind() const noexcept { return kind_; }

    /**
     The keyword as written in the schema.

     @return The keyword, for example `maxItems`.
    */
    [[nodiscard]] std::string_view keyword() const noexcept { return keyword_; }

    /**
     Where in the instance the keyword looked.

     @return A JSON Pointer from the instance's root.
    */
    [[nodiscard]] std::string_view instance_location() const noexcept { return instance_location_; }

    /**
     The evaluation path to the keyword.

     @return A JSON Pointer from the schema's root through the applicators
     evaluated, `$ref` and `$dynamicRef` segments included.
    */
    [[nodiscard]] std::string_view keyword_location() const noexcept { return keyword_location_; }

    /**
     Where the keyword is written.

     @return The canonical URI of its resource, `#`, and the keyword's JSON
     Pointer in that resource, percent-encoded as a URI fragment (RFC 6901
     section 6), so that it is a URI.
    */
    [[nodiscard]] std::string_view absolute_keyword_location() const noexcept {
        return absolute_keyword_location_;
    }

    /**
     The keyword's value, in the compiled schema.

     @return The value, which lives as long as the outcome.
    */
    [[nodiscard]] const boost::json::value& expected() const noexcept { return *expected_; }

    /**
     The value the keyword failed, in the instance.

     @return The value, which lives as long as the instance.
    */
    [[nodiscard]] const boost::json::value& received() const noexcept { return *received_; }

    /**
     What the error has to say beyond its kind.

     @return The details, whose alternative depends on the kind.
    */
    [[nodiscard]] const error_details& details() const noexcept { return details_; }

private:
    friend struct detail::error_access;

    explicit error(const detail::error_fields& f) noexcept
        : kind_(f.kind),
          keyword_(f.keyword),
          instance_location_(f.instance_location),
          keyword_location_(f.keyword_location),
          absolute_keyword_location_(f.absolute_keyword_location),
          expected_(f.expected),
          received_(f.received),
          details_(f.details) {}

    error_kind kind_;
    std::string_view keyword_;
    std::string_view instance_location_;
    std::string_view keyword_location_;
    std::string_view absolute_keyword_location_;
    const boost::json::value* expected_;
    const boost::json::value* received_;
    error_details details_;
};

/**
 A value a passing keyword produced: a schema's constant, or a value the
 validation computed, with where it was produced.
*/
class annotation {
public:
    /**
     The keyword that produced the annotation, as written in the schema.

     @return The keyword, for example `title`.
    */
    [[nodiscard]] std::string_view keyword() const noexcept { return keyword_; }

    /**
     Where in the instance the keyword applied.

     @return A JSON Pointer from the instance's root.
    */
    [[nodiscard]] std::string_view instance_location() const noexcept { return instance_location_; }

    /**
     The evaluation path to the keyword.

     @return A JSON Pointer from the schema's root through the applicators
     evaluated, `$ref` and `$dynamicRef` segments included.
    */
    [[nodiscard]] std::string_view keyword_location() const noexcept { return keyword_location_; }

    /**
     Where the keyword is written.

     @return The canonical URI of its resource, `#`, and the keyword's JSON
     Pointer in that resource, percent-encoded as a URI fragment (RFC 6901
     section 6), so that it is a URI.
    */
    [[nodiscard]] std::string_view absolute_keyword_location() const noexcept {
        return absolute_keyword_location_;
    }

    /**
     The annotation's value.

     @return The value, in the outcome's storage: a copy of the schema's
     constant, or the value computed, such as the names `properties` evaluated.
    */
    [[nodiscard]] const boost::json::value& value() const noexcept { return *value_; }

private:
    friend struct detail::error_access;

    explicit annotation(const detail::annotation_fields& f) noexcept
        : keyword_(f.keyword),
          instance_location_(f.instance_location),
          keyword_location_(f.keyword_location),
          absolute_keyword_location_(f.absolute_keyword_location),
          value_(f.value) {}

    std::string_view keyword_;
    std::string_view instance_location_;
    std::string_view keyword_location_;
    std::string_view absolute_keyword_location_;
    const boost::json::value* value_;
};

namespace detail {

/**
 The one way the engine builds an error or an annotation, and copies one into
 an outcome's storage.
*/
struct error_access {
    /** An error of the given fields, holding their views as they are. */
    static error make_error(const error_fields& f) noexcept {
        BOOST_ASSERT(f.expected != nullptr && f.received != nullptr);
        return error(f);
    }

    /** An annotation of the given fields, holding their views as they are. */
    static annotation make_annotation(const annotation_fields& f) noexcept {
        BOOST_ASSERT(f.value != nullptr);
        return annotation(f);
    }

    /**
     A copy of `e` whose strings and details live in `arena`.

     Tip: `expected` and `received` stay where they are, in the compiled schema
     and in the instance, which outlive the outcome.
    */
    static error copy_into(const error& e, std::pmr::memory_resource& arena) {
        return error({
            .kind = e.kind_,
            .keyword = copy_text(e.keyword_, arena),
            .instance_location = copy_text(e.instance_location_, arena),
            .keyword_location = copy_text(e.keyword_location_, arena),
            .absolute_keyword_location = copy_text(e.absolute_keyword_location_, arena),
            .expected = e.expected_,
            .received = e.received_,
            .details = copy_details(e.details_, arena),
        });
    }

    /**
     A copy of `a` whose strings live in `arena` and whose value is a copy in
     `values`, made with `scratch` for its work stack.

     Tip: the value's own `boost::json::value` lives in `arena` and is never
     destroyed, so `values` must deallocate trivially.
    */
    static annotation copy_into(const annotation& a, std::pmr::memory_resource& arena,
                                boost::json::storage_ptr values,
                                std::pmr::memory_resource& scratch) {
        BOOST_ASSERT(values.is_deallocate_trivial());
        void* const place = arena.allocate(sizeof(boost::json::value), alignof(boost::json::value));
        const auto* const value =
            ::new (place) boost::json::value(deep_copy(*a.value_, std::move(values), scratch));
        return annotation({
            .keyword = copy_text(a.keyword_, arena),
            .instance_location = copy_text(a.instance_location_, arena),
            .keyword_location = copy_text(a.keyword_location_, arena),
            .absolute_keyword_location = copy_text(a.absolute_keyword_location_, arena),
            .value = value,
        });
    }

private:
    /** `names`, each copied into `arena`, in an array of `arena`. */
    static std::span<const std::string_view> copy_names(std::span<const std::string_view> names,
                                                        std::pmr::memory_resource& arena) {
        if (names.empty()) {
            return {};
        }
        auto* const copied = static_cast<std::string_view*>(
            arena.allocate(names.size_bytes(), alignof(std::string_view)));
        for (std::size_t i = 0; i < names.size(); ++i) {
            ::new (static_cast<void*>(copied + i)) std::string_view(copy_text(names[i], arena));
        }
        return {copied, names.size()};
    }

    /** `indices`, copied into `arena`. */
    static std::span<const std::size_t> copy_indices(std::span<const std::size_t> indices,
                                                     std::pmr::memory_resource& arena) {
        if (indices.empty()) {
            return {};
        }
        auto* const copied =
            static_cast<std::size_t*>(arena.allocate(indices.size_bytes(), alignof(std::size_t)));
        std::memcpy(copied, indices.data(), indices.size_bytes());
        return {copied, indices.size()};
    }

    /** `details`, with every name and index it views copied into `arena`. */
    static error_details copy_details(const error_details& details,
                                      std::pmr::memory_resource& arena) {
        if (const auto* names = std::get_if<std::span<const std::string_view>>(&details.value)) {
            return {.value = copy_names(*names, arena)};
        }
        if (const auto* indices = std::get_if<std::span<const std::size_t>>(&details.value)) {
            return {.value = copy_indices(*indices, arena)};
        }
        return details;
    }
};

}  // namespace detail

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_OUTPUT_ERROR_HPP
