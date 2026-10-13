// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The loader's walk: a schema read into the IR, one value in a schema position at a time, on an
 explicit work list, over one copy of the schema.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_LOADER_WALK_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_LOADER_WALK_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/dialects/keyword_table.hpp>
#include <webcpp/json_schema/detail/json/exact_handler.hpp>
#include <webcpp/json_schema/detail/json/utf8.hpp>
#include <webcpp/json_schema/detail/loader/readers.hpp>
#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/ir/document.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>
#include <webcpp/json_schema/traits.hpp>
#include <webcpp/json_schema/validator/options.hpp>

#include <boost/container/map.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace webcpp::json_schema::detail {

/** A dialect of a validator's set, and the table of the keywords it reads. */
struct dialect_keywords {
    /** The dialect. */
    dialect id = dialect::draft2020_12;

    /** Its keyword table. */
    std::span<const keyword_rule> keywords{};
};

/**
 The name of the first option a load cannot honour yet, or nothing: one that asks for references,
 formats, content, user vocabularies, or an unknown keyword ignored or refused.

 Tip: `compatibility_keywords` and `validate_schema` off are honoured, as their defaults are not
 yet: `definitions` and `dependencies` load as unknown keywords, and a load never checks a schema
 against its meta-schema. `regex_step_budget` is the compiler's.
*/
template <class Traits>
std::optional<std::string_view> unread_option(const basic_compile_options<Traits>& o) noexcept {
    if (o.registry != nullptr) {
        return "registry";
    }
    if (o.resolver) {
        return "resolver";
    }
    if (!o.base_uri.empty()) {
        return "base_uri";
    }
    if (o.format_assertion) {
        return "format_assertion";
    }
    if (!o.formats.empty()) {
        return "formats";
    }
    if (o.content_assertion) {
        return "content_assertion";
    }
    if (o.unknown != unknown_keywords::dialect_default && o.unknown != unknown_keywords::annotate) {
        return "unknown";
    }
    if (!o.vocabularies.empty()) {
        return "vocabularies";
    }
    return std::nullopt;
}

/** Whether `uri` is absolute: a scheme (RFC 3986 section 3.1), then a colon. */
constexpr bool has_scheme(std::string_view uri) noexcept {
    const auto alpha = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
    const std::size_t colon = uri.find(':');
    if (colon == 0 || colon == std::string_view::npos || !alpha(uri.front())) {
        return false;
    }
    return std::all_of(
        uri.begin() + 1, uri.begin() + static_cast<std::ptrdiff_t>(colon), [&](char c) {
            return alpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
        });
}

