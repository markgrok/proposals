// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Boost.JSON's memory resource over a `std::pmr` one: what json_schema's JSON values and Boost.JSON's
 own temporaries allocate through.

 Tip: Boost.JSON's `storage_ptr` holds a `boost::container::pmr::memory_resource`, whose base class
 is header-only, while `boost::container::pmr`'s global resources are compiled; the adapter needs
 only the base. It is trystero's `json_resource`, taking its upstream by reference.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_JSON_RESOURCE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_JSON_RESOURCE_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/container/pmr/memory_resource.hpp>

#include <cstddef>
#include <memory_resource>

namespace webcpp::json_schema::detail {

/**
 A `boost::container::pmr::memory_resource` that passes every request to a
 `std::pmr::memory_resource`, with its size and alignment.

 A `boost::json::storage_ptr` made from its address does not own it, and counts nothing: no
 resource is created with `operator new`.
*/
class json_resource final : public boost::container::pmr::memory_resource {
public:
    /** Allocates from `upstream`, which must outlive this resource and what it allocates. */
    explicit json_resource(std::pmr::memory_resource& upstream) noexcept : upstream_(&upstream) {}

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        return upstream_->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        upstream_->deallocate(p, bytes, alignment);
    }

    /**
     Whether `other` is this resource: it is equal to itself alone.

     Tip: another adapter over the same upstream would do too, but telling one apart from another
     resource needs RTTI, which a user may build without; an unequal resource costs Boost.JSON a
     copy where it could have moved.
    */
    [[nodiscard]] bool do_is_equal(
        const boost::container::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

private:
    std::pmr::memory_resource* upstream_;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_JSON_RESOURCE_HPP
