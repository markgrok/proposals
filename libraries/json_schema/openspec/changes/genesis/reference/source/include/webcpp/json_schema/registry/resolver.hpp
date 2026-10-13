// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The resolver: the user's synchronous lookup of a URI that nothing else a compile reads knows.

 @see "References", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_REGISTRY_RESOLVER_HPP
#define WEBCPP_JSON_SCHEMA_REGISTRY_RESOLVER_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/compat/function_ref.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/system/result.hpp>

#include <string_view>

namespace webcpp::json_schema {

/**
 The user's synchronous lookup of a URI; never the library's network or disk.

 A compile calls it once per absolute URI (its fragment removed) that neither the document, the
 registry nor the bundled meta-schemas know. It builds the document in `storage`, which the
 schema's allocator backs, and returns it, or an error, which the compile reports as
 `errc::resolver_failed` naming the URI. What it returns is copied into the schema, so it need not
 outlive the compile; the callable it refers to must outlive the options that hold it.

 @see "References", in the guide.
*/
using resolver_ref = boost::compat::function_ref<boost::system::result<boost::json::value>(
    std::string_view uri, const boost::json::storage_ptr& storage)>;

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_REGISTRY_RESOLVER_HPP
