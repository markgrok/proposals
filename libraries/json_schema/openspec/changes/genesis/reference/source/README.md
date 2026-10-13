# json_schema

json_schema is a header-only JSON Schema validator in C++20, one of the
libraries of [webcpp](https://github.com/webcpporg/webcpp).

A program loads a JSON Schema (a `boost::json::value`, or JSON text read with
exact numbers), the library compiles it once into an immutable plan, and
validates any number of instances against it, from any number of threads,
answering a boolean (`is_valid`) or a structured outcome (`validate`) that
renders as the specification's output formats.

Its names are in the namespace `webcpp::json_schema`. Every operation that
can fail returns its error as a value, a `result` whose error names where the
failure happened; the library works without exceptions. It allocates only
through the allocators a program gives it. json_schema needs Boost's headers,
Boost.JSON's definitions, which the superproject compiles, and SRELL, which
the superproject brings as a submodule; there is nothing of its own to build
or link. It runs natively, on wasm32-wasip2, on wasm32-wasip3 and on
Emscripten.

## Building and testing

json_schema is developed inside the webcpp superproject, as a Boost library
is developed inside Boost:

    git clone --recursive https://github.com/webcpporg/webcpp
    cd webcpp
    b2 libs/json_schema/test

builds and runs the tests natively.
`toolset=clang-wasip2 testing.launcher=wasmtime` and
`toolset=clang-wasip3 testing.launcher=wasmtime` do the same for WASI, with
the toolsets that `user-config.jam` registers against wasi-sdk, and
`toolset=emscripten` for Emscripten, whose programs run under node. A b2
project uses json_schema through `/webcpp/json_schema//json_schema`.

## Documentation

json_schema's page, with its API reference, is published at
<https://webcpporg.github.io/webcpp/libs/json_schema/>.

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE_1_0.txt).