/** Whether `name` is an anchor's name in 2020-12: `^[A-Za-z_][-A-Za-z0-9._]*$`. */
constexpr bool is_anchor_name(std::string_view name) noexcept {
    const auto letter = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    if (name.empty() || !letter(name.front())) {
        return false;
    }
    return std::all_of(name.begin() + 1, name.end(), [&](char c) {
        return letter(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
    });
}

/**
 The loader: a schema walked into a document of the IR, iteratively.

 The schema is copied once into the document, and every keyword's source is a view of that copy.
 The walk keeps an explicit list of the values in schema positions whose nodes are reserved and not
 yet defined, so that nothing recurses on the schema's depth. It reads a node's keywords before it
 walks the node's subschemas, and walks those in document order, so the fault it reports is the
 first of that order.

 Tip: a walker reads one schema; it is made, run once and dropped.

 @tparam Traits The traits of the document.
*/
template <class Traits>
class walker {
public:
    /** The document a walk builds. */
    using document_type = ir::basic_document<Traits>;

    /** The allocator of the document and of the walk's tables. */
    using allocator_type = Traits::allocator_type;

    /**
     A walker for the validator's `dialects`, its first the default, with `options`; each must
     outlive it.
    */
    walker(std::span<const dialect_keywords> dialects, const basic_compile_options<Traits>& options,
           allocator_type allocator)
        : dialects_(dialects),
          options_(options),
          build_(allocator),
          keywords_(rebind<ir::keyword>(allocator)),
          resource_keywords_(rebind<std::span<const keyword_rule>>(allocator)),
          anchors_(anchor_less(), rebind<std::pair<const anchor_key, ir::node_id>>(allocator)) {}

    walker(const walker&) = delete;
    walker& operator=(const walker&) = delete;

    /**
     Walks `root` into a document.

     @param root The schema.
     @param exact The exact numbers of the text `root` was parsed from, or nullptr.
     @return The document, or the diagnostic of the first fault.
    */
    result<document_type> run(const boost::json::value& root, const exact_table<Traits>* exact) {
        if (const auto refused = refuse_options(); !refused) {
            return refused.error();
        }
        build_.use_exact_numbers(exact);
        const ir::value_id copy = build_.copy(root);
        const auto opened = open_root_resource(build_.doc.value(copy), copy);
        if (!opened) {
            return opened.error();
        }
        build_.work.push_back(*opened);
        while (!build_.work.empty()) {
            const pending p = build_.work.back();
            build_.work.pop_back();
            if (const auto r = walk_one(p); !r) {
                return r.error();
            }
        }
        build_.doc.set_root(opened->node);
        return std::move(build_.doc);
    }

private:
    template <class T>
    using rebind = std::allocator_traits<allocator_type>::template rebind_alloc<T>;

    /** An anchor's resource, name and dynamism: what makes one unique (invariant 10). */
    using anchor_key = std::tuple<ir::resource_id, ir::string_id, bool>;

    /** The order of @ref anchor_key, never a hash, so that names chosen to collide cost nothing. */
    using anchor_less = std::less<>;

    /**
     Refuses `default_dialect` outside the set, then an option a load cannot honour yet, naming
     it: `where` is `"options"`.
    */
    [[nodiscard]] result<void> refuse_options() const {
        if (options_.default_dialect && !dialect_entry(*options_.default_dialect)) {
            const dialect d = *options_.default_dialect;
            const std::string_view uri = d == dialect::openapi3_0 ? "openapi3_0" : uri_of(d);
            return make_diagnostic(errc::unknown_dialect, "options", uri);
        }
        if (const auto name = unread_option(options_)) {
            return make_diagnostic(errc::invalid_keyword_value, "options", *name);
        }
        return {};
    }

    /** The entry of `d` in the validator's set, or nullptr. */
    [[nodiscard]] const dialect_keywords* dialect_entry(dialect d) const noexcept {
        const auto found = std::ranges::find(dialects_, d, &dialect_keywords::id);
        return found == dialects_.end() ? nullptr : &*found;
    }

    /**
     The dialect of the root resource: its `$schema`, else `default_dialect`, else the validator's
     first.
    */
    result<const dialect_keywords*> root_dialect(const boost::json::object* root) {
        const ir::pointer_id where = ir::empty_pointer;
        const boost::json::value* schema = root == nullptr ? nullptr : root->if_contains("$schema");
        if (schema == nullptr) {
            return dialect_entry(options_.default_dialect.value_or(dialects_.front().id));
        }
        const auto* uri = schema->if_string();
        if (uri == nullptr) {
            return build_.refusal_at_member(errc::invalid_keyword_value, where, "$schema",
                                            "$schema");
        }
        const std::optional<dialect> named = dialect_of_uri(*uri);
        const dialect_keywords* entry = named ? dialect_entry(*named) : nullptr;
        if (entry == nullptr) {
            return build_.refusal_at_member(errc::unknown_dialect, where, "$schema", *uri);
        }
        return entry;
    }

    /** An `$id` as written, and its URI: as written, less an empty fragment. */
    struct id_text {
        /** As written. */
        std::string_view written{};

        /** The URI it names, without its empty fragment. */
        std::string_view uri{};
    };

    /**
     The `$id` of the node of `p`, read.

     @return Nothing when it has none, else the `$id`; or the refusal of a value that is not a
     string or not UTF-8, or of one whose fragment is not empty (`errc::invalid_uri`).
    */
    result<std::optional<id_text>> read_id(const pending& p, const boost::json::object& object) {
        const boost::json::value* id = object.if_contains("$id");
        if (id == nullptr) {
            return std::optional<id_text>{};
        }
        const auto* text = id->if_string();
        if (text == nullptr || !is_valid_utf8(*text)) {
            return build_.refusal_at_member(errc::invalid_keyword_value, p.pointer, "$id", "$id");
        }
        std::string_view uri = *text;
        if (const std::size_t hash = uri.find('#'); hash != std::string_view::npos) {
            if (hash + 1 != uri.size()) {
                return build_.refusal_at_member(errc::invalid_uri, p.pointer, "$id", *text);
            }
            uri.remove_suffix(1);
        }
        return std::optional<id_text>(id_text{.written = *text, .uri = uri});
    }

    /**
     Opens the root resource: its dialect, its base URI and its root node.

     Tip: the base URI is the root's `$id` when it is absolute, else `urn:webcpp:root`; a relative
     one would be resolved against that base, which needs the URL policy, so it is refused as
     `errc::unresolvable_reference` meanwhile, as a nested `$id` is.

     @param root The document's copy of the schema.
     @param view The copy's id.
     @return The root's pending value, or the refusal of its `$schema` or its `$id`.
    */
    result<pending> open_root_resource(const boost::json::value& root, ir::value_id view) {
        const boost::json::object* object = root.if_object();
        const auto entry = root_dialect(object);
        if (!entry) {
            return entry.error();
        }
        const pending root_pending{
            .value = &root,
            .node = build_.doc.reserve(ir::resource_id{0}, ir::empty_pointer),
            .resource = ir::resource_id{0},
            .depth = 0,
            .pointer = ir::empty_pointer,
            .view = view,
            .parent_view = view,
        };
        std::string_view uri = "urn:webcpp:root";
        if (object != nullptr) {
            const auto id = read_id(root_pending, *object);
            if (!id) {
                return id.error();
            }
            if (*id && !(*id)->uri.empty()) {
                if (!has_scheme((*id)->uri)) {
                    return build_.refusal_at_member(errc::unresolvable_reference,
                                                    root_pending.pointer, "$id", (*id)->written);
                }
                uri = (*id)->uri;
            }
        }
        build_.doc.add_resource({
            .uri = build_.doc.intern(uri),
            .source_dialect = (*entry)->id,
            .root = root_pending.node,
            .parent = ir::resource_id{0},
            .vocabularies = vocabularies_of((*entry)->keywords),
            .recursive_anchor = false,
        });
        resource_keywords_.push_back((*entry)->keywords);
        return root_pending;
    }

    /** Defines the node of `p` from its value, and queues its subschemas in document order. */
    result<void> walk_one(const pending& p) {
        if (p.depth > options_.max_schema_depth) {
            return build_.refusal(errc::schema_too_deep, p.pointer, "");
        }
        if (const bool* b = p.value->if_bool()) {
            build_.doc.define(p.node, *b ? ir::node_kind::always_valid : ir::node_kind::never_valid,
                              {});
            return {};
        }
        const auto* object = p.value->if_object();
        if (object == nullptr) {
            return build_.refusal(errc::not_a_schema, p.pointer, "");
        }
        if (auto r = read_resource_keywords(p, *object); !r) {
            return r;
        }
        const std::size_t queued = build_.work.size();
        // An empty object has no keyword whose source needs its view.
        const ir::value_id view = object->empty() ? ir::no_value : build_.view_of(p);
        keywords_.clear();
        for (const auto& member : *object) {
            if (auto r = read_member(p, view, member.key(), member.value()); !r) {
                return r;
            }
        }
        // The work list is a stack: its children reversed, the first in source order comes out
        // first.
        std::reverse(build_.work.begin() + static_cast<std::ptrdiff_t>(queued), build_.work.end());
        build_.doc.define(p.node, ir::node_kind::object, keywords_);
        return {};
    }

    /** Whether `name` is a resource keyword, read before the table and never an IR keyword. */
    static bool is_resource_keyword(std::string_view name) noexcept {
        return name == "$schema" || name == "$id" || name == "$anchor" ||
               name == "$dynamicAnchor" || name == "$vocabulary";
    }

    /**
     Reads the resource keywords of the node of `p`: `$schema` and `$vocabulary` for their shape,
     `$id`, which only the root may have yet, and `$anchor` and `$dynamicAnchor`, which name it.
    */
    result<void> read_resource_keywords(const pending& p, const boost::json::object& object) {
        if (const auto* schema = object.if_contains("$schema");
            schema != nullptr && !schema->is_string()) {
            return build_.refusal_at_member(errc::invalid_keyword_value, p.pointer, "$schema",
                                            "$schema");
        }
        if (const auto* vocabulary = object.if_contains("$vocabulary"); vocabulary != nullptr) {
            const auto* members = vocabulary->if_object();
            if (members == nullptr || !std::ranges::all_of(*members, [](const auto& member) {
                    return member.value().is_bool();
                })) {
                return build_.refusal_at_member(errc::invalid_keyword_value, p.pointer,
                                                "$vocabulary", "$vocabulary");
            }
        }
        if (p.node != build_.doc.resources()[static_cast<std::size_t>(p.resource)].root) {
            const auto id = read_id(p, object);
            if (!id) {
                return id.error();
            }
            if (*id) {
                return build_.refusal_at_member(errc::unresolvable_reference, p.pointer, "$id",
                                                (*id)->written);
            }
        }
        if (const auto* name = object.if_contains("$anchor")) {
            if (auto r = add_anchor(p, "$anchor", *name, false); !r) {
                return r;
            }
        }
        if (const auto* name = object.if_contains("$dynamicAnchor")) {
            if (auto r = add_anchor(p, "$dynamicAnchor", *name, true); !r) {
                return r;
            }
            return add_anchor(p, "$dynamicAnchor", *name, false);
        }
        return {};
    }

    /**
     Records an anchor of the node of `p`, named by the value of its keyword `keyword`.

     @return Nothing, or the refusal of a name that is no anchor's, or that another node of the
     resource already has (`errc::duplicate_anchor`).
    */
    result<void> add_anchor(const pending& p, std::string_view keyword,
                            const boost::json::value& value, bool dynamic) {
        const auto* text = value.if_string();
        if (text == nullptr || !is_anchor_name(*text)) {
            return build_.refusal_at_member(errc::invalid_keyword_value, p.pointer, keyword,
                                            keyword);
        }
        const ir::string_id name = build_.doc.intern(*text);
        const auto [at, added] =
            anchors_.try_emplace(anchor_key{p.resource, name, dynamic}, p.node);
        if (!added) {
            if (at->second == p.node) {
                return {};
            }
            return build_.refusal_at_member(errc::duplicate_anchor, p.pointer, keyword, *text);
        }
        build_.doc.add_anchor({
            .resource = p.resource,
            .name = name,
            .target = p.node,
            .dynamic = dynamic,
        });
        return {};
    }

    /** The row of `name` in the table of `resource`, or nullptr when no active vocabulary has it.
     */
    [[nodiscard]] const keyword_rule* rule_of(ir::resource_id resource,
                                              std::string_view name) const noexcept {
        const auto r = static_cast<std::size_t>(resource);
        const keyword_rule* rule = rule_named(resource_keywords_[r], name);
        if (rule == nullptr || (rule->vocabulary & build_.doc.resources()[r].vocabularies) == 0) {
            return nullptr;
        }
        return rule;
    }

    /**
     Reads the member `name` of the node of `p`, whose value `view` views, into a keyword: through
     its dialect's table, or as an unknown keyword, whose subschemas are not loaded.
    */
    result<void> read_member(const pending& p, ir::value_id view, std::string_view name,
                             const boost::json::value& v) {
        if (is_resource_keyword(name)) {
            return {};
        }
        if (!is_valid_utf8(name)) {
            return build_.refusal(errc::invalid_keyword_value, p.pointer, "a name not UTF-8");
        }
        const ir::string_id name_id = build_.doc.intern(name);
        const ir::value_id source = build_.view_of_member(view, name_id);
        const keyword_rule* rule = rule_of(p.resource, name);
        if (rule == nullptr) {
            keywords_.push_back({
                .kind = ir::keyword_kind::unknown,
                .name = name_id,
                .source = source,
                .data = ir::constant{.value = source},
            });
            return {};
        }
        const ir::pointer_id at = build_.member_pointer(p.pointer, name_id);
        auto data = build_.read_payload(*rule, p, v, at, source);
        if (!data) {
            return data.error();
        }
        keywords_.push_back({
            .kind = rule->kind,
            .name = name_id,
            .source = source,
            .data = *std::move(data),
        });
        return {};
    }

    std::span<const dialect_keywords> dialects_;
    const basic_compile_options<Traits>& options_;
    builder<Traits> build_;
    std::vector<ir::keyword, rebind<ir::keyword>> keywords_;
    std::vector<std::span<const keyword_rule>, rebind<std::span<const keyword_rule>>>
        resource_keywords_;
    boost::container::map<anchor_key, ir::node_id, anchor_less,
                          rebind<std::pair<const anchor_key, ir::node_id>>>
        anchors_;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_LOADER_WALK_HPP
