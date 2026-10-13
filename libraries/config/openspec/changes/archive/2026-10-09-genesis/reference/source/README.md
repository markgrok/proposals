# config

config is webcpp's configuration, as Boost.Config is Boost's:
`<webcpp/config.hpp>` tells a program the target it is built for and whether
webcpp has exceptions, and `<webcpp/version.hpp>` webcpp's version. Every
webcpp library's own `config.hpp` includes it first.

It is one of the libraries of [webcpp](https://github.com/webcpporg/webcpp).
It is macros only, declares no namespace, and needs Boost's headers and
nothing to build or link. It runs natively, on wasm32-wasip2, on
wasm32-wasip3 and on Emscripten.

| Macro | Defined |
| --- | --- |
| `WEBCPP_VERSION` | always: webcpp's version as a number, `major * 100000 + minor * 100 + patch`, as `BOOST_VERSION` is Boost's; `100` is 0.1.0 |
| `WEBCPP_LIB_VERSION` | always: webcpp's version as text, `"<major>_<minor>"`, with `"_<patch>"` after it when the patch level is not 0; `"0_1"` is 0.1.0 |
| `WEBCPP_TARGET_WASIP2` | when the compiler defines `__wasip2__`: a build for wasm32-wasip2 |
| `WEBCPP_TARGET_WASIP3` | when the compiler defines `__wasip3__`: a build for wasm32-wasip3 |
| `WEBCPP_TARGET_EMSCRIPTEN` | when the compiler defines `__EMSCRIPTEN__`: a build with Emscripten |
| `WEBCPP_TARGET_NATIVE` | for none of the three, and not for WebAssembly: any other WebAssembly build, such as wasm32-wasip1, stops with the error "unsupported target" |
| `WEBCPP_HAS_EXCEPTIONS` | when neither `BOOST_NO_EXCEPTIONS` nor `WEBCPP_NO_EXCEPTIONS` is defined |
| `WEBCPP_NO_EXCEPTIONS` | by the developer alone, to turn off the exceptions of every webcpp library at once; Boost's are left as the compiler has them |

A library builds its own configuration on it, as
[example/config_quick_start.cpp](example/config_quick_start.cpp) shows:

<!-- include::example/config_quick_start.cpp[tag=library-config,indent=0] -->
```cpp
// my_library's own macro: defined when webcpp has no exceptions (the compiler has none, or the
// developer defined WEBCPP_NO_EXCEPTIONS), or when the developer defines it alone.
#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(MY_LIBRARY_NO_EXCEPTIONS)
#define MY_LIBRARY_NO_EXCEPTIONS
#endif
```

## Building and testing

config is developed inside the webcpp superproject, as a Boost library is
developed inside Boost:

    git clone --recursive https://github.com/webcpporg/webcpp
    cd webcpp
    b2 libs/config/test libs/config/example

builds the tests and the examples natively, runs every example and compares
its output with the `.expected` file beside it.
`toolset=clang-wasip2 testing.launcher=wasmtime` and
`toolset=clang-wasip3 testing.launcher=wasmtime` do the same for WASI, with
the toolsets that `user-config.jam` registers against wasi-sdk, and
`toolset=emscripten` for Emscripten, whose programs b2's toolset runs with
node itself. A b2 project uses config through `/webcpp/config//config`.

## Documentation

config's page, with its API reference, is published at
<https://webcpporg.github.io/webcpp/libs/config/>; `b2 libs/config/doc`
builds it into `doc/html/index.html`.

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE_1_0.txt).
