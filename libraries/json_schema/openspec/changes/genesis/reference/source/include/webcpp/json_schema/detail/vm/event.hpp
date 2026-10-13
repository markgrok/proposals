// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Where an evaluation stands, as the machine tells a sink: the evaluation path
 and the instance path as tokens, and the schema node's resource and pointer.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_VM_EVENT_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_VM_EVENT_HPP

#include <webcpp/json_schema/config.hpp>

#include <cstdint>
#include <span>
#include <string_view>

namespace webcpp::json_schema::detail {

/** One step of a path: a keyword or a property name, or an array index. */
struct path_token {
    /** The keyword or the property name, unescaped; empty with `is_index`. */
    std::string_view name{};
    /** The array index, with `is_index`. */
    std::uint64_t index = 0;
    /** Whether the step is an array index rather than a name. */
    bool is_index = false;
};

/**
 Where an evaluation stands; a sink renders pointers from it only when it records.

 Tip: every view is valid only during the call that receives it.
*/
struct event_context {
    /** The keywords from the root, `$ref` and `$dynamicRef` segments kept. */
    std::span<const path_token> evaluation_path{};
    /** The steps from the instance's root. */
    std::span<const path_token> instance_path{};
    /** The absolute URI of the node's resource, without fragment. */
    std::string_view resource_uri{};
    /** The node's JSON Pointer in its resource, escaped, not yet percent-encoded. */
    std::string_view schema_pointer{};
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_VM_EVENT_HPP
