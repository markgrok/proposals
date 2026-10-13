// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 A compiled plan: a flat list of instructions in blocks, one block per compiled node, and the
 tables its instructions name by index.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_PLAN_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_PLAN_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/compiler/opcode.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace webcpp::json_schema::detail {

/** No block: an absent `then`, `else`, `additionalProperties` or `items`. */
inline constexpr std::uint32_t no_block = 0xFFFF'FFFFU;

/** No location: an instruction that belongs to no keyword, `ret` and `fail`. */
inline constexpr std::uint32_t no_location = 0xFFFF'FFFFU;

/** The bit of an instruction's `flags` that @ref opcode documents for it. */
inline constexpr std::uint8_t flag_bit = 1;

/** One instruction: its opcode, its operands as @ref opcode says, and its keyword's location. */
struct instruction {
    /** What it does. */
    opcode op = opcode::ret;

    /** A bit its opcode reads. */
    std::uint8_t flags = 0;

    /** A short operand: a type's bits. */
    std::uint16_t small = 0;

    /** The first operand: an index into one of the plan's tables, or a value. */
    std::uint32_t a = 0;

    /** The second operand. */
    std::uint32_t b = 0;

    /** The index of its keyword's location in the plan's `locations`, or @ref no_location. */
    std::uint32_t location = no_location;
};

/** The code of one compiled node: `count` instructions from `first`, the last one `ret`. */
struct block {
    /** The index of its first instruction. */
    std::uint32_t first = 0;

    /** How many instructions it holds. */
    std::uint32_t count = 0;

    /** The node it was compiled from. */
    ir::node_id node{};

    /** Nothing yet; 0. */
    std::uint32_t flags = 0;
};

/**
 Where an instruction's keyword is, as indices only: rendered when an error is built, from the
 node's resource and pointer and the keyword's name.
*/
struct keyword_location {
    /** The keyword. */
    ir::keyword_id keyword{};

    /** The node it belongs to. */
    ir::node_id node{};
};

/**
 A compiled plan: the instructions, grouped in blocks, and the tables they name.

 It holds indices only, never an address, so it can be copied, or moved to another allocator,
 whole. Every table allocates through the traits' allocator: `std::pmr::vector` with the default
 traits.

 @tparam Traits The traits of the document it was compiled from.
*/
template <traits Traits>
struct plan {
    /** The allocator of every table. */
    using allocator_type = Traits::allocator_type;

    /** A table of the plan, in its allocator. */
    template <class T>
    using table =
        std::vector<T, typename std::allocator_traits<allocator_type>::template rebind_alloc<T>>;

    /** A pattern, compiled by the traits' regex policy in the plan's allocator. */
    using regex_type = Traits::regex_policy::template compiled<allocator_type>;

    /** The instructions, block after block. */
    table<instruction> code;

    /** The blocks, numbered in the order a breadth-first walk from the root reaches them. */
    table<block> blocks;

    /** The keywords' locations, one per keyword compiled. */
    table<keyword_location> locations;

    /** The lists the instructions name: blocks, value ids, string ids, pairs and counts. */
    table<std::uint32_t> operands;

    /** The patterns, one per distinct text. */
    table<regex_type> regexes;

    /** The root's block: 0. */
    std::uint32_t root_block = 0;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_PLAN_HPP
