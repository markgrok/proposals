// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Whether a tag is a dialect policy does not depend on what was included before the question: it
// is asked here from the dialects' header alone, and the validator, included after, still compiles
// and loads.

#include <webcpp/json_schema/dialects/dialect.hpp>

static_assert(webcpp::json_schema::dialect_policy<webcpp::json_schema::draft2020_12>);
static_assert(!webcpp::json_schema::dialect_policy<webcpp::json_schema::draft7>);

// Included after the question on purpose: the answer above must not change below.
#include <webcpp/json_schema/validator/validator.hpp>  // NOLINT(llvm-include-order)

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

static_assert(js::dialect_policy<js::draft2020_12>);

int main() {
    const js::validator validator;
    BOOST_TEST(validator.load(boost::json::parse(R"({"type": "string"})")).has_value());
    return boost::report_errors();
}
