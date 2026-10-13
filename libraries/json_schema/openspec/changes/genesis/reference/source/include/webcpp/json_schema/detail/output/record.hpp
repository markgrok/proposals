// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 What a validation recorded: the schema nodes it entered, the errors and the
 annotations, which of them count, all in the validation's resource. The
 collecting sink writes it; the outcome and the output formats read it.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_OUTPUT_RECORD_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_OUTPUT_RECORD_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/value_arena.hpp>
#include <webcpp/json_schema/output/error.hpp>

#include <boost/assert.hpp>
#include <boost/json/storage_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>
#include <vector>

namespace webcpp::json_schema::detail {

/** The parent of a root unit. */
inline constexpr std::uint32_t no_unit = 0xFFFF'FFFFu;

/**
 One entered node: its locations, its parent, its result, and whether it counts.

 Tip: its locations are written when it closes; they are empty while it is open.
*/
struct unit {
    /** The evaluation path to the node, a JSON Pointer. */
    std::string_view keyword_location{};
    /** The node's resource URI, `#` and its pointer, percent-encoded as a URI fragment. */
    std::string_view absolute_keyword_location{};
    /** The instance location, a JSON Pointer. */
    std::string_view instance_location{};
    /** The unit that was open when this one was entered, or `no_unit` for a root. */
    std::uint32_t parent = no_unit;
    /** Whether the node passed; true while it is open. */
    bool valid = true;
    /** False once discarded, which only `collect::everything` keeps. */
    bool counts = true;
};

/**
 What a validation recorded, in the validation's resource; neither copyable nor movable.

 Three parallel sequences, in evaluation order: the units, the errors with the
 unit each was recorded in, and the annotations likewise. Each error and
 annotation has a count flag, 1 while it counts. Under `collect::everything` a
 discard flags what it drops, units included, and keeps it for the verbose
 output; under any other `collect` it removes it, so that what is recorded is
 what counts.

 The record keeps what the collecting sink gives it. That sink gives a unit's
 locations only when the unit closes, and, under any `collect` but
 `everything`, removes a unit that ends valid with nothing recorded in it, so
 that a valid instance costs its open chain, not every node it passed.

 Tip: every string and every vector lives in a monotonic arena over the given
 resource, and every annotation value in a trivially freed arena over it, so
 the record frees all of it at once and no value's destruction recurses.
*/
class record {
public:
    /** Where the three sequences end: a mark of the collecting sink. */
    struct position {
        /** The count of units. */
        std::uint32_t units = 0;
        /** The count of errors recorded. */
        std::uint32_t errors = 0;
        /** The count of annotations recorded. */
        std::uint32_t annotations = 0;
    };

    /** An empty record that allocates from `resource` and collects `what`. */
    record(std::pmr::memory_resource& resource, collect what)
        : what_(what),
          arena_(&resource),
          values_(resource),
          units_(&arena_),
          errors_(&arena_),
          error_units_(&arena_),
          error_counts_(&arena_),
          annotations_(&arena_),
          annotation_units_(&arena_),
          annotation_counts_(&arena_),
          counting_errors_(&arena_),
          counting_error_units_(&arena_),
          counting_annotations_(&arena_),
          counting_annotation_units_(&arena_) {}

    /** Not copyable: views into the arena would be shared. */
    record(const record&) = delete;

    /** Not copyable: views into the arena would be shared. */
    record& operator=(const record&) = delete;

    /** What the record collects. */
    [[nodiscard]] collect collected() const noexcept { return what_; }

    /** The errors that count, in evaluation order, complete once `finish()` has run. */
    [[nodiscard]] std::span<const error> errors() const noexcept {
        return what_ == collect::everything ? std::span<const error>(counting_errors_) : errors_;
    }

    /** The unit of each error `errors()` holds. */
    [[nodiscard]] std::span<const std::uint32_t> error_units() const noexcept {
        return what_ == collect::everything ? std::span<const std::uint32_t>(counting_error_units_)
                                            : error_units_;
    }

