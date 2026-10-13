// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 webcpp's version: the one place it is written, which the superproject's build and its release
 tooling read.
*/
#ifndef WEBCPP_VERSION_HPP
#define WEBCPP_VERSION_HPP

// WEBCPP_VERSION is a macro, as BOOST_VERSION is, so that #if compares it: hence the exemption
// from modernize-macro-to-enum, on the line above it, since the build and the release tooling
// read the #define line itself.

/**
 webcpp's version as a number, `major * 100000 + minor * 100 + patch`, as `BOOST_VERSION` is
 Boost's.

 0.1.0 is 100 and 1.2.3 is 100203: `WEBCPP_VERSION % 100` is the patch level,
 `WEBCPP_VERSION / 100 % 1000` the minor version and `WEBCPP_VERSION / 100000` the major one.

 @see "Version", in the guide.
*/
// NOLINTNEXTLINE(modernize-macro-to-enum)
#define WEBCPP_VERSION 100

/**
 webcpp's version as text, `"<major>_<minor>"`, with `"_<patch>"` after it when the patch level
 is not 0, as `BOOST_LIB_VERSION` is Boost's.

 `"0_1"` is 0.1.0 and `"1_2_3"` is 1.2.3.

 @see "Version", in the guide.
*/
#define WEBCPP_LIB_VERSION "0_1"

#endif  // WEBCPP_VERSION_HPP
