// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A WebAssembly build for none of wasm32-wasip2, wasm32-wasip3 and Emscripten stops with
// config.hpp's message rather than read as native: test/Jamfile builds this file so.

// expected-error@webcpp/config.hpp:* {{unsupported target}}
#include <webcpp/config.hpp>
