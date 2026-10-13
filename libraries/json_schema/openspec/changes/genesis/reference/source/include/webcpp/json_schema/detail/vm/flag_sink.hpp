// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The sink of `is_valid`, which records nothing, so that the machine
 short-circuits.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_VM_FLAG_SINK_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_VM_FLAG_SINK_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/vm/event.hpp>
#include <webcpp/json_schema/detail/vm/sink.hpp>
#include <webcpp/json_schema/output/error.hpp>

namespace webcpp::json_schema::detail {

/**
 A sink that records nothing: with `records == false` the machine calls none of
 its members (P6), which exist for the concept.
*/
struct flag_sink {
    /** The machine short-circuits and skips every call below. */
    static constexpr bool records = false;

    /** A mark that marks nothing. */
    struct mark {};

    /** Does nothing. */
    void enter(const event_context& /*at*/) noexcept {}

    /** Does nothing. */
    void leave(const event_context& /*at*/, bool /*valid*/) noexcept {}

    /** Does nothing. */
    void fail(const error& /*e*/) noexcept {}

    /** Does nothing. */
    void annotate(const annotation& /*a*/) noexcept {}

    /**
     An empty mark.

     Tip: a member like every sink's, which the machine calls on its sink.
    */
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    mark checkpoint() noexcept { return {}; }

    /** Does nothing. */
    void discard(mark /*m*/) noexcept {}
};

static_assert(evaluation_sink<flag_sink>);

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_VM_FLAG_SINK_HPP
