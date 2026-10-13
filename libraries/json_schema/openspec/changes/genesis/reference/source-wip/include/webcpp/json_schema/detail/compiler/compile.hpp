// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The compiler: lowers a document of the IR into a plan, one block per node it reaches, walking the
 nodes breadth first from the root, without recursion.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_COMPILE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_COMPILE_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/compiler/opcode.hpp>
#include <webcpp/json_schema/detail/compiler/plan.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/ir/document.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>
#include <webcpp/json_schema/traits.hpp>
#include <webcpp/json_schema/validator/options.hpp>

#include <boost/container/map.hpp>
#include <boost/throw_exception.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace webcpp::json_schema::detail {

/**
 Lowers the nodes of a document into a plan, a block per node, as the walk reaches them.

 A node's block is reserved the first time an instruction names it, and the blocks are lowered in
 the order they were reserved, so the plan's blocks follow a breadth-first walk from the root, and
 its block table is the walk's queue. Each keyword that applies to one type of instance opens
 with a `type_guard`. The keywords that one instruction serves (`properties`, `patternProperties`
 and `additionalProperties`; `prefixItems` and `items`; `if`, `then` and `else`; `contains`,
 `minContains` and `maxContains`) are lowered together, where the first of them stands, and their
 subschemas reach their blocks in the order the keywords stand.

 Every table, and every scratch one, is allocated through the traits' allocator; a pattern is
 compiled once for each distinct text, looked up by an ordered map, never a hash.

 @tparam Traits The traits of the document and of the plan.
*/
template <traits Traits>
class lowering {
public:
    /** The allocator of the plan and of the scratch tables. */
    using allocator_type = Traits::allocator_type;

    /** The document lowered. */
    using document_type = ir::basic_document<Traits>;

    /** The plan built. */
    using plan_type = plan<Traits>;

    /**
     An empty plan for `document`, in `allocator`.

     @param document The document, checked; it must outlive the lowering.
     @param allocator The allocator of the plan and of the scratch tables.
     @param regex_step_budget The step budget each pattern is compiled with.
    */
    lowering(const document_type& document, allocator_type allocator,
             std::size_t regex_step_budget)
        : document_(document),
          allocator_(allocator),
          regex_step_budget_(regex_step_budget),
          plan_{
              .code = table<instruction>(allocator),
              .blocks = table<block>(allocator),
              .locations = table<keyword_location>(allocator),
              .operands = table<std::uint32_t>(allocator),
              .regexes = table<regex_type>(allocator),
              .root_block = 0,
          },
          block_of_node_(document.size(), no_block, allocator),
          regex_of_text_(std::less<>(), rebind<std::pair<const ir::string_id, std::uint32_t>>(
                                            allocator)),
          names_(allocator),
          patterns_(allocator),
          where_(allocator) {}

    /**
     The block of `node`, reserved and queued the first time the node is reached.

     @param node The node.
     @return Its block's index.
    */
    std::uint32_t block_of(ir::node_id node) {
        std::uint32_t& known = block_of_node_[static_cast<std::size_t>(node)];
        if (known != no_block) {
            return known;
        }
        known = next_index(plan_.blocks.size(), 1);
        plan_.blocks.push_back({.first = 0, .count = 0, .node = node, .flags = 0});
        return known;
    }

    /**
     The next block reserved and not lowered yet.

     @return Its index, or nothing when every block reserved is lowered.
    */
    std::optional<std::uint32_t> next_queued() noexcept {
        if (lowered_ == plan_.blocks.size()) {
            return std::nullopt;
        }
        return lowered_++;
    }

