// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// How a library builds its own configuration on webcpp's, as every webcpp library's config.hpp
// does, and how a program chooses code for the target it is built for.

#include <webcpp/config.hpp>

#include <cstdio>
#include <string_view>

// tag::library-config[]
// my_library's own macro: defined when webcpp has no exceptions (the compiler has none, or the
// developer defined WEBCPP_NO_EXCEPTIONS), or when the developer defines it alone.
#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(MY_LIBRARY_NO_EXCEPTIONS)
#define MY_LIBRARY_NO_EXCEPTIONS
#endif
// end::library-config[]

namespace {

// tag::target[]
// Where the program's standard output goes, chosen when it is compiled.
constexpr std::string_view output() {
#ifdef WEBCPP_TARGET_EMSCRIPTEN
    return "the JavaScript console";
#elif defined(WEBCPP_TARGET_WASIP2) || defined(WEBCPP_TARGET_WASIP3)
    return "the WASI host's standard output";
#else
    return "the process's standard output";
#endif
}

// end::target[]

}  // namespace

int main() {
    static_assert(!output().empty());
    // tag::main[]
#ifdef MY_LIBRARY_NO_EXCEPTIONS
    std::puts("my_library: exceptions off");
#else
    std::puts("my_library: exceptions on");
#endif
    // end::main[]
}
