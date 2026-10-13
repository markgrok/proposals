// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 JSON equality, as `const`, `enum` and `uniqueItems` read it: numbers by their exact value across
 kinds, objects whatever their members' order, arrays in order, strings by their bytes.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_EQUAL_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_EQUAL_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/memory/inline_stack.hpp>
#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>

#include <boost/json/value.hpp>

#include <cmath>
#include <cstddef>
#include <memory_resource>

namespace webcpp::json_schema::detail {

/** Two values still to compare. */
struct value_pair {
    /** The value of the first operand. */
    const boost::json::value* left;

    /** The value of the second operand at the same place. */
    const boost::json::value* right;
};

/** The pairs still to compare, the first 32 inline. */
using pair_stack = inline_stack<value_pair, 32>;

/** Whether two arrays may be equal, pushing the pairs of their elements when they may. */
inline bool equal_arrays(const boost::json::array& l, const boost::json::array& r,
                         pair_stack& work) {
    if (l.size() != r.size()) {
        return false;
    }
    for (std::size_t i = 0; i < l.size(); ++i) {
        work.push_back({.left = &l[i], .right = &r[i]});
    }
    return true;
}

/**
 Whether two objects may be equal, pushing the pairs of their members' values when they may.

 Tip: a Boost.JSON object's keys are unique, so objects of one size whose every key of one is a
 key of the other have the same keys.
*/
inline bool equal_objects(const boost::json::object& l, const boost::json::object& r,
                          pair_stack& work) {
    if (l.size() != r.size()) {
        return false;
    }
    for (const auto& member : l) {
        const auto* other = r.if_contains(member.key());
        if (other == nullptr) {
            return false;
        }
        work.push_back({.left = &member.value(), .right = other});
    }
    return true;
}

/**
 Whether two doubles that are not finite are one: the same infinity, or both NaN.

 Tip: JSON writes neither, but Boost.JSON holds both, and parses a number past the largest double
 as an infinity. A NaN is equal to a NaN so that equality stays an equivalence, as `enum` and
 `uniqueItems` need.
*/
inline bool same_non_finite(double a, double b) noexcept {
    return a == b || (std::isnan(a) && std::isnan(b));
}

/**
 One pair: true when it is equal so far, having pushed the pairs of its children.

 Numbers compare by their exact decimals (`decompose_value`, `compare`): an int64, a uint64 and a
 finite double are equal when their values are, so `1` equals `1.0`, `-0.0` equals `0`, and a
 double is the decimal its shortest text denotes.
*/
inline bool equal_step(const value_pair& p, pair_stack& work) {
    const auto left = decompose_value(*p.left);
    const auto right = decompose_value(*p.right);
    if (left || right) {
        return left && right && compare(*left, *right) == 0;
    }
    if (p.left->kind() != p.right->kind()) {
        return false;
    }
    switch (p.left->kind()) {
        case boost::json::kind::bool_: return p.left->get_bool() == p.right->get_bool();
        case boost::json::kind::double_:
            return same_non_finite(p.left->get_double(), p.right->get_double());
        case boost::json::kind::string: return p.left->get_string() == p.right->get_string();
        case boost::json::kind::array:
            return equal_arrays(p.left->get_array(), p.right->get_array(), work);
        case boost::json::kind::object:
            return equal_objects(p.left->get_object(), p.right->get_object(), work);
        default: return true;  // null; every int64 and uint64 was decomposed above
    }
}

/**
 Whether `a` and `b` are equal as JSON values, walked iteratively: one pair of values on a stack
 per element still to compare, never a call per level, so that no depth reaches the call stack.

 The stack starts inline, so that a value of at most 32 pending pairs allocates nothing; a wider
 or deeper one spills into `scratch`, and gives it back before returning.

 An allocation `scratch` cannot make raises what `scratch` raises, `std::bad_alloc` for a
 `std::pmr` resource, as in `deep_copy`: it is not a verdict, and json_schema reports no
 allocation failure as a value; built without exceptions, it ends the program.
*/
inline bool equal(const boost::json::value& a, const boost::json::value& b,
                  std::pmr::memory_resource& scratch) {
    pair_stack work{std::pmr::polymorphic_allocator<value_pair>(&scratch)};
    work.push_back({.left = &a, .right = &b});
    while (!work.empty()) {
        const value_pair p = work.back();
        work.pop_back();
        if (!equal_step(p, work)) {
            return false;
        }
    }
    return true;
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_EQUAL_HPP
