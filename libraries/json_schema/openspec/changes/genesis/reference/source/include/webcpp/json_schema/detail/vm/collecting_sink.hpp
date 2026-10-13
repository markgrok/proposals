// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The sink of `validate`, which writes what the machine tells it into a record.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_VM_COLLECTING_SINK_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_VM_COLLECTING_SINK_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/pointer.hpp>
#include <webcpp/json_schema/detail/memory/inline_stack.hpp>
#include <webcpp/json_schema/detail/output/record.hpp>
#include <webcpp/json_schema/detail/vm/event.hpp>
#include <webcpp/json_schema/detail/vm/sink.hpp>
#include <webcpp/json_schema/output/error.hpp>

#include <boost/assert.hpp>

#include <cstdint>
#include <memory_resource>
#include <span>
#include <string>

namespace webcpp::json_schema::detail {

/**
 A sink that records, into a @ref record, a unit per node kept, each error, and
 each annotation the record collects, with P2 and P3 applied.

 A unit's locations are written when it closes, from the context `leave`
 receives, which is its `enter`'s (P1): the evaluation path and the instance
 path as JSON Pointers, and the absolute location as the resource URI, `#` and
 the node's pointer percent-encoded as a URI fragment. Under any `collect` but
 `everything`, a node that ends valid with nothing recorded in it but its own
 unit is removed whole, without its locations ever being written, so that a
 valid walk holds only its open chain.

 Tip: a valid node holds no counting error, since P4 discards a failed
 alternative of a passing applicator, so removing it loses nothing an output
 reads; `everything` keeps it for the verbose output.
*/
class collecting_sink {
public:
    /** The machine evaluates every branch in full and calls every member. */
    static constexpr bool records = true;

    /** Where the record ended when the mark was taken. */
    using mark = record::position;

    /** A sink that records into `into`, which outlives it. */
    explicit collecting_sink(record& into) noexcept
        : record_(&into),
          opened_(std::pmr::polymorphic_allocator<record::position>(&into.arena())),
          keyword_location_(&into.arena()),
          instance_location_(&into.arena()),
          absolute_location_(&into.arena()) {}

    /** Opens a unit, noting where the record ended before it. */
    void enter(const event_context& /*at*/) {
        opened_.push_back(record_->here());
        record_->open_unit();
    }

    /**
     Closes the unit `enter` opened: when it failed, its annotations no longer
     count (P2); when it holds nothing worth keeping, it is removed.
    */
    void leave(const event_context& at, bool valid) {
        BOOST_ASSERT(!opened_.empty());
        const record::position since = opened_.back();
        opened_.pop_back();
        if (!valid) {
            record_->discard_annotations_since(since.annotations);
        }
        const record::position now = record_->here();
        const bool holds_nothing = now.units == since.units + 1 && now.errors == since.errors &&
                                   now.annotations == since.annotations;
        if (valid && holds_nothing && record_->collected() != collect::everything) {
            record_->close_unit(valid, {}, {}, {});
            record_->discard_since(since);
            return;
        }
        render(keyword_location_, at.evaluation_path);
        render(instance_location_, at.instance_path);
        absolute_location_.assign(at.resource_uri);
        absolute_location_.push_back('#');
        append_fragment(absolute_location_, at.schema_pointer);
        record_->close_unit(valid, keyword_location_, absolute_location_, instance_location_);
    }

    /** Records `e`, copied, in the open unit. */
    void fail(const error& e) { record_->add_error(e); }

    /** Records `a`, copied, in the open unit, unless the record collects errors alone. */
    void annotate(const annotation& a) { record_->add_annotation(a); }

    /** Where the record ends now. */
    [[nodiscard]] mark checkpoint() const noexcept { return record_->here(); }

    /** What was recorded since `m` no longer counts (P3). */
    void discard(mark m) { record_->discard_since(m); }

private:
    /** `out`, made the JSON Pointer of `path`. */
    static void render(std::pmr::string& out, std::span<const path_token> path) {
        out.clear();
        for (const path_token& token : path) {
            if (token.is_index) {
                append_index(out, token.index);
            } else {
                append_token(out, token.name);
            }
        }
    }

    record* record_;
    inline_stack<record::position, 32> opened_;
    std::pmr::string keyword_location_;
    std::pmr::string instance_location_;
    std::pmr::string absolute_location_;
};

static_assert(evaluation_sink<collecting_sink>);

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_VM_COLLECTING_SINK_HPP
