// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 JSON text read exactly: the value as Boost.JSON stores it, and the exact `Number` of each number
 whose double denotes another, which the loader prefers.

 @see "Exact numbers", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_JSON_PARSE_HPP
#define WEBCPP_JSON_SCHEMA_JSON_PARSE_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/exact_handler.hpp>
#include <webcpp/json_schema/detail/json/value_arena.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <boost/json/basic_parser_impl.hpp>
#include <boost/json/error.hpp>
#include <boost/json/parse_options.hpp>
#include <boost/json/value.hpp>
#include <boost/system/error_code.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

namespace webcpp::json_schema {

namespace detail {

/**
 The deepest nesting of arrays and objects @ref parse reads.

 Boost.JSON's parser recurses once per level, so the bound is measured: the largest power of two
 at most half the shallowest nesting that ran out of stack, on any target, in a debug build
 (-O0) with the target's default stack, parsing through this handler arrays or objects nested
 one in another, each the same:

 | Target | Deepest read | First out of stack |
 | --- | --- | --- |
 | wasip2, wasip3 (64 KiB) | 560 | 561 |
 | emscripten (64 KiB) | 557 | 558 |
 | native, macOS (8 MiB) | at least 16,384 | at most 32,768 |

 Half of 557 is 278, and 256 the power of two at most that.
*/
inline constexpr std::size_t max_parse_depth = 256;

/**
 Boost.JSON's default options but `max_depth`, which is @ref max_parse_depth, not Boost.JSON's 32.
*/
inline boost::json::parse_options default_parse_options() noexcept {
    boost::json::parse_options options;
    options.max_depth = max_parse_depth;
    return options;
}

/** "offset <n>", the place of a refusal in the text. */
inline diagnostic parse_refusal(boost::system::error_code code, std::size_t offset) noexcept {
    std::array<char, 32> where{'o', 'f', 'f', 's', 'e', 't', ' '};
    constexpr std::size_t prefix = 7;
    const auto written = std::to_chars(where.data() + prefix, where.data() + where.size(), offset);
    return make_diagnostic(
        code, std::string_view(where.data(), static_cast<std::size_t>(written.ptr - where.data())),
        "");
}

}  // namespace detail

/**
 A JSON document read from text: its value, and the exact number of each number whose double
 denotes another.

 The value lives in storage of its own, which frees it all at once, so that a value nested
 however deep dies without recursion. A document moves and is not copied; an assignment frees the
 document it replaces, value then storage, and takes the other's.

 @tparam Traits The traits: its `number_type` holds the exact numbers, its `allocator_type`
 allocates everything.
 @see "Exact numbers", in the guide.
*/
template <traits Traits = default_traits>
struct basic_parsed_json final {
    /** The allocator of the document's storage and of its table. */
    using allocator_type = Traits::allocator_type;

    /**
     An empty document, a null value, in storage made through a default allocator.

     Tip: it is a constructor of its own, not a default argument, so that an allocator that
     cannot be made without arguments leaves the type whole; Boost.System's `result` asks whether
     it is default constructible.
    */
    basic_parsed_json()
        requires std::default_initializable<allocator_type>
        : basic_parsed_json(allocator_type{}) {}

    /**
     An empty document, a null value, in storage made through `allocator`.

     @param allocator The allocator of the storage and the table.
    */
    explicit basic_parsed_json(allocator_type allocator)
        : storage(std::allocate_shared<detail::value_arena<allocator_type>>(allocator, allocator)),
          value(storage->storage()),
          exact_numbers(allocator) {}

    /** Takes the storage, the value and the table of `other`, which holds none after. */
    basic_parsed_json(basic_parsed_json&& other) noexcept = default;

    /**
     Frees this document, value then storage, and takes the storage, the value and the table of
     `other`.

     Tip: the members cannot be assigned one by one. The storage would die before the value in
     it, and `boost::json::value`'s assignment copies a value into its own storage when the two
     storages differ, which here they always do.

     @param other The document taken.
     @return This document.
    */
    basic_parsed_json& operator=(basic_parsed_json&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    /** Not copied: a copy would share the storage, which frees nothing until the last owner. */
    basic_parsed_json(const basic_parsed_json&) = delete;

    /** Not copied, as the copy constructor is not. */
    basic_parsed_json& operator=(const basic_parsed_json&) = delete;

    /** Frees the table, the value, and then the storage. */
    ~basic_parsed_json() = default;

    /** The storage of the value, declared first so that it dies last. */
    std::shared_ptr<detail::value_arena<allocator_type>> storage;

    /** The value, as Boost.JSON stores it, with `number_precision::precise`. */
    boost::json::value value;

    /**
     The exact number of each number whose double denotes another, by JSON Pointer; empty for
     most documents.
    */
    detail::exact_table<Traits> exact_numbers;
};

/** A document read with the default traits. */
using parsed_json = basic_parsed_json<>;

/**
 Reads JSON text, with the exact `Number` of each number whose double denotes another.

 Each number's lexeme is read into `Number`. Where the shortest round-trip text of its double
 denotes the same `Number` (most numbers), nothing is recorded; otherwise the exact `Number` is
 recorded under the number's JSON Pointer. Duplicate keys keep the last, in the value and in the
 table alike.

 @tparam Traits The traits.
 @param text The JSON text.
 @param allocator The allocator of everything the document holds and the reading needs.
 @param options Boost.JSON's options, with `numbers` set to `number_precision::precise`, and
 `max_depth` lowered to 256 when it is larger; by default Boost.JSON's, but `max_depth`, which
 is 256, not 32.
 @return The document, or a diagnostic of Boost.JSON's error, `boost::json::error::too_deep` for
 text nested deeper than the depth allowed and `boost::json::error::extra_data` for text after
 the value, its `where` "offset <n>".
 @throws std::bad_alloc When the allocator throws it; without exceptions, the program ends.
 @see "Exact numbers", in the guide.
*/
template <traits Traits = default_traits>
result<basic_parsed_json<Traits>> parse(
    std::string_view text, typename Traits::allocator_type allocator = {},
    boost::json::parse_options options = detail::default_parse_options()) {
    options.numbers = boost::json::number_precision::precise;
    options.max_depth = std::min(options.max_depth, detail::max_parse_depth);
    basic_parsed_json<Traits> parsed(allocator);
    boost::json::basic_parser<detail::exact_handler<Traits>> parser(
        options, parsed.storage->storage(), parsed.storage->resource(), parsed.exact_numbers);
    boost::system::error_code ec;
    const std::size_t consumed = parser.write_some(false, text.data(), text.size(), ec);
    if (ec) {
        return detail::parse_refusal(ec, consumed);
    }
    if (consumed < text.size()) {
        return detail::parse_refusal(boost::json::error::extra_data, consumed);
    }
    parsed.value = parser.handler().release();
    return parsed;
}

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_JSON_PARSE_HPP