    /**
     Lowers the node of the block `index`: `fail` for the schema `false`, each keyword in IR order,
     then `ret`.

     @param index The block, reserved.
     @return Nothing, or @ref errc::invalid_regex for a pattern the regex policy refuses, `where`
     its keyword's pointer and `what` the pattern.
    */
    result<void> lower_block(std::uint32_t index) {
        const ir::node_id node = plan_.blocks[index].node;
        const std::uint32_t first = next_index(plan_.code.size(), 0);
        lowered_groups_ = {};
        if (document_.at(node).kind == ir::node_kind::never_valid) {
            emit({.op = opcode::fail});
        }
        const std::span<const ir::keyword> keywords = document_.keywords(node);
        for (const ir::keyword& k : keywords) {
            if (auto r = lower_keyword(node, k); !r) {
                return r;
            }
        }
        emit({.op = opcode::ret});
        plan_.blocks[index].first = first;
        plan_.blocks[index].count = static_cast<std::uint32_t>(plan_.code.size() - first);
        return {};
    }

    /**
     Takes the plan.

     @return The plan; the lowering holds none after.
    */
    plan_type release() noexcept { return std::move(plan_); }

private:
    template <class T>
    using rebind = std::allocator_traits<allocator_type>::template rebind_alloc<T>;

    template <class T>
    using table = plan_type::template table<T>;

    using regex_type = plan_type::regex_type;

    using regex_policy = Traits::regex_policy;

    /** The fused instructions lowered in the current node, each where its first keyword stands. */
    struct groups {
        /** `loop_properties`. */
        bool properties = false;
        /** `loop_items`. */
        bool items = false;
        /** `if_then_else`. */
        bool conditional = false;
        /** `loop_contains`. */
        bool contains = false;
    };

    /**
     The index of the first of `added` entries appended to a table of `size`.

     Tip: 2^32 - 1 is never an index: it is @ref no_block and @ref no_location.
    */
    static std::uint32_t next_index(std::size_t size, std::size_t added) {
        constexpr std::size_t limit = 0xFFFF'FFFFU;
        if (added > limit || size > limit - added) {
            boost::throw_exception(std::length_error("webcpp::json_schema: a plan is full"));
        }
        return static_cast<std::uint32_t>(size);
    }

    /** The id of the keyword `k`, which belongs to `node`. */
    [[nodiscard]] ir::keyword_id id_of(ir::node_id node, const ir::keyword& k) const noexcept {
        const std::span<const ir::keyword> keywords = document_.keywords(node);
        return ir::keyword_id{document_.at(node).keywords.first +
                              static_cast<std::uint32_t>(&k - keywords.data())};
    }

    void emit(const instruction& in) {
        (void)next_index(plan_.code.size(), 1);
        plan_.code.push_back(in);
    }

    /** Adds the location of `k`, in `node`, and returns its index. */
    std::uint32_t locate(ir::node_id node, const ir::keyword& k) {
        const std::uint32_t at = next_index(plan_.locations.size(), 1);
        plan_.locations.push_back({.keyword = id_of(node, k), .node = node});
        return at;
    }

    /** Emits `in`, after the `type_guard` that skips it when the instance is not of `type`. */
    void emit_guarded(ir::type_set::bit type, const instruction& in) {
        emit({.op = opcode::type_guard, .small = type, .a = 1, .location = in.location});
        emit(in);
    }

    /** Appends `words` to the operands, and returns the index of the first. */
    std::uint32_t append(std::span<const std::uint32_t> words) {
        const std::uint32_t first = next_index(plan_.operands.size(), words.size());
        plan_.operands.insert(plan_.operands.end(), words.begin(), words.end());
        return first;
    }

    /** Appends one word to the operands, and returns its index. */
    std::uint32_t append(std::uint32_t word) { return append(std::span(&word, 1)); }

