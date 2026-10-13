// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 What the loader builds a document with: the document, the values waiting to be walked, and the
 readers that turn a keyword's value into its payload, refusing a value of the wrong shape.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_LOADER_READERS_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_LOADER_READERS_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/dialects/keyword_table.hpp>
#include <webcpp/json_schema/detail/json/exact_handler.hpp>
#include <webcpp/json_schema/detail/json/utf8.hpp>
#include <webcpp/json_schema/detail/numbers/arithmetic.hpp>
#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/ir/document.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>
#include <webcpp/json_schema/ir/number.hpp>
#include <webcpp/json_schema/numbers/number.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <boost/json/value.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace webcpp::json_schema::detail {

/** A value in a schema position, its node reserved, waiting to be walked. */
struct pending {
    /** The value, inside the document's one copy of the schema. */
    const boost::json::value* value = nullptr;

    /** Its node. */
    ir::node_id node{};

    /** The resource its node belongs to. */
    ir::resource_id resource{};

    /** How many schemas enclose it: 0 for the root. */
    std::uint32_t depth = 0;

    /** Its JSON Pointer from the document's root. */
    ir::pointer_id pointer = ir::empty_pointer;

    /** The document's view of the value, or @ref ir::no_value until the walk makes it. */
    ir::value_id view = ir::no_value;

    /** The view of the value it lies in, one step above it: its keyword's, or its own. */
    ir::value_id parent_view = ir::no_value;
};

/**
 The document a load builds, the values it has still to walk, and the readers of keyword values.

 Every reader takes a keyword's rule, the node it is in, its value, its pointer and the view of its
 value, and returns its payload or the diagnostic of its refusal: @ref errc::invalid_keyword_value
 for a value of the wrong shape, `where` the keyword's pointer and `what` its name. A reader that
 meets a subschema reserves its node and queues it, so that nothing recurses on the schema's depth.

 A pointer is one step added to its parent's, and a view one step inside its parent's view, so a
 load holds memory linear in its schema however long its keys and however deep its nesting; a
 pointer is spelled only where a refusal reports it.

 Tip: every scratch table lives in the document's allocator, and is cleared, not freed, between
 two keywords, so it grows to what the largest keyword needs and is allocated no more after.
*/
template <class Traits>
class builder {
    template <class T>
    using rebind = std::allocator_traits<typename Traits::allocator_type>::template rebind_alloc<T>;

    template <class T>
    using table = std::vector<T, rebind<T>>;

public:
    /** The document built. */
    using document_type = ir::basic_document<Traits>;

    /** The allocator of the document and of every scratch table. */
    using allocator_type = Traits::allocator_type;

    /** The schema's number type. */
    using number_type = Traits::number_type;

    /** An empty document, and empty tables, in `allocator`. */
    explicit builder(allocator_type allocator)
        : doc(allocator),
          work(rebound<pending>(allocator)),
          nodes_(rebound<ir::node_id>(allocator)),
          entries_(rebound<ir::named_subschema>(allocator)),
          strings_(rebound<ir::string_id>(allocator)),
          sorted_(rebound<ir::string_id>(allocator)),
          dependencies_(rebound<ir::dependency>(allocator)),
          values_(rebound<ir::value_id>(allocator)),
          member_steps_(std::less<>(),
                        rebound<std::pair<const ir::string_id, ir::pointer_id>>(allocator)),
          element_steps_(rebound<ir::pointer_id>(allocator)),
          exact_nodes_(rebound<std::size_t>(allocator)),
          chain_(rebound<ir::pointer_id>(allocator)),
          path_(rebound<char>(allocator)),
          token_(rebound<char>(allocator)) {}

    /**
     Copies `root` into the document, once: every value the walk reads is inside this copy.

     @return The copy's id.
    */
    ir::value_id copy(const boost::json::value& root) { return doc.add_value(root); }

    /** Reads the exact numbers of `exact`, or of nothing; `exact` must outlive the builder. */
    void use_exact_numbers(const exact_table<Traits>* exact) noexcept { exact_ = exact; }

    /** The pointer `parent`, then the member `name`. */
    ir::pointer_id member_pointer(ir::pointer_id parent, ir::string_id name) {
        return doc.add_pointer(parent, name);
    }

