// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The registry: the user's documents, by their retrieval URIs, which references reach, and the
 meta-schemas compiled for them.

 @see "References", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_REGISTRY_REGISTRY_HPP
#define WEBCPP_JSON_SCHEMA_REGISTRY_REGISTRY_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/traits.hpp>

#include <concepts>
#include <memory>
#include <utility>

namespace webcpp::json_schema {

/**
 The user's documents, by their retrieval URIs, which a compile's references reach after the
 document's own resources and before the bundled meta-schemas.

 A compile given a registry also keeps in it the meta-schemas it compiles, for later compiles. It
 is the user's object, so this is no global state; two threads never write one registry at once.

 @tparam Traits The traits, whose allocator holds the registry's documents.
 @see "References", in the guide.
*/
template <traits Traits>
class basic_registry final {
public:
    /** The allocator of everything the registry holds. */
    using allocator_type = Traits::allocator_type;

    /**
     An empty registry, made through a default allocator.

     Tip: it is a constructor of its own, not a default argument, so that an allocator that cannot
     be made without arguments leaves the type whole.
    */
    basic_registry()
        requires std::default_initializable<allocator_type>
        : basic_registry(allocator_type{}) {}

    /**
     An empty registry, which allocates through `allocator`.

     @param allocator The allocator of everything the registry holds.
    */
    explicit basic_registry(allocator_type allocator) noexcept : allocator_(allocator) {}

    /** Takes everything `other` holds, and its allocator; `other` holds nothing after. */
    basic_registry(basic_registry&&) noexcept = default;

    /**
     Frees this registry, then takes everything `other` holds, and its allocator.

     Tip: the members cannot be assigned one by one: a polymorphic allocator is not assignable, and
     a member whose allocator differs would copy its elements, which may throw.

     @param other The registry taken.
     @return This registry.
    */
    basic_registry& operator=(basic_registry&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    /** Not copied: a registry is moved, as a document is, so no copy keeps another's allocator. */
    basic_registry(const basic_registry&) = delete;

    /** Not copied, as the copy constructor is not. */
    basic_registry& operator=(const basic_registry&) = delete;

    /** Frees everything the registry holds. */
    ~basic_registry() = default;

    /**
     The allocator.

     @return A copy of the allocator everything the registry holds is allocated through.
    */
    [[nodiscard]] allocator_type get_allocator() const noexcept { return allocator_; }

private:
    allocator_type allocator_;
};

/** A registry of the default traits. */
using registry = basic_registry<default_traits>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_REGISTRY_REGISTRY_HPP
