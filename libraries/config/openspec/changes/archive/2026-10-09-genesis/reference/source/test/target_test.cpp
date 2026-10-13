// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/config.hpp> defines exactly one WEBCPP_TARGET_* macro, the one of the target the
// build is for, which test/Jamfile tells this file by the toolset: WEBCPP_TEST_TARGET_WASIP2,
// WEBCPP_TEST_TARGET_WASIP3, WEBCPP_TEST_TARGET_EMSCRIPTEN, or none natively.

#include <webcpp/config.hpp>

#if (defined(WEBCPP_TARGET_NATIVE) + defined(WEBCPP_TARGET_WASIP2) + \
     defined(WEBCPP_TARGET_WASIP3) + defined(WEBCPP_TARGET_EMSCRIPTEN)) != 1
#error "config.hpp defines other than exactly one WEBCPP_TARGET_* macro"
#endif

#ifdef WEBCPP_TEST_TARGET_WASIP2
#ifndef WEBCPP_TARGET_WASIP2
#error "a build for wasm32-wasip2 does not define WEBCPP_TARGET_WASIP2"
#endif
#elif defined(WEBCPP_TEST_TARGET_WASIP3)
#ifndef WEBCPP_TARGET_WASIP3
#error "a build for wasm32-wasip3 does not define WEBCPP_TARGET_WASIP3"
#endif
#elif defined(WEBCPP_TEST_TARGET_EMSCRIPTEN)
#ifndef WEBCPP_TARGET_EMSCRIPTEN
#error "a build for emscripten does not define WEBCPP_TARGET_EMSCRIPTEN"
#endif
#elif !defined(WEBCPP_TARGET_NATIVE)
#error "a native build does not define WEBCPP_TARGET_NATIVE"
#endif
