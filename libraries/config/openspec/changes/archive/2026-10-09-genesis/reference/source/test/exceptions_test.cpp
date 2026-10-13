// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/config.hpp> defines WEBCPP_HAS_EXCEPTIONS exactly when webcpp has exceptions.
// test/Jamfile builds this file three times: as the build asks; with exception-handling=off,
// which WEBCPP_TEST_CONFIG_OFF tells this file; and with WEBCPP_NO_EXCEPTIONS defined as a
// developer defines it, which WEBCPP_TEST_CONFIG_DEVELOPER_OFF tells it.

#include <webcpp/config.hpp>

#ifndef BOOST_CONFIG_HPP
#error "<webcpp/config.hpp> does not include <boost/config.hpp>"
#endif

#if defined(WEBCPP_TEST_CONFIG_OFF) && !defined(BOOST_NO_EXCEPTIONS)
#error "exceptions_off is built with exceptions: exception-handling=off was not applied"
#endif

#ifdef WEBCPP_TEST_CONFIG_DEVELOPER_OFF
#ifdef WEBCPP_HAS_EXCEPTIONS
#error "WEBCPP_HAS_EXCEPTIONS is defined though the developer defined WEBCPP_NO_EXCEPTIONS"
#endif
// WEBCPP_NO_EXCEPTIONS is webcpp's alone: Boost keeps the exceptions the compiler has.
#if (defined(__cpp_exceptions) || defined(_CPPUNWIND)) && defined(BOOST_NO_EXCEPTIONS)
#error "WEBCPP_NO_EXCEPTIONS turned Boost's exceptions off too"
#endif
#elif defined(WEBCPP_NO_EXCEPTIONS)
#error "config.hpp defines WEBCPP_NO_EXCEPTIONS, which is the developer's alone"
#elif defined(BOOST_NO_EXCEPTIONS) == defined(WEBCPP_HAS_EXCEPTIONS)
#error "WEBCPP_HAS_EXCEPTIONS is not defined exactly when the build has exceptions"
#endif