    /** The view of the member `name` of the value `base` views. */
    ir::value_id view_of_member(ir::value_id base, ir::string_id name) {
        auto [step, added] = member_steps_.try_emplace(name, ir::empty_pointer);
        if (added) {
            step->second = doc.add_pointer(ir::empty_pointer, name);
        }
        return doc.add_value_at(base, step->second);
    }

    /** The view of the element `index` of the value `base` views. */
    ir::value_id view_of_element(ir::value_id base, std::uint32_t index) {
        while (element_steps_.size() <= index) {
            element_steps_.push_back(doc.add_pointer(
                ir::empty_pointer, static_cast<std::uint32_t>(element_steps_.size())));
        }
        return doc.add_value_at(base, element_steps_[index]);
    }

    /**
     The view of the value of `p`: the one it was queued with, else one step inside its parent's
     view, by the last step of its pointer.
    */
    ir::value_id view_of(const pending& p) {
        if (p.view != ir::no_value) {
            return p.view;
        }
        const ir::pointer_step last = doc.step(p.pointer);
        return last.element ? view_of_element(p.parent_view, last.index)
                            : view_of_member(p.parent_view, last.name);
    }

    /**
     The diagnostic of `code` at the member `name` of the value at `parent`, `what` as given.

     Tip: the member's pointer is spelled in scratch and never added: a refusal adds no pointer to
     the document.
    */
    diagnostic refusal_at_member(errc code, ir::pointer_id parent, std::string_view name,
                                 std::string_view what) {
        path_.clear();
        doc.write_pointer(parent, path_);
        path_.push_back('/');
        for (const char c : name) {
            if (c == '~') {
                path_.append("~0");
            } else if (c == '/') {
                path_.append("~1");
            } else {
                path_.push_back(c);
            }
        }
        return make_diagnostic(code, path_, what);
    }

    /** The diagnostic of `code` at the pointer `at`, spelled, `what` as given. */
    diagnostic refusal(errc code, ir::pointer_id at, std::string_view what) {
        path_.clear();
        doc.write_pointer(at, path_);
        return make_diagnostic(code, path_, what);
    }

    /**
     The payload of the keyword of `rule` whose value `v` is at `at`, in the node of `p`.

     @return The payload, or the diagnostic of its refusal.
    */
    result<ir::payload> read_payload(const keyword_rule& rule, const pending& p,
                                     const boost::json::value& v, ir::pointer_id at,
                                     ir::value_id source) {
        switch (rule.read) {
            case reader::schema:
                return ir::payload(ir::subschema{.schema = child(p, v, at, source, source)});
            case reader::schema_list:
            case reader::nonempty_schema_list: return read_schema_list(rule, p, v, at, source);
            case reader::schema_map:
            case reader::pattern_map: return read_schema_map(rule, p, v, at, source);
            case reader::types: return read_types(rule, v, at);
            case reader::constant: return read_constant(rule, v, at, source);
            case reader::constants: return read_constants(rule, v, at, source);
            case reader::bound:
            case reader::positive_bound: return read_bound(rule, v, at);
            case reader::count: return read_count(rule, v, at);
            case reader::text:
            case reader::pattern: return read_text(rule, v, at);
            case reader::names: return read_names(rule, v, at);
            case reader::dependent_required: return read_dependent_required(rule, v, at);
            case reader::format: return read_format(rule, v, at);
            case reader::contains:
                return ir::payload(ir::contains_payload{
                    .schema = child(p, v, at, source, source),
                    .annotates_items = true,
                });
            case reader::ref:
            case reader::dynamic_ref: return read_reference(rule, v, at);
            case reader::annotation: return read_annotation(rule, v, at, source);
        }
        return invalid(rule, at);
    }

    /**
     A text of the schema as a string of the document, or the refusal of text that is not UTF-8,
     which no string of the document may hold (invariant 8).
    */
    result<ir::string_id> intern(const keyword_rule& rule, std::string_view text,
                                 ir::pointer_id at) {
        if (!is_valid_utf8(text)) {
            return invalid(rule, at);
        }
        return doc.intern(text);
    }

    /** The document built. */
    document_type doc;

    /** The values still to walk, the next one last. */
    table<pending> work;

private:
    template <class T>
    static rebind<T> rebound(const allocator_type& allocator) noexcept {
        return rebind<T>(allocator);
    }

