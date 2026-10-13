// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/errors.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/system/system_error.hpp>

#include <string>

namespace js = webcpp::json_schema;

int main() {
    const js::result<int> r = js::detail::make_diagnostic(js::errc::invalid_regex, "/pattern", "(");
    try {
        (void)r.value();
        BOOST_ERROR("value() of an error returned");
    } catch (const boost::system::system_error& e) {
        BOOST_TEST(e.code() == js::errc::invalid_regex);
        BOOST_TEST(std::string(e.what()).find("/pattern: (") != std::string::npos);
    }
    return boost::report_errors();
}