    /** The annotations that count, in evaluation order, complete once `finish()` has run. */
    [[nodiscard]] std::span<const annotation> annotations() const noexcept {
        return what_ == collect::everything ? std::span<const annotation>(counting_annotations_)
                                            : annotations_;
    }

    /** The unit of each annotation `annotations()` holds. */
    [[nodiscard]] std::span<const std::uint32_t> annotation_units() const noexcept {
        return what_ == collect::everything
                   ? std::span<const std::uint32_t>(counting_annotation_units_)
                   : annotation_units_;
    }

    /** Every unit entered; under `collect::everything` the discarded ones stay, flagged. */
    [[nodiscard]] std::span<const unit> units() const noexcept { return units_; }

    /** Every error recorded; under `collect::everything` the discarded ones stay. */
    [[nodiscard]] std::span<const error> recorded_errors() const noexcept { return errors_; }

    /** The unit of each recorded error. */
    [[nodiscard]] std::span<const std::uint32_t> recorded_error_units() const noexcept {
        return error_units_;
    }

    /** For each recorded error, 1 while it counts and 0 once discarded. */
    [[nodiscard]] std::span<const std::uint8_t> recorded_error_counts() const noexcept {
        return error_counts_;
    }

    /** Every annotation recorded; under `collect::everything` the discarded ones stay. */
    [[nodiscard]] std::span<const annotation> recorded_annotations() const noexcept {
        return annotations_;
    }

    /** The unit of each recorded annotation. */
    [[nodiscard]] std::span<const std::uint32_t> recorded_annotation_units() const noexcept {
        return annotation_units_;
    }

    /** For each recorded annotation, 1 while it counts and 0 once dropped. */
    [[nodiscard]] std::span<const std::uint8_t> recorded_annotation_counts() const noexcept {
        return annotation_counts_;
    }

    /** The arena, monotonic over the given resource, which holds everything recorded. */
    std::pmr::memory_resource& arena() noexcept { return arena_; }

    /**
     A storage for the values the engine computes, such as annotations, which
     deallocates trivially: a value in it dies without recursion.
    */
    boost::json::storage_ptr storage() noexcept { return {&values_}; }

    /** Where the three sequences end now. */
    [[nodiscard]] position here() const noexcept {
        return {
            .units = index_of(units_.size()),
            .errors = index_of(errors_.size()),
            .annotations = index_of(annotations_.size()),
        };
    }

    /** Opens a unit inside the current one, its locations empty until it closes; its index. */
    std::uint32_t open_unit() {
        const std::uint32_t index = index_of(units_.size());
        units_.push_back({
            .keyword_location = {},
            .absolute_keyword_location = {},
            .instance_location = {},
            .parent = current_,
            .valid = true,
            .counts = true,
        });
        current_ = index;
        return index;
    }

    /**
     Closes the current unit with its result and its locations, copied into the
     arena; its parent becomes current.

     Tip: a unit about to be discarded is closed with empty locations, which
     copy nothing.
    */
    void close_unit(bool valid, std::string_view keyword_location, std::string_view absolute,
                    std::string_view instance_location) {
        BOOST_ASSERT(current_ != no_unit);
        unit& u = units_[current_];
        u.keyword_location = copy_text(keyword_location, arena_);
        u.absolute_keyword_location = copy_text(absolute, arena_);
        u.instance_location = copy_text(instance_location, arena_);
        u.valid = valid;
        current_ = u.parent;
    }

    /** The open unit that errors and annotations are recorded in, or `no_unit`. */
    [[nodiscard]] std::uint32_t current_unit() const noexcept { return current_; }

    /** Records `e` in the current unit, its strings and details copied into the arena. */
    void add_error(const error& e) {
        errors_.push_back(error_access::copy_into(e, arena_));
        error_units_.push_back(current_);
        error_counts_.push_back(1);
    }