    /** The refusal of `rule`'s value: invalid_keyword_value, at its pointer, naming it. */
    diagnostic invalid(const keyword_rule& rule, ir::pointer_id at) {
        return refusal(errc::invalid_keyword_value, at, rule.name);
    }

    /**
     Reserves the node of the subschema `v` at `at`, inside the node of `p`, and queues it with its
     view, when it is known, else the view one step above it.
    */
    ir::node_id child(const pending& p, const boost::json::value& v, ir::pointer_id at,
                      ir::value_id view, ir::value_id parent_view) {
        const ir::node_id node = doc.reserve(p.resource, at);
        work.push_back({
            .value = &v,
            .node = node,
            .resource = p.resource,
            .depth = p.depth + 1,
            .pointer = at,
            .view = view,
            .parent_view = parent_view,
        });
        return node;
    }

    /** Whether an array's indices fit a pointer's step, as every array the document holds does. */
    static bool indexable(const boost::json::array& array) noexcept {
        return array.size() <= std::numeric_limits<std::uint32_t>::max();
    }

    /** `allOf`, `anyOf`, `oneOf`, `prefixItems`: an array of subschemas, non-empty if asked. */
    result<ir::payload> read_schema_list(const keyword_rule& rule, const pending& p,
                                         const boost::json::value& v, ir::pointer_id at,
                                         ir::value_id source) {
        const auto* array = v.if_array();
        if (array == nullptr || !indexable(*array) ||
            (rule.read == reader::nonempty_schema_list && array->empty())) {
            return invalid(rule, at);
        }
        nodes_.clear();
        for (std::uint32_t i = 0; i < array->size(); ++i) {
            nodes_.push_back(child(p, (*array)[i], doc.add_pointer(at, i), ir::no_value, source));
        }
        return ir::payload(ir::subschema_list{.nodes = doc.add_nodes(nodes_)});
    }

    /**
     `$defs`, `properties`, `patternProperties`, `dependentSchemas`: an object of subschemas, in
     source order; a pattern is compiled by the compiler, not here.
    */
    result<ir::payload> read_schema_map(const keyword_rule& rule, const pending& p,
                                        const boost::json::value& v, ir::pointer_id at,
                                        ir::value_id source) {
        const auto* object = v.if_object();
        if (object == nullptr) {
            return invalid(rule, at);
        }
        entries_.clear();
        for (const auto& member : *object) {
            const auto name = intern(rule, member.key(), at);
            if (!name) {
                return name.error();
            }
            const ir::node_id schema =
                child(p, member.value(), member_pointer(at, *name), ir::no_value, source);
            entries_.push_back({.name = *name, .schema = schema});
        }
        return ir::payload(ir::named_subschemas{.entries = doc.add_entries(entries_)});
    }

    /** The bit of the type `name`, or nothing for a name that is no type. */
    static std::optional<ir::type_set::bit> type_named(std::string_view name) noexcept {
        using b = ir::type_set;
        if (name == "null") {
            return b::null;
        }
        if (name == "boolean") {
            return b::boolean;
        }
        if (name == "object") {
            return b::object;
        }
        if (name == "array") {
            return b::array;
        }
        if (name == "number") {
            return b::number;
        }
        if (name == "string") {
            return b::string;
        }
        if (name == "integer") {
            return b::integer;
        }
        return std::nullopt;
    }

    /** `type`: a type name, or a non-empty array of distinct ones; integer by value. */
    [[nodiscard]] result<ir::payload> read_types(const keyword_rule& rule,
                                                 const boost::json::value& v, ir::pointer_id at) {
        ir::type_set set{};
        const auto add = [&](const boost::json::value& name) {
            const auto* text = name.if_string();
            const auto bit = text == nullptr ? std::nullopt : type_named(*text);
            if (!bit || set.contains(*bit)) {
                return false;
            }
            set.bits = static_cast<std::uint8_t>(set.bits | *bit);
            return true;
        };
        if (const auto* array = v.if_array()) {
            if (array->empty() || !std::ranges::all_of(*array, add)) {
                return invalid(rule, at);
            }
        } else if (!add(v)) {
            return invalid(rule, at);
        }
        return ir::payload(ir::types{.set = set, .integer_by_value = true});
    }

