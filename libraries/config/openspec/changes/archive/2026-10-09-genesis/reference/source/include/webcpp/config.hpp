// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 webcpp's configuration, which every webcpp library's own configuration includes first:
 Boost.Config, webcpp's version, the target a program is built for, and whether webcpp has
 exceptions.
*/
#ifndef WEBCPP_CONFIG_HPP
#define WEBCPP_CONFIG_HPP

// MrDocs parses webcpp natively and with exceptions, where only WEBCPP_TARGET_NATIVE and
// WEBCPP_HAS_EXCEPTIONS are defined: these definitions, each undone at once, are the ones the
// reference lists for the other macros.
#ifdef __MRDOCS__
/**
 Defined when the program is built for wasm32-wasip2, as a component of WASI 0.2.

 The compiler says so with `\__wasip2__`, which wasi-sdk's clang defines for the target
 `wasm32-wasip2`.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_WASIP2
#undef WEBCPP_TARGET_WASIP2
/**
 Defined when the program is built for wasm32-wasip3, as a component of WASI 0.3.

 The compiler says so with `\__wasip3__`, which wasi-sdk's clang defines for the target
 `wasm32-wasip3`.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_WASIP3
#undef WEBCPP_TARGET_WASIP3
/**
 Defined when the program is built with Emscripten, for a browser or for node.

 The compiler says so with `\__EMSCRIPTEN__`.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_EMSCRIPTEN
#undef WEBCPP_TARGET_EMSCRIPTEN
/**
 Defined by the developer to disable the exceptions of every webcpp library at once, even in a
 build that has them.

 With it, `WEBCPP_HAS_EXCEPTIONS` is not defined, and so each library's
 `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` is. Boost's own exceptions are left as the compiler has them:
 `BOOST_NO_EXCEPTIONS` is not defined by it.

 @see "Exceptions", in the guide.
*/
#define WEBCPP_NO_EXCEPTIONS
#undef WEBCPP_NO_EXCEPTIONS
#endif

// A build that defines a target macro or WEBCPP_HAS_EXCEPTIONS before this header stops at this
// message alone: the rest of the header is the #else, which defines none of them a second time.
#if defined(WEBCPP_TARGET_NATIVE) || defined(WEBCPP_TARGET_WASIP2) ||     \
    defined(WEBCPP_TARGET_WASIP3) || defined(WEBCPP_TARGET_EMSCRIPTEN) || \
    defined(WEBCPP_HAS_EXCEPTIONS)
#error "WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS are defined by <webcpp/config.hpp> alone"
#else

// The target, from the compiler's own macros alone, before Boost.Config: a WebAssembly target
// without a C++ library, wasm32-unknown-unknown, stops inside <boost/config.hpp>, and is named
// here first.
#ifdef __wasip2__
#define WEBCPP_TARGET_WASIP2
#elif defined(__wasip3__)
#define WEBCPP_TARGET_WASIP3
#elif defined(__EMSCRIPTEN__)
#define WEBCPP_TARGET_EMSCRIPTEN
#elif defined(__wasm__)
#error "unsupported target: webcpp builds for wasm32-wasip2, wasm32-wasip3, Emscripten and natively"
#else
/**
 Defined when the program is built natively, for the host's system: for none of wasm32-wasip2,
 wasm32-wasip3 and Emscripten, and not for WebAssembly, which, outside those three targets,
 stops the build.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_NATIVE
#endif

#include <boost/config.hpp>
#include <webcpp/version.hpp>

#if !defined(BOOST_NO_EXCEPTIONS) && !defined(WEBCPP_NO_EXCEPTIONS)
/**
 Defined when webcpp has exceptions: when the compiler has them, so that Boost.Config defines no
 `BOOST_NO_EXCEPTIONS`, and the developer has not defined `WEBCPP_NO_EXCEPTIONS`.

 Each library's `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` is defined when this is not, or when the
 developer defines it alone.

 @see "Exceptions", in the guide.
*/
#define WEBCPP_HAS_EXCEPTIONS
#endif
#endif  // a WEBCPP_TARGET_* macro or WEBCPP_HAS_EXCEPTIONS defined before this header

#endif  // WEBCPP_CONFIG_HPP