    /** The low word of a count. */
    static std::uint32_t low(std::uint64_t n) noexcept {
        return static_cast<std::uint32_t>(n & 0xFFFF'FFFFU);
    }

    /** The high word of a count. */
    static std::uint32_t high(std::uint64_t n) noexcept { return static_cast<std::uint32_t>(n >> 32U); }

    /** Lowers one keyword by its kind; a kind with no instruction yet lowers to nothing. */
    result<void> lower_keyword(ir::node_id node, const ir::keyword& k) {
        using kind = ir::keyword_kind;
        switch (k.kind) {
            case kind::type: lower_type(node, k); return {};
            case kind::const_:
            case kind::enum_: lower_constants(node, k); return {};
            case kind::multiple_of:
            case kind::maximum:
            case kind::exclusive_maximum:
            case kind::minimum:
            case kind::exclusive_minimum: lower_bound(node, k); return {};
            case kind::max_length:
            case kind::min_length:
            case kind::max_items:
            case kind::min_items:
            case kind::max_properties:
            case kind::min_properties: lower_count(node, k); return {};
            case kind::unique_items: lower_unique(node, k); return {};
            case kind::pattern: return lower_pattern(node, k);
            case kind::required:
            case kind::dependent_required: lower_required(node, k); return {};
            case kind::all_of:
            case kind::any_of:
            case kind::one_of: lower_list(node, k); return {};
            case kind::not_:
            case kind::property_names:
            case kind::dependent_schemas: lower_subschemas(node, k); return {};
            case kind::if_:
            case kind::then_:
            case kind::else_: lower_conditional(node); return {};
            case kind::properties:
            case kind::pattern_properties:
            case kind::additional_properties: return lower_properties(node);
            case kind::prefix_items:
            case kind::items: lower_items(node); return {};
            case kind::contains:
            case kind::min_contains:
            case kind::max_contains: lower_contains(node); return {};
            default: return {};
        }
    }

    /** `type`: `assert_type`, unguarded. */
    void lower_type(ir::node_id node, const ir::keyword& k) {
        const auto& t = std::get<ir::types>(k.data);
        emit({
            .op = opcode::assert_type,
            .flags = t.integer_by_value ? flag_bit : std::uint8_t{0},
            .small = t.set.bits,
            .location = locate(node, k),
        });
    }

    /** `const` and `enum`: the value, or the values in the operands. */
    void lower_constants(ir::node_id node, const ir::keyword& k) {
        if (k.kind == ir::keyword_kind::const_) {
            const auto value = static_cast<std::uint32_t>(std::get<ir::constant>(k.data).value);
            emit({.op = opcode::assert_const, .a = value, .location = locate(node, k)});
            return;
        }
        const std::span<const ir::value_id> values =
            document_.values(std::get<ir::constants>(k.data).values);
        const std::uint32_t first = next_index(plan_.operands.size(), values.size());
        for (const ir::value_id v : values) {
            plan_.operands.push_back(static_cast<std::uint32_t>(v));
        }
        emit({
            .op = opcode::assert_enum,
            .a = first,
            .b = static_cast<std::uint32_t>(values.size()),
            .location = locate(node, k),
        });
    }

    /** `multipleOf` and the four bounds: the number, guarded by `number`. */
    void lower_bound(ir::node_id node, const ir::keyword& k) {
        using kind = ir::keyword_kind;
        const opcode op = k.kind == kind::multiple_of         ? opcode::assert_multiple_of
                          : k.kind == kind::maximum           ? opcode::assert_maximum
                          : k.kind == kind::exclusive_maximum ? opcode::assert_exclusive_maximum
                          : k.kind == kind::minimum           ? opcode::assert_minimum
                                                              : opcode::assert_exclusive_minimum;
        const auto value = static_cast<std::uint32_t>(std::get<ir::bound>(k.data).value);
        emit_guarded(ir::type_set::number, {.op = op, .a = value, .location = locate(node, k)});
    }

    /** The limits on a size: the count's two words, `flags` set for a maximum. */
    void lower_count(ir::node_id node, const ir::keyword& k) {
        using kind = ir::keyword_kind;
        const bool maximum = k.kind == kind::max_length || k.kind == kind::max_items ||
                             k.kind == kind::max_properties;
        const bool length = k.kind == kind::max_length || k.kind == kind::min_length;
        const bool items = k.kind == kind::max_items || k.kind == kind::min_items;
        const ir::type_set::bit type = length  ? ir::type_set::string
                                       : items ? ir::type_set::array
                                               : ir::type_set::object;
        const opcode op = length  ? opcode::assert_length
                          : items ? opcode::assert_item_count
                                  : opcode::assert_property_count;
        const std::uint64_t n = std::get<ir::count>(k.data).value;
        emit_guarded(type, {
                               .op = op,
                               .flags = maximum ? flag_bit : std::uint8_t{0},
                               .a = low(n),
                               .b = high(n),
                               .location = locate(node, k),
                           });
    }

    /** `uniqueItems`: `assert_unique` when it is `true`, else nothing. */
    void lower_unique(ir::node_id node, const ir::keyword& k) {
        const boost::json::value& v = document_.value(std::get<ir::constant>(k.data).value);
        if (!v.is_bool() || !v.get_bool()) {
            return;
        }
        emit_guarded(ir::type_set::array, {.op = opcode::assert_unique, .location = locate(node, k)});
    }

    /** `pattern`: the regex, compiled, guarded by `string`. */
    result<void> lower_pattern(ir::node_id node, const ir::keyword& k) {
        const ir::string_id text = std::get<ir::text>(k.data).value;
        const auto regex = regex_of(node, k, text, false);
        if (!regex) {
            return regex.error();
        }
        emit_guarded(ir::type_set::string,
                     {.op = opcode::assert_pattern, .a = *regex, .location = locate(node, k)});
        return {};
    }

    /** `required`, the names in the operands, and `dependentRequired`, the IR's range. */
    void lower_required(ir::node_id node, const ir::keyword& k) {
        if (k.kind == ir::keyword_kind::dependent_required) {
            const ir::range r = std::get<ir::dependencies>(k.data).entries;
            emit_guarded(ir::type_set::object, {
                                                   .op = opcode::assert_dependent_required,
                                                   .a = r.first,
                                                   .b = r.count,
                                                   .location = locate(node, k),
                                               });
            return;
        }
        const std::span<const ir::string_id> names =
            document_.strings(std::get<ir::names>(k.data).strings);
        const std::uint32_t first = next_index(plan_.operands.size(), names.size());
        for (const ir::string_id name : names) {
            plan_.operands.push_back(static_cast<std::uint32_t>(name));
        }
        emit_guarded(ir::type_set::object, {
                                               .op = opcode::assert_required,
                                               .a = first,
                                               .b = static_cast<std::uint32_t>(names.size()),
                                               .location = locate(node, k),
                                           });
    }

    /** `allOf`, `anyOf` and `oneOf`: the children's blocks in the operands. */
    void lower_list(ir::node_id node, const ir::keyword& k) {
        using kind = ir::keyword_kind;
        const opcode op = k.kind == kind::all_of   ? opcode::all_of
                          : k.kind == kind::any_of ? opcode::any_of
                                                   : opcode::one_of;
        const std::span<const ir::node_id> children =
            document_.nodes(std::get<ir::subschema_list>(k.data).nodes);
        const std::uint32_t at = locate(node, k);
        const std::uint32_t first = reserve_blocks(children);
        emit({.op = op, .a = first, .b = static_cast<std::uint32_t>(children.size()), .location = at});
    }

    /** Reserves the blocks of `children`, in order, into the operands; returns the first's index. */
    std::uint32_t reserve_blocks(std::span<const ir::node_id> children) {
        const std::uint32_t first = next_index(plan_.operands.size(), children.size());
        for (const ir::node_id child : children) {
            const std::uint32_t b = block_of(child);
            plan_.operands.push_back(b);
        }
        return first;
    }

    /**
     `not`, `propertyNames` (guarded by `object`) and `dependentSchemas` (guarded by `object`, its
     (name, block) pairs in the operands).
    */
    void lower_subschemas(ir::node_id node, const ir::keyword& k) {
        const std::uint32_t at = locate(node, k);
        if (k.kind == ir::keyword_kind::not_) {
            const std::uint32_t b = block_of(std::get<ir::subschema>(k.data).schema);
            emit({.op = opcode::not_, .a = b, .location = at});
            return;
        }
        if (k.kind == ir::keyword_kind::property_names) {
            const std::uint32_t b = block_of(std::get<ir::subschema>(k.data).schema);
            emit_guarded(ir::type_set::object,
                         {.op = opcode::loop_property_names, .a = b, .location = at});
            return;
        }
        const std::span<const ir::named_subschema> entries =
            document_.entries(std::get<ir::named_subschemas>(k.data).entries);
        const std::uint32_t first = next_index(plan_.operands.size(), 2 * entries.size());
        for (const ir::named_subschema& e : entries) {
            const std::uint32_t b = block_of(e.schema);
            plan_.operands.push_back(static_cast<std::uint32_t>(e.name));
            plan_.operands.push_back(b);
        }
        emit_guarded(ir::type_set::object, {
                                               .op = opcode::loop_dependent_schemas,
                                               .a = first,
                                               .b = static_cast<std::uint32_t>(entries.size()),
                                               .location = at,
                                           });
    }

    /** The keywords of `node` of the kinds `kinds`, those it has, in the order they stand. */
    template <std::size_t N>
    [[nodiscard]] std::array<const ir::keyword*, N> in_order(
        ir::node_id node, const std::array<ir::keyword_kind, N>& kinds) const noexcept {
        std::array<const ir::keyword*, N> found{};
        std::ranges::transform(kinds, found.begin(),
                               [&](ir::keyword_kind kind) { return document_.find(node, kind); });
        // nullptr, for a keyword the node lacks, sorts last
        std::ranges::sort(found, [](const ir::keyword* x, const ir::keyword* y) {
            return y == nullptr ? x != nullptr : x != nullptr && std::less<>()(x, y);
        });
        return found;
    }

    /** The subschema of a keyword whose payload is one. */
    static ir::node_id child_of(const ir::keyword& k) noexcept {
        return std::get<ir::subschema>(k.data).schema;
    }

    /** `if`, `then` and `else`: one `if_then_else`, located at `if`; nothing without `if`. */
    void lower_conditional(ir::node_id node) {
        using kind = ir::keyword_kind;
        if (std::exchange(lowered_groups_.conditional, true)) {
            return;
        }
        const ir::keyword* condition = document_.find(node, kind::if_);
        if (condition == nullptr) {
            return;
        }
        std::array<std::uint32_t, 2> branches{no_block, no_block};
        std::uint32_t condition_block = no_block;
        for (const ir::keyword* k : in_order(node, std::array{kind::if_, kind::then_, kind::else_})) {
            if (k == nullptr) {
                break;
            }
            const std::uint32_t b = block_of(child_of(*k));
            if (k->kind == kind::if_) {
                condition_block = b;
            } else {
                branches[k->kind == kind::then_ ? 0 : 1] = b;
            }
        }
        const std::uint32_t at = locate(node, *condition);
        emit({.op = opcode::if_then_else, .a = condition_block, .b = append(branches), .location = at});
    }

    /**
     `properties`, `patternProperties` and `additionalProperties`: one `loop_properties`, guarded
     by `object`, located at the first of them.
    */
    result<void> lower_properties(ir::node_id node) {
        using kind = ir::keyword_kind;
        if (std::exchange(lowered_groups_.properties, true)) {
            return {};
        }
        const auto members = in_order(
            node, std::array{kind::properties, kind::pattern_properties, kind::additional_properties});
        names_.clear();
        patterns_.clear();
        std::uint32_t additional = no_block;
        for (const ir::keyword* k : members) {
            if (k == nullptr) {
                break;
            }
            if (k->kind == kind::additional_properties) {
                additional = block_of(child_of(*k));
                continue;
            }
            if (auto r = reserve_entries(node, *k); !r) {
                return r;
            }
        }
        // a name is looked up among the names by their bytes; the patterns keep their order
        std::ranges::sort(names_, [&](const auto& x, const auto& y) {
            return document_.string(ir::string_id{x.first}) <
                   document_.string(ir::string_id{y.first});
        });
        const std::uint32_t at = locate(node, *members[0]);
        const std::uint32_t name_table = append(static_cast<std::uint32_t>(names_.size()));
        append_pairs(names_);
        const std::uint32_t pattern_table = append(additional);
        (void)append(static_cast<std::uint32_t>(patterns_.size()));
        append_pairs(patterns_);
        emit_guarded(ir::type_set::object, {
                                               .op = opcode::loop_properties,
                                               .a = name_table,
                                               .b = pattern_table,
                                               .location = at,
                                           });
        return {};
    }

    /** Reserves the blocks of the entries of `properties`, or of `patternProperties` with their
        regexes, into the scratch pairs. */
    result<void> reserve_entries(ir::node_id node, const ir::keyword& k) {
        const bool patterns = k.kind == ir::keyword_kind::pattern_properties;
        for (const ir::named_subschema& e :
             document_.entries(std::get<ir::named_subschemas>(k.data).entries)) {
            const std::uint32_t b = block_of(e.schema);
            if (!patterns) {
                names_.emplace_back(static_cast<std::uint32_t>(e.name), b);
                continue;
            }
            const auto regex = regex_of(node, k, e.name, true);
            if (!regex) {
                return regex.error();
            }
            patterns_.emplace_back(*regex, b);
        }
        return {};
    }

    /** Appends the pairs to the operands, flat. */
    void append_pairs(std::span<const std::pair<std::uint32_t, std::uint32_t>> pairs) {
        (void)next_index(plan_.operands.size(), 2 * pairs.size());
        for (const auto& [first, second] : pairs) {
            plan_.operands.push_back(first);
            plan_.operands.push_back(second);
        }
    }

    /** `prefixItems` and `items`: one `loop_items`, guarded by `array`, at the first of them. */
    void lower_items(ir::node_id node) {
        using kind = ir::keyword_kind;
        if (std::exchange(lowered_groups_.items, true)) {
            return;
        }
        const auto members = in_order(node, std::array{kind::prefix_items, kind::items});
        std::uint32_t prefix = 0;
        std::uint32_t rest = no_block;
        bool counted = false;
        for (const ir::keyword* k : members) {
            if (k == nullptr) {
                break;
            }
            if (k->kind == kind::items) {
                rest = block_of(child_of(*k));
                continue;
            }
            const std::span<const ir::node_id> children =
                document_.nodes(std::get<ir::subschema_list>(k->data).nodes);
            prefix = append(static_cast<std::uint32_t>(children.size()));
            (void)reserve_blocks(children);
            counted = true;
        }
        if (!counted) {
            prefix = append(0U);
        }
        const std::uint32_t at = locate(node, *members[0]);
        emit_guarded(ir::type_set::array,
                     {.op = opcode::loop_items, .a = prefix, .b = rest, .location = at});
    }

    /**
     `contains`, with `minContains` (1 when absent) and `maxContains`: one `loop_contains`,
     guarded by `array`, located at `contains`, where the first of them stands; nothing without
     `contains`.
    */
    void lower_contains(ir::node_id node) {
        using kind = ir::keyword_kind;
        if (std::exchange(lowered_groups_.contains, true)) {
            return;
        }
        const ir::keyword* contains = document_.find(node, kind::contains);
        if (contains == nullptr) {
            return;
        }
        const ir::keyword* minimum = document_.find(node, kind::min_contains);
        const ir::keyword* maximum = document_.find(node, kind::max_contains);
        const std::uint64_t min = minimum == nullptr ? 1 : std::get<ir::count>(minimum->data).value;
        const std::uint64_t max = maximum == nullptr ? 0 : std::get<ir::count>(maximum->data).value;
        const std::uint32_t at = locate(node, *contains);
        const std::uint32_t b = block_of(std::get<ir::contains_payload>(contains->data).schema);
        const std::array<std::uint32_t, 4> bounds{low(min), high(min), low(max), high(max)};
        emit_guarded(ir::type_set::array, {
                                              .op = opcode::loop_contains,
                                              .flags = maximum == nullptr ? std::uint8_t{0} : flag_bit,
                                              .a = b,
                                              .b = append(bounds),
                                              .location = at,
                                          });
    }

    /**
     The regex of the pattern `text`, compiled the first time it is met.

     @param node The node of the keyword.
     @param k The keyword: `pattern`, or `patternProperties`, whose member `text` names.
     @param text The pattern.
     @param member Whether the pattern is a member's name, which the refusal's pointer ends with.
     @return Its index in the plan's regexes, or @ref errc::invalid_regex, `where` the pointer of
     the pattern and `what` the pattern.
    */
    result<std::uint32_t> regex_of(ir::node_id node, const ir::keyword& k, ir::string_id text,
                                   bool member) {
        if (const auto found = regex_of_text_.find(text); found != regex_of_text_.end()) {
            return found->second;
        }
        const std::string_view pattern = document_.string(text);
        auto compiled = regex_policy::compile(pattern, allocator_, regex_step_budget_);
        if (!compiled) {
            where_.clear();
            document_.write_pointer(document_.at(node).pointer, where_);
            append_token(document_.string(k.name));
            if (member) {
                append_token(pattern);
            }
            return make_diagnostic(errc::invalid_regex, where_, pattern);
        }
        const std::uint32_t index = next_index(plan_.regexes.size(), 1);
        plan_.regexes.push_back(std::move(*compiled));
        regex_of_text_.emplace(text, index);
        return index;
    }

    /** Appends to the refusal's pointer a slash and `name`, escaped as RFC 6901 writes it. */
    void append_token(std::string_view name) {
        where_.push_back('/');
        for (const char c : name) {
            if (c == '~' || c == '/') {
                where_.push_back('~');
                where_.push_back(c == '~' ? '0' : '1');
            } else {
                where_.push_back(c);
            }
        }
    }

    const document_type& document_;
    allocator_type allocator_;
    std::size_t regex_step_budget_;
    plan_type plan_;
    /** Each node's block, or @ref no_block before it is reached: by node id. */
    table<std::uint32_t> block_of_node_;
    /** How many blocks are lowered: the walk's queue is the blocks after them. */
    std::uint32_t lowered_ = 0;
    groups lowered_groups_{};
    /** Each pattern's regex, by its interned text, so a text is compiled once. */
    boost::container::map<ir::string_id, std::uint32_t, std::less<>,
                          rebind<std::pair<const ir::string_id, std::uint32_t>>>
        regex_of_text_;
    /** The (name, block) pairs of `properties`, sorted before they are appended. */
    table<std::pair<std::uint32_t, std::uint32_t>> names_;
    /** The (regex, block) pairs of `patternProperties`. */
    table<std::pair<std::uint32_t, std::uint32_t>> patterns_;
    /** The pointer of a refused pattern, spelled. */
    basic_string<Traits> where_;
};

/**
 Compiles a document into a plan: a block for each node the walk from the root reaches, numbered
 breadth first, keywords in IR order.

 Nothing recurses on the schema's depth: the walk's queue is the plan's block table. The plan
 holds memory linear in the document; every allocation, scratch included, goes through
 `allocator`.

 @tparam Traits The traits of the document.
 @param document The document, which @ref ir::basic_document::check accepts.
 @param allocator The allocator of the plan, its compiled patterns and the scratch.
 @param options The options; the compiler reads `regex_step_budget`.
 @return The plan, or @ref errc::invalid_regex for the first pattern, in the walk's order, that
 the regex policy refuses, `where` its pointer and `what` the pattern.
 @throws std::bad_alloc When the allocator throws it; without exceptions, the program ends.
*/
template <traits Traits>
result<plan<Traits>> compile_plan(
    const ir::basic_document<Traits>& document, typename Traits::allocator_type allocator,
    const basic_compile_options<Traits>& options = basic_compile_options<Traits>()) {
    lowering<Traits> lower(document, allocator, options.regex_step_budget);
    (void)lower.block_of(document.root());
    while (const auto next = lower.next_queued()) {
        if (auto r = lower.lower_block(*next); !r) {
            return r.error();
        }
    }
    return lower.release();
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_COMPILE_HPP
