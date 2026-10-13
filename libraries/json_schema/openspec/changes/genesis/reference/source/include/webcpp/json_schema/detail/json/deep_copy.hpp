// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The one way json_schema copies a JSON value: iteratively, never by Boost.JSON's copy constructor,
 which recurses on the value's depth.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_DEEP_COPY_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_DEEP_COPY_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/json_resource.hpp>
#include <webcpp/json_schema/detail/memory/inline_stack.hpp>

#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_stack.hpp>

#include <cstddef>
#include <memory_resource>
#include <utility>

namespace webcpp::json_schema::detail {

/** Pushes a copy of `v`, which is neither an array nor an object, onto `built`. */
inline void push_scalar(boost::json::value_stack& built, const boost::json::value& v) {
    switch (v.kind()) {
        case boost::json::kind::null: built.push_null(); break;
        case boost::json::kind::bool_: built.push_bool(v.get_bool()); break;
        case boost::json::kind::int64: built.push_int64(v.get_int64()); break;
        case boost::json::kind::uint64: built.push_uint64(v.get_uint64()); break;
        case boost::json::kind::double_: built.push_double(v.get_double()); break;
        case boost::json::kind::string: built.push_string(v.get_string()); break;
        default: break;
    }
}

/**
 A copy of `v` in `storage`, made in post-order through Boost.JSON's own builder: each container
 is pushed once its elements are, so that the walk keeps one frame per level on a stack of its own,
 never on the call stack.

 Every value of the copy is allocated in `storage`; the walk's stack and the builder's own stack
 are allocated in `scratch`, and given back before the copy returns. A copy keeps the kinds of
 its numbers (an int64, a uint64, a double), its members' order and its strings' bytes.

 Tip: the builder's stack is kept out of `storage` because `storage` is most often an arena, which
 would keep every stack the builder outgrew for as long as the copy lives.
*/
inline boost::json::value deep_copy(const boost::json::value& v, boost::json::storage_ptr storage,
                                    std::pmr::memory_resource& scratch) {
    struct frame {
        const boost::json::value* value;
        std::size_t next;
    };

    json_resource temporaries(scratch);
    boost::json::value_stack built{boost::json::storage_ptr(&temporaries)};
    built.reset(std::move(storage));
    inline_stack<frame, 32> work{std::pmr::polymorphic_allocator<frame>(&scratch)};
    work.push_back({.value = &v, .next = 0});
    while (!work.empty()) {
        frame& f = work.back();
        if (const auto* array = f.value->if_array()) {
            if (f.next < array->size()) {
                work.push_back({.value = &(*array)[f.next++], .next = 0});
                continue;
            }
            built.push_array(array->size());
        } else if (const auto* object = f.value->if_object()) {
            if (f.next < object->size()) {
                const auto& member = *(object->begin() + static_cast<std::ptrdiff_t>(f.next++));
                built.push_key(member.key());
                work.push_back({.value = &member.value(), .next = 0});
                continue;
            }
            built.push_object(object->size());
        } else {
            push_scalar(built, *f.value);
        }
        work.pop_back();
    }
    return built.release();
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_DEEP_COPY_HPP