    /** `const`, any value, and `uniqueItems`, a boolean: a view of it. */
    result<ir::payload> read_constant(const keyword_rule& rule, const boost::json::value& v,
                                      ir::pointer_id at, ir::value_id source) {
        if (rule.kind == ir::keyword_kind::unique_items && !v.is_bool()) {
            return invalid(rule, at);
        }
        return ir::payload(ir::constant{.value = source});
    }

    /** `enum`: an array, a view of each element. */
    result<ir::payload> read_constants(const keyword_rule& rule, const boost::json::value& v,
                                       ir::pointer_id at, ir::value_id source) {
        const auto* array = v.if_array();
        if (array == nullptr || !indexable(*array)) {
            return invalid(rule, at);
        }
        values_.clear();
        for (std::uint32_t i = 0; i < array->size(); ++i) {
            values_.push_back(view_of_element(source, i));
        }
        return ir::payload(ir::constants{.values = doc.add_values(values_)});
    }

    /** No node of the table of exact numbers: a step not looked up yet, and one the table lacks. */
    static constexpr std::size_t unknown_node = std::numeric_limits<std::size_t>::max();
    static constexpr std::size_t absent_node = unknown_node - 1;

    /** Spells into `token_` the token of the step `s`, escaped as the table of exact numbers holds
     * it. */
    void spell_token(const ir::pointer_step& s) {
        token_.clear();
        if (s.element) {
            std::array<char, 16> digits{};
            const auto written =
                std::to_chars(digits.data(), digits.data() + digits.size(), s.index);
            token_.append(digits.data(), written.ptr);
            return;
        }
        for (const char c : doc.string(s.name)) {
            if (c == '~') {
                token_.append("~0");
            } else if (c == '/') {
                token_.append("~1");
            } else {
                token_.push_back(c);
            }
        }
    }

    /**
     The node of the table of exact numbers the pointer `at` names, or @ref absent_node.

     Tip: each step's node is looked up once and kept, from the nearest step whose node is known,
     so the lookups of a load cost each step's token once, however long the pointers.
    */
    std::size_t exact_node(ir::pointer_id at) {
        if (at == ir::empty_pointer) {
            return exact_table<Traits>::root;
        }
        const auto index = [](ir::pointer_id id) { return static_cast<std::size_t>(id); };
        if (exact_nodes_.size() <= index(at)) {
            exact_nodes_.resize(index(at) + 1, unknown_node);
        }
        chain_.clear();
        ir::pointer_id up = at;
        while (up != ir::empty_pointer && exact_nodes_[index(up)] == unknown_node) {
            chain_.push_back(up);
            up = doc.step(up).parent;
        }
        std::size_t node =
            up == ir::empty_pointer ? exact_table<Traits>::root : exact_nodes_[index(up)];
        for (auto step = chain_.rbegin(); step != chain_.rend(); ++step) {
            if (node != absent_node) {
                spell_token(doc.step(*step));
                node = exact_->child_of(node, token_).value_or(absent_node);
            }
            exact_nodes_[index(*step)] = node;
        }
        return node;
    }

    /**
     The exact `Number` the text at `at` was read as, when the table of exact numbers holds one
     for it that still counts: the double there is the one it was recorded with.

     @return Nothing when no entry counts; an entry whose text no `Number` holds is
     `errc::invalid_number`.
    */
    [[nodiscard]] result<std::optional<number_type>> exact_number(const keyword_rule& rule,
                                                                  double d, ir::pointer_id at) {
        if (exact_ == nullptr || exact_->empty()) {
            return std::optional<number_type>{};
        }
        const auto* entry = exact_->entry_at(exact_node(at));
        if (entry == nullptr || entry->read_as != d) {
            return std::optional<number_type>{};
        }
        if (!entry->representable) {
            return refusal(errc::invalid_number, at, rule.name);
        }
        return std::optional<number_type>(entry->value);
    }

