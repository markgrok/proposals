// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 What the machine tells the sink it runs with, and the protocol it keeps
 while it does: the contract between the machine and the output.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_VM_SINK_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_VM_SINK_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/vm/event.hpp>
#include <webcpp/json_schema/output/error.hpp>

#include <concepts>

namespace webcpp::json_schema::detail {

/**
 A sink of evaluation events, which the machine calls by this protocol.

 - **P1.** Every evaluation of a schema node at an instance location is
   bracketed by `enter` and `leave`, properly nested. `leave` receives the
   context its `enter` received.
 - **P2.** On `leave(at, false)`, every annotation recorded since the matching
   `enter` no longer counts (2020-12 core 7.7.1.2). The sink applies this
   itself.
 - **P3.** `discard(m)`: everything recorded since `m`, errors and
   annotations, no longer counts. The machine calls it in LIFO order, never
   with a mark older than one it already discarded to. A sink that collects
   everything marks what follows `m`, and never removes it.
 - **P4.** The machine calls `discard` after a subschema whose records do not
   count: the condition of `if` when it failed; an item that does not match
   `contains`; the operand of `not`, whatever its result; an alternative of
   `anyOf` or `oneOf` that failed, unless that applicator is one whose failed
   alternatives count. An applicator's failed alternatives count when it
   fails with no passing alternative. The machine knows which applicators
   those are without nested re-evaluation: `validate` runs the plan a first
   time, discarding every failed alternative and noting, for each `anyOf`
   and `oneOf` evaluation in order, whether it so failed; only when the
   instance is invalid and one did, it runs the plan once more from the top,
   keeping those applicators' failed alternatives. Evaluation is
   deterministic, so the two runs meet the applicators in the same order;
   `validate` runs the plan at most twice.
 - **P5.** `fail` and `annotate` receive views valid only during the call. A
   sink that keeps them copies them.
 - **P6.** With `records == false`, the machine calls none of the six and
   short-circuits: a block stops at its first failing assertion; `allOf`
   stops at its first failing subschema; `anyOf` stops at its first passing
   one, unless the compiler marked it exhaustive; `oneOf` stops at its second
   passing one.
*/
template <class S>
concept evaluation_sink = requires(S& sink, const event_context& at, const error& e,
                                   const annotation& a, S::mark m, bool valid) {
    { S::records } -> std::convertible_to<bool>;
    sink.enter(at);
    sink.leave(at, valid);
    sink.fail(e);
    sink.annotate(a);
    { sink.checkpoint() } -> std::same_as<typename S::mark>;
    sink.discard(m);
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_VM_SINK_HPP
