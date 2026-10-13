// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 User vocabularies: keywords a program adds, each an assertion or an annotation that works as an
 official keyword does.

 @see "User vocabularies", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_VOCABULARY_VOCABULARY_HPP
#define WEBCPP_JSON_SCHEMA_VOCABULARY_VOCABULARY_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <boost/compat/function_ref.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/system/result.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace webcpp::json_schema {

/**
 What a user keyword's `validate` receives: the keyword's value and the instance it applies to.

 @see "User vocabularies", in the guide.
*/
struct user_call {
    /** The keyword's value in the schema. */
    const boost::json::value& value;

    /** The value at the instance location the keyword applies to. */
    const boost::json::value& instance;
};

/**
 A keyword of a user vocabulary: its name and the functions that check its value and decide an
 instance.

 It loads as `ir::keyword_kind::user` and fails with errors of kind `error_kind::user`. Its value is
 data, never a schema: the keyword is an assertion or an annotation, not an applicator. Its
 functions must outlive every schema compiled with it.

 @tparam Traits The traits of the validator that reads it.
 @see "User vocabularies", in the guide.
*/
template <traits Traits>
struct basic_user_keyword {
    /** The keyword as a schema writes it; one an active official or user keyword takes conflicts.
     */
    std::string_view name{};

    /** Checks the keyword's value at load; an error refuses the load, naming the keyword. */
    boost::compat::function_ref<boost::system::result<void>(const boost::json::value&)> check;

    /**
     Decides an instance: true when it satisfies the keyword.

     An error is an operational failure: `validate` returns it as its result's error, with the
     keyword's location, and `is_valid` answers `false`.
    */
    boost::compat::function_ref<boost::system::result<bool>(const user_call&)> validate;

    /** The instance types the keyword applies to; values of other types pass untouched. */
    ir::type_set applies_to = ir::type_set::all();

    /** Whether a passing keyword annotates its value. */
    bool annotates = false;

    /**
     Writes the keyword for a target dialect, in the storage given; without it the emitter copies
     the value.
    */
    std::optional<boost::compat::function_ref<boost::system::result<boost::json::value>(
        const boost::json::value&, dialect, const boost::json::storage_ptr&)>>
        write{};
};

/**
 Where a user vocabulary's keywords are recognized.

 @see "User vocabularies", in the guide.
*/
enum class activation : std::uint8_t {
    /** In every resource. */
    always,

    /** Only in a resource whose meta-schema's `$vocabulary` lists the vocabulary's URI. */
    when_declared,
};

/**
 A user vocabulary: its URI, its keywords, and where they are recognized.

 A required vocabulary of a meta-schema's `$vocabulary` is known when its URI is a user
 vocabulary's, as when it is an official one.

 @tparam Traits The traits of the validator that reads it.
 @see "User vocabularies", in the guide.
*/
template <traits Traits>
struct basic_vocabulary {
    /** The vocabulary's URI, as a meta-schema's `$vocabulary` names it. */
    std::string_view uri{};

    /** The vocabulary's keywords, which must outlive every schema compiled with them. */
    std::span<const basic_user_keyword<Traits>> keywords{};

    /** Where the keywords are recognized. */
    activation active = activation::always;
};

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_VOCABULARY_VOCABULARY_HPP