    /**
     A number as invariant 7 keeps it: an integer that fits stays one, any other value is a
     `Number`, read from its text when the schema was parsed by `parse`, else from its double's
     shortest text.
    */
    [[nodiscard]] result<ir::number<number_type>> number_of(const keyword_rule& rule,
                                                            const boost::json::value& v,
                                                            ir::pointer_id at) {
        if (const auto* i = v.if_int64()) {
            return ir::number<number_type>{std::in_place_index<0>, *i};
        }
        if (const auto* u = v.if_uint64()) {
            return ir::number<number_type>{std::in_place_index<1>, *u};
        }
        const auto* d = v.if_double();
        if (d == nullptr) {
            return invalid(rule, at);
        }
        const auto exact = exact_number(rule, *d, at);
        if (!exact) {
            return exact.error();
        }
        if (*exact) {
            const ir::number<number_type> n{std::in_place_index<2>, **exact};
            return integer_of<number_type>(decompose(n)).value_or(n);
        }
        if (auto n = schema_number_of_double<number_type>(*d)) {
            return *std::move(n);
        }
        return refusal(errc::invalid_number, at, rule.name);
    }

    /** `maximum` and the other bounds: a number; `multipleOf`: a number above zero. */
    result<ir::payload> read_bound(const keyword_rule& rule, const boost::json::value& v,
                                   ir::pointer_id at) {
        const auto n = number_of(rule, v, at);
        if (!n) {
            return n.error();
        }
        if (rule.read == reader::positive_bound &&
            compare(decompose(*n), decomposed{}) != std::strong_ordering::greater) {
            return invalid(rule, at);
        }
        return ir::payload(ir::bound{.value = doc.add_number(*n)});
    }

    /**
     `maxLength` and the other counts: a non-negative integer, written with a fraction of zero
     too (`2.0`).

     Tip: an integer past `std::uint64_t` is held as its largest value, which no size reaches, so
     that every count the meta-schema accepts loads, and means what it says.
    */
    [[nodiscard]] result<ir::payload> read_count(const keyword_rule& rule,
                                                 const boost::json::value& v, ir::pointer_id at) {
        if (const auto* i = v.if_int64()) {
            if (*i < 0) {
                return invalid(rule, at);
            }
            return ir::payload(ir::count{.value = static_cast<std::uint64_t>(*i)});
        }
        if (const auto* u = v.if_uint64()) {
            return ir::payload(ir::count{.value = *u});
        }
        const auto* d = v.if_double();
        if (d == nullptr) {
            return invalid(rule, at);
        }
        const auto exact = exact_number(rule, *d, at);
        if (!exact) {
            return exact.error();
        }
        const auto value =
            *exact ? std::optional<decomposed>(number_traits<number_type>::decompose(**exact))
                   : decompose_double(*d);
        if (!value) {
            return refusal(errc::invalid_number, at, rule.name);
        }
        const magnitude m = normalize(*value);
        if (m.coefficient == 0U) {
            return ir::payload(ir::count{.value = 0});
        }
        if (value->negative || m.exponent < 0) {
            return invalid(rule, at);
        }
        if (const auto integer = integer_of<number_type>(*value)) {
            if (const auto* small = std::get_if<0>(&*integer)) {
                return ir::payload(ir::count{.value = static_cast<std::uint64_t>(*small)});
            }
            if (const auto* large = std::get_if<1>(&*integer)) {
                return ir::payload(ir::count{.value = *large});
            }
        }
        return ir::payload(ir::count{.value = std::numeric_limits<std::uint64_t>::max()});
    }

    /** `pattern`, `contentEncoding`, `contentMediaType`: a string. */
    result<ir::payload> read_text(const keyword_rule& rule, const boost::json::value& v,
                                  ir::pointer_id at) {
        const auto* text = v.if_string();
        if (text == nullptr) {
            return invalid(rule, at);
        }
        const auto value = intern(rule, *text, at);
        if (!value) {
            return value.error();
        }
        return ir::payload(ir::text{.value = *value});
    }

    /** `format`: a string, the format's name, which annotates. */
    result<ir::payload> read_format(const keyword_rule& rule, const boost::json::value& v,
                                    ir::pointer_id at) {
        const auto* text = v.if_string();
        if (text == nullptr) {
            return invalid(rule, at);
        }
        const auto name = intern(rule, *text, at);
        if (!name) {
            return name.error();
        }
        return ir::payload(ir::format_payload{.name = *name, .assertion = false});
    }