    /**
     Records `a` in the current unit, its strings copied into the arena and its
     value into `storage()`; nothing under `collect::errors`.

     Tip: the copy's work stacks come from the given resource, which takes them
     back before the copy returns; in the arena they would stay.
    */
    void add_annotation(const annotation& a) {
        if (what_ == collect::errors) {
            return;
        }
        annotations_.push_back(
            error_access::copy_into(a, arena_, storage(), *arena_.upstream_resource()));
        annotation_units_.push_back(current_);
        annotation_counts_.push_back(1);
    }

    /** What was recorded since `p` no longer counts: removed, or flagged under `everything`. */
    void discard_since(position p) {
        BOOST_ASSERT(current_ == no_unit || current_ < p.units);
        if (what_ != collect::everything) {
            truncate(units_, p.units);
            truncate(errors_, p.errors);
            truncate(error_units_, p.errors);
            truncate(error_counts_, p.errors);
            truncate(annotations_, p.annotations);
            truncate(annotation_units_, p.annotations);
            truncate(annotation_counts_, p.annotations);
            return;
        }
        for (std::size_t i = p.units; i < units_.size(); ++i) {
            units_[i].counts = false;
        }
        zero(error_counts_, p.errors);
        zero(annotation_counts_, p.annotations);
    }

    /** The annotations recorded since index `annotations` no longer count (P2), the same way. */
    void discard_annotations_since(std::uint32_t annotations) {
        if (what_ != collect::everything) {
            truncate(annotations_, annotations);
            truncate(annotation_units_, annotations);
            truncate(annotation_counts_, annotations);
            return;
        }
        zero(annotation_counts_, annotations);
    }

    /**
     Gathers what counts into `errors()`, `error_units()`, `annotations()` and
     `annotation_units()`; again after more is recorded.

     Tip: only `collect::everything` keeps what does not count, so only it
     gathers; under any other `collect` those four view what was recorded.
    */
    void finish() {
        if (what_ != collect::everything) {
            return;
        }
        counting_errors_.clear();
        counting_error_units_.clear();
        for (std::size_t i = 0; i < errors_.size(); ++i) {
            if (error_counts_[i] != 0) {
                counting_errors_.push_back(errors_[i]);
                counting_error_units_.push_back(error_units_[i]);
            }
        }
        counting_annotations_.clear();
        counting_annotation_units_.clear();
        for (std::size_t i = 0; i < annotations_.size(); ++i) {
            if (annotation_counts_[i] != 0) {
                counting_annotations_.push_back(annotations_[i]);
                counting_annotation_units_.push_back(annotation_units_[i]);
            }
        }
    }

private:
    /** A size of a sequence, as the 32-bit index the record keeps. */
    static std::uint32_t index_of(std::size_t size) noexcept {
        BOOST_ASSERT(size < no_unit);
        return static_cast<std::uint32_t>(size);
    }

    /** Removes the elements of `v` from index `size` on, if it has that many. */
    template <class T>
    static void truncate(std::pmr::vector<T>& v, std::size_t size) {
        v.erase(v.begin() + static_cast<std::ptrdiff_t>(std::min(size, v.size())), v.end());
    }

    /** Sets the flags of `counts` from index `from` on to 0. */
    static void zero(std::pmr::vector<std::uint8_t>& counts, std::size_t from) noexcept {
        std::fill(counts.begin() + static_cast<std::ptrdiff_t>(std::min(from, counts.size())),
                  counts.end(), std::uint8_t{0});
    }

    collect what_;
    std::pmr::monotonic_buffer_resource arena_;
    arena_resource values_;
    std::pmr::vector<unit> units_;
    std::pmr::vector<error> errors_;
    std::pmr::vector<std::uint32_t> error_units_;
    std::pmr::vector<std::uint8_t> error_counts_;
    std::pmr::vector<annotation> annotations_;
    std::pmr::vector<std::uint32_t> annotation_units_;
    std::pmr::vector<std::uint8_t> annotation_counts_;
    std::pmr::vector<error> counting_errors_;
    std::pmr::vector<std::uint32_t> counting_error_units_;
    std::pmr::vector<annotation> counting_annotations_;
    std::pmr::vector<std::uint32_t> counting_annotation_units_;
    std::uint32_t current_ = no_unit;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_OUTPUT_RECORD_HPP
