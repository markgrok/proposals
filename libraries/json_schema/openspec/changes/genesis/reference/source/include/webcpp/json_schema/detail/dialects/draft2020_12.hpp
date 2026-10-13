// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The machinery of draft 2020-12: the table of the keywords it reads.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_DIALECTS_DRAFT2020_12_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_DIALECTS_DRAFT2020_12_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/dialects/keyword_table.hpp>
#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>

#include <array>
#include <span>

namespace webcpp::json_schema::detail {

/**
 The machinery of draft 2020-12.

 Tip: the resource keywords, `$schema`, `$id`, `$anchor`, `$dynamicAnchor` and `$vocabulary`, are
 not in the table: the walk reads them before it, and none is a keyword of the IR.
*/
template <>
struct dialect_machinery<draft2020_12> {
    /**
     The keywords of 2020-12's vocabularies, in the order of its specification.

     @return The table, which lives as long as the program.
    */
    static std::span<const keyword_rule> keywords() noexcept {
        using k = ir::keyword_kind;
        using r = reader;
        namespace v = ir::vocabulary_bit;
        // clang-format off
        static constexpr std::array<keyword_rule, 52> table{
            keyword_rule{.name = "$ref", .kind = k::ref, .read = r::ref, .vocabulary = v::core},
            keyword_rule{.name = "$dynamicRef", .kind = k::dynamic_ref, .read = r::dynamic_ref, .vocabulary = v::core},
            keyword_rule{.name = "$defs", .kind = k::defs, .read = r::schema_map, .vocabulary = v::core},
            keyword_rule{.name = "$comment", .kind = k::comment, .read = r::annotation, .vocabulary = v::core},
            keyword_rule{.name = "allOf", .kind = k::all_of, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            keyword_rule{.name = "anyOf", .kind = k::any_of, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            keyword_rule{.name = "oneOf", .kind = k::one_of, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            keyword_rule{.name = "not", .kind = k::not_, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "if", .kind = k::if_, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "then", .kind = k::then_, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "else", .kind = k::else_, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "dependentSchemas", .kind = k::dependent_schemas, .read = r::schema_map, .vocabulary = v::applicator},
            keyword_rule{.name = "prefixItems", .kind = k::prefix_items, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            keyword_rule{.name = "items", .kind = k::items, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "contains", .kind = k::contains, .read = r::contains, .vocabulary = v::applicator},
            keyword_rule{.name = "properties", .kind = k::properties, .read = r::schema_map, .vocabulary = v::applicator},
            keyword_rule{.name = "patternProperties", .kind = k::pattern_properties, .read = r::pattern_map, .vocabulary = v::applicator},
            keyword_rule{.name = "additionalProperties", .kind = k::additional_properties, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "propertyNames", .kind = k::property_names, .read = r::schema, .vocabulary = v::applicator},
            keyword_rule{.name = "unevaluatedItems", .kind = k::unevaluated_items, .read = r::schema, .vocabulary = v::unevaluated},
            keyword_rule{.name = "unevaluatedProperties", .kind = k::unevaluated_properties, .read = r::schema, .vocabulary = v::unevaluated},
            keyword_rule{.name = "type", .kind = k::type, .read = r::types, .vocabulary = v::validation},
            keyword_rule{.name = "enum", .kind = k::enum_, .read = r::constants, .vocabulary = v::validation},
            keyword_rule{.name = "const", .kind = k::const_, .read = r::constant, .vocabulary = v::validation},
            keyword_rule{.name = "multipleOf", .kind = k::multiple_of, .read = r::positive_bound, .vocabulary = v::validation},
            keyword_rule{.name = "maximum", .kind = k::maximum, .read = r::bound, .vocabulary = v::validation},
            keyword_rule{.name = "exclusiveMaximum", .kind = k::exclusive_maximum, .read = r::bound, .vocabulary = v::validation},
            keyword_rule{.name = "minimum", .kind = k::minimum, .read = r::bound, .vocabulary = v::validation},
            keyword_rule{.name = "exclusiveMinimum", .kind = k::exclusive_minimum, .read = r::bound, .vocabulary = v::validation},
            keyword_rule{.name = "maxLength", .kind = k::max_length, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "minLength", .kind = k::min_length, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "pattern", .kind = k::pattern, .read = r::pattern, .vocabulary = v::validation},
            keyword_rule{.name = "maxItems", .kind = k::max_items, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "minItems", .kind = k::min_items, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "uniqueItems", .kind = k::unique_items, .read = r::constant, .vocabulary = v::validation},
            keyword_rule{.name = "maxContains", .kind = k::max_contains, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "minContains", .kind = k::min_contains, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "maxProperties", .kind = k::max_properties, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "minProperties", .kind = k::min_properties, .read = r::count, .vocabulary = v::validation},
            keyword_rule{.name = "required", .kind = k::required, .read = r::names, .vocabulary = v::validation},
            keyword_rule{.name = "dependentRequired", .kind = k::dependent_required, .read = r::dependent_required, .vocabulary = v::validation},
            keyword_rule{.name = "title", .kind = k::title, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "description", .kind = k::description, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "default", .kind = k::default_, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "deprecated", .kind = k::deprecated, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "readOnly", .kind = k::read_only, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "writeOnly", .kind = k::write_only, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "examples", .kind = k::examples, .read = r::annotation, .vocabulary = v::meta_data},
            keyword_rule{.name = "format", .kind = k::format, .read = r::format, .vocabulary = v::format_annotation},
            keyword_rule{.name = "contentEncoding", .kind = k::content_encoding, .read = r::text, .vocabulary = v::content},
            keyword_rule{.name = "contentMediaType", .kind = k::content_media_type, .read = r::text, .vocabulary = v::content},
            keyword_rule{.name = "contentSchema", .kind = k::content_schema, .read = r::schema, .vocabulary = v::content},
        };
        // clang-format on
        return table;
    }
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_DIALECTS_DRAFT2020_12_HPP