    /**
     An array of distinct strings into `strings_`, in source order.

     Tip: equal texts are one string of the document, so distinct ids are distinct texts, and a
     sort of the ids finds two equal ones in `O(n log n)`.
    */
    result<void> read_distinct_strings(const keyword_rule& rule, const boost::json::value& v,
                                       ir::pointer_id at) {
        const auto* array = v.if_array();
        if (array == nullptr) {
            return invalid(rule, at);
        }
        strings_.clear();
        for (const boost::json::value& element : *array) {
            const auto* text = element.if_string();
            if (text == nullptr) {
                return invalid(rule, at);
            }
            const auto id = intern(rule, *text, at);
            if (!id) {
                return id.error();
            }
            strings_.push_back(*id);
        }
        sorted_.assign(strings_.begin(), strings_.end());
        std::ranges::sort(sorted_);
        if (std::ranges::adjacent_find(sorted_) != sorted_.end()) {
            return invalid(rule, at);
        }
        return {};
    }

    /** `required`: an array of distinct strings. */
    result<ir::payload> read_names(const keyword_rule& rule, const boost::json::value& v,
                                   ir::pointer_id at) {
        if (const auto r = read_distinct_strings(rule, v, at); !r) {
            return r.error();
        }
        return ir::payload(ir::names{.strings = doc.add_strings(strings_)});
    }

    /** `dependentRequired`: an object whose every member is an array of distinct strings. */
    result<ir::payload> read_dependent_required(const keyword_rule& rule,
                                                const boost::json::value& v, ir::pointer_id at) {
        const auto* object = v.if_object();
        if (object == nullptr) {
            return invalid(rule, at);
        }
        dependencies_.clear();
        for (const auto& member : *object) {
            const auto name = intern(rule, member.key(), at);
            if (!name) {
                return name.error();
            }
            if (const auto r = read_distinct_strings(rule, member.value(), at); !r) {
                return r.error();
            }
            dependencies_.push_back({.name = *name, .strings = doc.add_strings(strings_)});
        }
        return ir::payload(ir::dependencies{.entries = doc.add_dependencies(dependencies_)});
    }

    /**
     `$ref` and `$dynamicRef`: a string, refused with `errc::unresolvable_reference` naming the
     URI as written, since no reference resolves yet.
    */
    [[nodiscard]] result<ir::payload> read_reference(const keyword_rule& rule,
                                                     const boost::json::value& v,
                                                     ir::pointer_id at) {
        const auto* uri = v.if_string();
        if (uri == nullptr) {
            return invalid(rule, at);
        }
        return refusal(errc::unresolvable_reference, at, *uri);
    }

    /**
     An annotation, of the shape its meta-schema gives: a string for `$comment`, `title` and
     `description`, a boolean for `deprecated`, `readOnly` and `writeOnly`, an array for
     `examples`, any value for `default`; a view of it.
    */
    result<ir::payload> read_annotation(const keyword_rule& rule, const boost::json::value& v,
                                        ir::pointer_id at, ir::value_id source) {
        using k = ir::keyword_kind;
        bool shaped = true;
        switch (rule.kind) {
            case k::comment:
            case k::title:
            case k::description: shaped = v.is_string(); break;
            case k::deprecated:
            case k::read_only:
            case k::write_only: shaped = v.is_bool(); break;
            case k::examples: shaped = v.is_array(); break;
            default: break;
        }
        if (!shaped) {
            return invalid(rule, at);
        }
        return ir::payload(ir::constant{.value = source});
    }

    const exact_table<Traits>* exact_ = nullptr;
    table<ir::node_id> nodes_;
    table<ir::named_subschema> entries_;
    table<ir::string_id> strings_;
    table<ir::string_id> sorted_;
    table<ir::dependency> dependencies_;
    table<ir::value_id> values_;
    // The steps that name a member or an element one step inside a view, one for each name or
    // index, shared by every view that takes it.
    boost::container::map<ir::string_id, ir::pointer_id, std::less<>,
                          rebind<std::pair<const ir::string_id, ir::pointer_id>>>
        member_steps_;
    table<ir::pointer_id> element_steps_;
    // The node of the table of exact numbers each pointer names, by its step, once looked up.
    table<std::size_t> exact_nodes_;
    table<ir::pointer_id> chain_;
    basic_string<Traits> path_;
    basic_string<Traits> token_;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_LOADER_READERS_HPP
