# config Specification

## Purpose
webcpp::config is webcpp's configuration, the counterpart of Boost.Config: a header-only,
macros-only C++20 library that every webcpp library includes first. `<webcpp/config.hpp>` tells a
program the target it is built for (native, wasm32-wasip2, wasm32-wasip3 or Emscripten) and
whether webcpp has exceptions, and `<webcpp/version.hpp>` webcpp's version. These requirements
describe 0.1.0 as built (`../../changes/archive/2026-10-09-genesis/reference/source/`).

## Requirements

### Requirement: Library: Macros only, header-only, no namespace
The library SHALL be header-only and macros only, as Boost.Config is: it MUST declare no namespace, no type and no function, it needs Boost's headers and nothing to build or link, and its macros are the collection's own, `WEBCPP_*` (never `WEBCPP_CONFIG_*`), where every other library's are `WEBCPP_<LIBRARY>_*`. Its include guards are `WEBCPP_CONFIG_HPP` for `<webcpp/config.hpp>` and `WEBCPP_VERSION_HPP` for `<webcpp/version.hpp>`. It is webcpp's own and ports nothing. Its sources are C++20-compatible and carry the Boost Software License, Version 1.0 notice "Copyright (c) 2026 WebCpp.org".

#### Scenario: A header declares no name but macros
- **WHEN** `<webcpp/config.hpp>` and `<webcpp/version.hpp>` are read
- **THEN** they contain only preprocessor directives and comments, with no `namespace`, `struct`, `class`, `enum`, `using`, variable or function

#### Scenario: A program that includes only config links nothing
- **WHEN** a program includes `<webcpp/config.hpp>` and is built with only `/webcpp/config//config`
- **THEN** it compiles and links without any library of config's own, since config builds nothing

### Requirement: Library: The two public headers
The library SHALL ship exactly two public headers, `include/webcpp/config.hpp` and `include/webcpp/version.hpp`, named as Boost's `<boost/config.hpp>` and `<boost/version.hpp>` are. `<webcpp/version.hpp>` MUST lie outside `webcpp/config/` and compile alone as the first include of a translation unit. `<webcpp/config.hpp>` MUST include `<boost/config.hpp>` and `<webcpp/version.hpp>`, so that a program that includes it has Boost.Config's macros and webcpp's version too.

#### Scenario: config.hpp brings Boost.Config and the version
- **WHEN** a translation unit includes only `<webcpp/config.hpp>`
- **THEN** `BOOST_CONFIG_HPP`, `WEBCPP_VERSION` and `WEBCPP_LIB_VERSION` are defined (`test/exceptions_test.cpp` stops with "`<webcpp/config.hpp>` does not include `<boost/config.hpp>`" otherwise)

#### Scenario: version.hpp compiles alone
- **WHEN** `test/version_test.cpp` includes `<webcpp/version.hpp>` first and alone
- **THEN** it compiles, since `version.hpp` needs no other header

### Requirement: Library: Every webcpp library includes config first
The library SHALL be the first include of every webcpp library: each library's own `<webcpp/<library>/config.hpp>` MUST include `<webcpp/config.hpp>` first and every public header of the library includes its own configuration first, so that whatever a program includes of webcpp, config is read before it. Each library keeps its own `WEBCPP_<LIBRARY>_NO_EXCEPTIONS`, defined by the two-line pattern `#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_<LIBRARY>_NO_EXCEPTIONS)` then `#define WEBCPP_<LIBRARY>_NO_EXCEPTIONS`, which `example/config_quick_start.cpp` shows with `MY_LIBRARY_NO_EXCEPTIONS` between the markers `tag::library-config[]` and `end::library-config[]`.

#### Scenario: A library builds its configuration on config
- **WHEN** `example/config_quick_start.cpp` is compiled with webcpp having exceptions
- **THEN** `MY_LIBRARY_NO_EXCEPTIONS` is not defined and the program prints `my_library: exceptions on`

#### Scenario: A library's macro follows webcpp's
- **WHEN** the same program is compiled with `-DWEBCPP_NO_EXCEPTIONS`, or with exceptions off
- **THEN** `WEBCPP_HAS_EXCEPTIONS` is not defined, `MY_LIBRARY_NO_EXCEPTIONS` is, and it would print `my_library: exceptions off`

### Requirement: Library: Names the macros reserve
The library SHALL reserve the macro families `WEBCPP_HAS_*`, `WEBCPP_LIB_*`, `WEBCPP_NO_*`, `WEBCPP_TARGET_*` and `WEBCPP_VERSION*` to itself, and the tests' macros are `WEBCPP_TEST_*`. Consequently no webcpp library MUST be named `target`, `has`, `no`, `lib`, `version` or `test`, alone or followed by an underscore and more (`has_x`, in any case), since its `WEBCPP_<NAME>_*` macros would meet config's or the tests'. The documentation page says so (`doc/introduction.adoc`), and the superproject's tooling refuses such a name (`tools/doc/libraries.py`).

#### Scenario: The page names the reserved words
- **WHEN** the introduction of the page is read
- **THEN** it says that no library's name is `target`, `has`, `no`, `lib`, `version` or `test`, alone or followed by an underscore and more

#### Scenario: A reserved library name is refused
- **WHEN** a library under `libs/` named `has_x` or `version` is added to the superproject
- **THEN** the superproject's documentation tooling stops with "a library is not named ..." (a check of the superproject, not of config)

### Requirement: Library: Build target
`build.jam` SHALL declare `project /webcpp/config` and one alias `config` with no sources whose usage requirements are `<include>include` and `<library>/boost//headers`, so that a b2 project uses config through `/webcpp/config//config` and receives config's headers and Boost's. It MUST NOT name any compiled library: config needs Boost's headers and nothing to build or link.

#### Scenario: The test and example projects use the alias
- **WHEN** `test/Jamfile` and `example/Jamfile` are read
- **THEN** each begins with `project : requirements <library>/webcpp/config//config ;`

#### Scenario: Include paths reach the compile line
- **WHEN** a program names `/webcpp/config//config`
- **THEN** config's `include` directory and Boost's headers are on its include path

### Requirement: Library: Library metadata
`meta/libraries.json` SHALL describe the library with `"key": "config"`, `"name": "config"`, `"authors": ["R"]`, the description "webcpp's configuration, which every webcpp library includes first: the target a program is built for, whether webcpp has exceptions, and webcpp's version.", `"category": ["Miscellaneous"]`, `"maintainers": ["R <github.overboard312@passmail.net>"]`, `"cxxstd": "20"` and `"port-of": null`.

#### Scenario: The metadata is its own
- **WHEN** `meta/libraries.json` is read
- **THEN** `key` is `config`, `cxxstd` is `"20"` and `port-of` is `null`, since config is not a port

### Requirement: Library: Repository files and rules
The library's repository SHALL hold `LICENSE_1_0.txt` (the Boost Software License), `.gitignore` (`doc/html/`), `.gitattributes` (`* text=auto eol=lf` and `*.bat text eol=crlf`), `README.md`, `AGENTS.md` and `.github/workflows/ci.yml`. `AGENTS.md` MUST send the reader to webcpp's rulebook first and hold only what is config's own: the macros are the collection's `WEBCPP_*` and no namespace is declared; `<webcpp/version.hpp>` lies outside `webcpp/config/`; the version is written once, in `version.hpp`, and moves only by the release procedure right after a tag; the target comes from the compiler and never from b2, and the target block comes before Boost.Config.

#### Scenario: AGENTS.md states config's own rules
- **WHEN** `AGENTS.md` is read
- **THEN** it has four bullets: macros only, `version.hpp` outside `webcpp/config/`, the version written once, and the target from the compiler

#### Scenario: Built docs are not tracked
- **WHEN** `b2 libs/config/doc` has built the page
- **THEN** `doc/html/` is ignored by git

### Requirement: Version: WEBCPP_VERSION
`<webcpp/version.hpp>` SHALL define `WEBCPP_VERSION` as a number, `major * 100000 + minor * 100 + patch`, as `BOOST_VERSION` is Boost's, a macro (not a constant) so that `#if` compares it, always defined. Its value in 0.1.0 MUST be `100`. `WEBCPP_VERSION % 100` is the patch level, `WEBCPP_VERSION / 100 % 1000` the minor version and `WEBCPP_VERSION / 100000` the major one.

#### Scenario: Version 0.1.0
- **WHEN** `<webcpp/version.hpp>` is included
- **THEN** `WEBCPP_VERSION` is `100`, so major is 0, minor is 1 and patch is 0, and `#if WEBCPP_VERSION >= 100` holds

#### Scenario: Version 1.2.3 would be 100203
- **WHEN** a version is major 1, minor 2, patch 3
- **THEN** `WEBCPP_VERSION` would be `100203`, with `WEBCPP_VERSION % 100` equal to 3, `WEBCPP_VERSION / 100 % 1000` equal to 2 and `WEBCPP_VERSION / 100000` equal to 1

### Requirement: Version: WEBCPP_LIB_VERSION
`<webcpp/version.hpp>` SHALL define `WEBCPP_LIB_VERSION` as a string literal, `"<major>_<minor>"` with `"_<patch>"` after it only when the patch level is not 0, as `BOOST_LIB_VERSION` is Boost's. Its value in 0.1.0 MUST be `"0_1"`. `test/version_test.cpp` MUST check by `static_assert` that the text is digits separated by single underscores, that its major and minor numbers are `WEBCPP_VERSION`'s, that it has three numbers exactly when the patch level is not 0, and that a third number is the patch level.

#### Scenario: Version 0.1.0 as text
- **WHEN** `<webcpp/version.hpp>` is included
- **THEN** `WEBCPP_LIB_VERSION` is `"0_1"`

#### Scenario: Version 1.2.3 as text
- **WHEN** a version is major 1, minor 2, patch 3
- **THEN** `WEBCPP_LIB_VERSION` would be `"1_2_3"`, and `test/version_test.cpp` accepts it as three numbers whose third is the patch level

### Requirement: Version: Written in one place
The version SHALL be written in one place only, `include/webcpp/version.hpp`, which the superproject's build and its release tooling read (the `#define WEBCPP_VERSION` line itself, hence the `NOLINTNEXTLINE(modernize-macro-to-enum)` above it). It MUST move to the next version only by the release procedure of webcpp's `AGENTS.md`, right after a tag, so that between two releases it names the version in preparation. The version has three numbers, as Boost's tags do (`webcpp-0.1.0`).

#### Scenario: The version is not repeated in config
- **WHEN** the sources of the library are searched for the literal `0_1` and for `WEBCPP_VERSION 100`
- **THEN** each occurs as a definition only in `include/webcpp/version.hpp` (the page and the README describe them in prose)

### Requirement: Target: Exactly one target macro
`<webcpp/config.hpp>` SHALL define exactly one of `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN` and `WEBCPP_TARGET_NATIVE`, the one that names the target the program is built for, tested in this order from the compiler's own macros. Each macro is a plain definition without a value.

#### Scenario: One macro in every build
- **WHEN** `test/target_test.cpp` includes `<webcpp/config.hpp>` in a build for any supported target
- **THEN** `defined(WEBCPP_TARGET_NATIVE) + defined(WEBCPP_TARGET_WASIP2) + defined(WEBCPP_TARGET_WASIP3) + defined(WEBCPP_TARGET_EMSCRIPTEN)` equals 1, or the test stops with "config.hpp defines other than exactly one WEBCPP_TARGET_* macro"

#### Scenario: The toolset tells the test, the compiler tells config
- **WHEN** `test/Jamfile` builds `target` with `clang-wasip2`, `clang-wasip3` or `emscripten`
- **THEN** it defines `WEBCPP_TEST_TARGET_WASIP2`, `WEBCPP_TEST_TARGET_WASIP3` or `WEBCPP_TEST_TARGET_EMSCRIPTEN` and the test requires the matching `WEBCPP_TARGET_*` (and `WEBCPP_TARGET_NATIVE` in every other build)

### Requirement: Target: wasip2, wasip3 and Emscripten
`WEBCPP_TARGET_WASIP2` SHALL be defined when the compiler defines `__wasip2__` (wasi-sdk's clang for `wasm32-wasip2`, a component of WASI 0.2), `WEBCPP_TARGET_WASIP3` when it defines `__wasip3__` (`wasm32-wasip3`, WASI 0.3), and `WEBCPP_TARGET_EMSCRIPTEN` when it defines `__EMSCRIPTEN__` (Emscripten, for a browser or for node). `__wasip2__` is tested first, then `__wasip3__`, then `__EMSCRIPTEN__`. The macros come from the compiler alone, never from the build system, so a program compiled by hand with a plain clang++ or wasi-sdk's gets the macro b2 would give it.

#### Scenario: A wasm32-wasip2 build
- **WHEN** `test/target_test.cpp` is built with `toolset=clang-wasip2` and `WEBCPP_TEST_TARGET_WASIP2`
- **THEN** `WEBCPP_TARGET_WASIP2` is defined, or the test stops with "a build for wasm32-wasip2 does not define WEBCPP_TARGET_WASIP2"

#### Scenario: An Emscripten build
- **WHEN** `test/target_test.cpp` is built with `toolset=emscripten`
- **THEN** `WEBCPP_TARGET_EMSCRIPTEN` is defined, since em++ 6.0.11 defines `__EMSCRIPTEN__`

### Requirement: Target: native
`WEBCPP_TARGET_NATIVE` SHALL be defined when the build is for none of wasm32-wasip2, wasm32-wasip3 and Emscripten and the compiler does not define `__wasm__`: the host's system, with any compiler for it (GCC, Clang or MSVC).

#### Scenario: A native build
- **WHEN** `test/target_test.cpp` is built natively
- **THEN** `WEBCPP_TARGET_NATIVE` is defined, or the test stops with "a native build does not define WEBCPP_TARGET_NATIVE"

#### Scenario: The quick start chooses by target
- **WHEN** `example/config_quick_start.cpp` evaluates its constexpr `output()` natively
- **THEN** it returns "the process's standard output" (the JavaScript console under Emscripten, the WASI host's standard output under wasip2 and wasip3), and `static_assert(!output().empty())` holds

### Requirement: Target: Unsupported WebAssembly builds stop
`<webcpp/config.hpp>` SHALL stop a build for which the compiler defines `__wasm__` and that is none of wasip2, wasip3 and Emscripten with the preprocessor error `unsupported target: webcpp builds for wasm32-wasip2, wasm32-wasip3, Emscripten and natively`, and MUST NOT read it as native. A new target will get a macro of its own.

#### Scenario: wasm32-wasip1
- **WHEN** `test/unsupported_test.cpp` is built with wasi-sdk's clang and `--target=wasm32-wasip1`
- **THEN** clang `-verify` sees the error "unsupported target" from `webcpp/config.hpp`

#### Scenario: __wasm__ defined natively
- **WHEN** `test/unsupported_test.cpp` is built natively with `-D__wasm__`
- **THEN** clang `-verify` sees the same error, so the test runs in every native clang lane

### Requirement: Target: The target block comes before Boost.Config
The target block SHALL come before `#include <boost/config.hpp>`, so that a WebAssembly target without a C++ library, wasm32-unknown-unknown, on which Boost.Config stops at a header that is not there, gets config's message first. The order MUST be pinned by a test.

#### Scenario: wasm32-unknown-unknown
- **WHEN** `test/unsupported_test.cpp` is built with wasi-sdk's clang and `--target=wasm32-unknown-unknown`
- **THEN** clang `-verify` sees the error "unsupported target" from `webcpp/config.hpp`, before any fatal error of `<boost/config.hpp>`

### Requirement: Target: Predefined macros stop at one message
`<webcpp/config.hpp>` SHALL stop with the single error `WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS are defined by <webcpp/config.hpp> alone` when any of `WEBCPP_TARGET_NATIVE`, `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN` or `WEBCPP_HAS_EXCEPTIONS` is defined before it, on the command line or in a header of the program's own, and the rest of the header MUST be the `#else` of that test, so that no macro is defined a second time and nothing more is reported.

#### Scenario: A target defined on the command line
- **WHEN** `test/predefined_test.cpp` is built with `-DWEBCPP_TARGET_NATIVE`, `-DWEBCPP_TARGET_WASIP2`, `-DWEBCPP_TARGET_WASIP3` or `-DWEBCPP_TARGET_EMSCRIPTEN`, with warnings as errors off
- **THEN** clang `-verify` sees exactly the error "are defined by `<webcpp/config.hpp>` alone" and no redefinition warning

#### Scenario: WEBCPP_HAS_EXCEPTIONS defined on the command line
- **WHEN** `test/predefined_test.cpp` is built with `-DWEBCPP_HAS_EXCEPTIONS`, with warnings as errors off
- **THEN** clang `-verify` sees the same single error

### Requirement: Exceptions: WEBCPP_HAS_EXCEPTIONS
`<webcpp/config.hpp>` SHALL define `WEBCPP_HAS_EXCEPTIONS` exactly when webcpp has exceptions: when the compiler has them, so that Boost.Config defines no `BOOST_NO_EXCEPTIONS`, and the developer has not defined `WEBCPP_NO_EXCEPTIONS`. A build without exceptions, such as one with `-fno-exceptions` (b2's `exception-handling=off`), MUST NOT have it. Every target of webcpp builds with exceptions on by default.

#### Scenario: A build with exceptions
- **WHEN** `test/exceptions_test.cpp` is built as the build asks (exceptions on)
- **THEN** `BOOST_NO_EXCEPTIONS` is not defined and `WEBCPP_HAS_EXCEPTIONS` is, or the test stops with "WEBCPP_HAS_EXCEPTIONS is not defined exactly when the build has exceptions"

#### Scenario: A build with exceptions off
- **WHEN** `test/exceptions_test.cpp` is built as `exceptions_off`, with `exception-handling=off` and `WEBCPP_TEST_CONFIG_OFF`
- **THEN** `BOOST_NO_EXCEPTIONS` is defined and `WEBCPP_HAS_EXCEPTIONS` is not (the test first checks that `BOOST_NO_EXCEPTIONS` is defined, so a flag that was not applied is refused)

### Requirement: Exceptions: WEBCPP_NO_EXCEPTIONS is the developer's
`WEBCPP_NO_EXCEPTIONS` SHALL be defined by the developer alone, on the compiler's command line (`-DWEBCPP_NO_EXCEPTIONS`) or in a b2 project's requirements (`<define>WEBCPP_NO_EXCEPTIONS`), to turn off the exceptions of every webcpp library at once, even in a build that has them. config MUST NOT define it, and it MUST NOT define `BOOST_NO_EXCEPTIONS`: Boost's own exceptions are left as the compiler has them. With it, `WEBCPP_HAS_EXCEPTIONS` is not defined, and so each library's `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` is.

#### Scenario: The developer turns exceptions off in a build that has them
- **WHEN** `test/exceptions_test.cpp` is built as `exceptions_developer_off`, with `<define>WEBCPP_NO_EXCEPTIONS` and `WEBCPP_TEST_CONFIG_DEVELOPER_OFF`
- **THEN** `WEBCPP_HAS_EXCEPTIONS` is not defined and, when the compiler has exceptions, `BOOST_NO_EXCEPTIONS` is not defined either

#### Scenario: config never defines the developer's macro
- **WHEN** `test/exceptions_test.cpp` is built without `WEBCPP_NO_EXCEPTIONS` on the command line
- **THEN** `WEBCPP_NO_EXCEPTIONS` is not defined after the include, or the test stops with "config.hpp defines WEBCPP_NO_EXCEPTIONS, which is the developer's alone"

### Requirement: Exceptions: A library's own macro
A webcpp library's own `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` SHALL be defined when `WEBCPP_HAS_EXCEPTIONS` is not, and a developer MAY still define it alone, to turn off the exceptions of one library and keep the others'. A program sets `WEBCPP_NO_EXCEPTIONS` once, for every webcpp library alike, without knowing which of them throws.

#### Scenario: One library off, the others on
- **WHEN** a program is built with `-DMY_LIBRARY_NO_EXCEPTIONS` and exceptions on, and includes `<webcpp/config.hpp>` then the quick start's configuration
- **THEN** `WEBCPP_HAS_EXCEPTIONS` is defined and `MY_LIBRARY_NO_EXCEPTIONS` stays defined, so the program prints `my_library: exceptions off`

### Requirement: Tests: Compile-time tests declared in test/Jamfile
The library's tests SHALL be compiled and never run, since what config does it does when a program is compiled. `test/Jamfile` MUST declare them, with `project : requirements <library>/webcpp/config//config ;`, `import webcpp ;` and `webcpp.targets native wasip2 wasip3 emscripten ;`, using three rules of the superproject's `webcpp` module: `webcpp.headers-alone` (each public header as the only include of its translation unit), `webcpp.compile` (a translation unit that checks the macros with `#error` and `static_assert` and passes when it compiles) and `webcpp.compile-diagnostic` (a source that passes when clang stops it with the error it states). It declares fourteen tests: `alone-config`, `version`, `target`, `exceptions`, `exceptions_off`, `exceptions_developer_off`, `predefined_target`, `predefined_wasip2`, `predefined_wasip3`, `predefined_emscripten`, `predefined_exceptions`, `unsupported_wasip1`, `unsupported_wasm32_unknown` and `unsupported_wasm`.

#### Scenario: The suite is built from the superproject
- **WHEN** `b2 -a libs/config/test libs/config/example` runs from the root of a webcpp checkout
- **THEN** every test is built and the example is built, run and compared with its `.expected` file

#### Scenario: One test alone
- **WHEN** `b2 libs/config/test//target` runs
- **THEN** only the `target` test is built

### Requirement: Tests: The headers-alone test
The test `alone-config` SHALL compile `<webcpp/config.hpp>` as the only include of its translation unit (`webcpp.headers-alone config : ../include ;`). `<webcpp/version.hpp>` lies outside `webcpp/config/`, so headers-alone does not take it, and the test `version` compiles it alone instead.

#### Scenario: config.hpp is self-contained
- **WHEN** `alone-config` is built
- **THEN** it compiles with nothing included before the header

### Requirement: Tests: Diagnostic tests run on clang
A `webcpp.compile-diagnostic` test SHALL be built on b2's clang toolset alone (native clang, `clang-wasip2` and `clang-wasip3`), since `-verify`, which reads the error a source states, is clang's: GCC, MSVC and Emscripten skip the `predefined_*` and `unsupported_*` tests, which show as grey cells in the test matrix. `unsupported_wasip1` and `unsupported_wasm32_unknown` MUST be built for the wasip2 target (`--target=wasm32-wasip1`, `--target=wasm32-unknown-unknown`), `unsupported_wasm` for the native target (`-D__wasm__`), and each `predefined_*` test is built with warnings as errors off, so that a macro config went on to define again would be a warning `-verify` sees and not an error it ignores.

#### Scenario: GCC skips the diagnostics
- **WHEN** the tests are built with GCC
- **THEN** `predefined_*` and `unsupported_*` are skipped and the other tests pass

#### Scenario: Native clang runs the unsupported test
- **WHEN** `unsupported_wasm` is built by native clang with `<define>__wasm__`
- **THEN** it passes because config stops with "unsupported target"

### Requirement: Tests: Every target
`test/Jamfile` and `example/Jamfile` SHALL declare the four targets, `webcpp.targets native wasip2 wasip3 emscripten`, and b2 builds each program for those alone: native (any toolset that is not one of the three below, GCC, Clang and MSVC among them), wasip2 (`clang-wasip2`, run by wasmtime), wasip3 (`clang-wasip3`, run by wasmtime) and emscripten (`emscripten`, whose programs b2's toolset runs with node itself, so it takes no `testing.launcher`). A result that counts is built from scratch with `b2 -a`.

#### Scenario: The four invocations
- **WHEN** a developer runs `b2 libs/config/test libs/config/example`, then the same with `toolset=clang-wasip2 testing.launcher=wasmtime`, `toolset=clang-wasip3 testing.launcher=wasmtime` and `toolset=emscripten`
- **THEN** each builds every test and builds and runs the example for that target

### Requirement: Tests: The test matrix
The library's continuous integration SHALL run its tests and examples in lanes, one toolset each (the native compilers, wasip2, wasip3 and emscripten), each built from scratch with `b2 -a --dump-tests --out-xml=<lane>.xml toolset=<toolset> libs/config/test libs/config/example`, and the superproject's `tools/report/report.py` merges the lanes into a matrix published at https://webcpporg.github.io/webcpp/report/, where a test b2 skips in a lane is a grey cell.

#### Scenario: A skipped test is grey
- **WHEN** the GCC lane skips `predefined_target`
- **THEN** the matrix shows that cell grey and not failed

### Requirement: Examples: The quick start
The library SHALL ship one example, `example/config_quick_start.cpp`, whose `example/Jamfile` is `webcpp.targets native wasip2 wasip3 emscripten ;` and `webcpp.example config_quick_start.cpp ;`: b2 builds it, runs it and compares what it printed with `example/config_quick_start.expected`; a program that returns anything but 0 fails too. It holds three regions the page includes, `tag::library-config[]`, `tag::target[]` and `tag::main[]`, defines `MY_LIBRARY_NO_EXCEPTIONS` by the library pattern, chooses the name of where standard output goes by target in a `constexpr std::string_view output()`, and prints one line.

#### Scenario: The quick start's output
- **WHEN** `example/config_quick_start.cpp` is built with exceptions and run
- **THEN** it prints exactly `my_library: exceptions on` (one line, the content of `config_quick_start.expected`) on native, wasm32-wasip2, wasm32-wasip3 and Emscripten

### Requirement: Documentation: The page
`doc/Jamfile` SHALL declare the page (`webcpp.doc config : config.adoc ;`) and its reference (`webcpp.reference config ;`), and `doc/config.adoc`, titled "config: webcpp's configuration for C++", MUST include, in this order, `introduction.adoc`, `targets.adoc`, `exceptions.adoc`, `version.adoc`, `testing.adoc`, a section `[#reference]` "Reference" that includes the generated reference (`include::{reference}[leveloffset=+1]`) and the appendix `changelog.adoc`, with `:attribute-missing: warn` so a missing attribute fails the build. The page is published at https://webcpporg.github.io/webcpp/libs/config/ and `b2 libs/config/doc` builds it into `doc/html/index.html`. Section anchors are `config-introduction`, `config-targets`, `config-exceptions`, `config-version`, `config-testing` (with `config-testing-targets` and `config-testing-report`) and `config-changelog`.

#### Scenario: The page builds
- **WHEN** `b2 -a libs/config/doc` runs
- **THEN** the page is built with its reference and no unresolved attribute

### Requirement: Documentation: Introduction, targets, exceptions and version sections
The page SHALL explain, in `introduction.adoc`, config as Boost.Config's counterpart, the names it reserves, and the quick start's `library-config` and `main` regions with its expected output; in `targets.adoc`, the quick start's `target` region and a table of the four macros with the compiler's macro that defines each and the target (wasm32-wasip2 built with wasi-sdk's clang, wasm32-wasip3, Emscripten, the host), that the macros come from the compiler never the build system, the `unsupported target` message and why the header names the target before `<boost/config.hpp>`, and the message for a macro defined before the header; in `exceptions.adoc`, `WEBCPP_HAS_EXCEPTIONS`, `WEBCPP_NO_EXCEPTIONS` and each library's own macro, and that Boost's exceptions are left as the compiler has them; in `version.adoc`, `WEBCPP_VERSION` and `WEBCPP_LIB_VERSION` with the examples 0.1.0 (`100`, `"0_1"`) and 1.2.3 (`100203`, `"1_2_3"`) and that `version.hpp` is the one place the version is written, moved by the release procedure right after each release.

#### Scenario: The targets table
- **WHEN** `doc/targets.adoc` is rendered
- **THEN** it has a three-column table of `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN` and `WEBCPP_TARGET_NATIVE`, each with its compiler macro (`__wasip2__`, `__wasip3__`, `__EMSCRIPTEN__`, "none of the three, nor `__wasm__`")

### Requirement: Documentation: The testing section
`doc/testing.adoc` SHALL describe how config is tested: that its tests are compiled and never run, a table of what each test checks (`alone-config`, `version`, `target`, `exceptions`, `exceptions_off`, `exceptions_developer_off`, the `predefined_*` and the `unsupported_*`), a table per target (native, wasip2, wasip3, emscripten) with its toolset, the number of tests declared and examples, filled by the attributes `{n-tests}`, `{n-tests-native}`, `{n-tests-wasip2}`, `{n-tests-wasip3}`, `{n-tests-emscripten}`, `{n-examples-native}`, `{n-examples-wasip2}`, `{n-examples-wasip3}` and `{n-examples-emscripten}` that the superproject's tooling computes, the four `b2` invocations of the lanes, and the test matrix. A count is of the tests a target's Jamfile declares, and the page says that b2 skips the diagnostic tests with GCC, MSVC and Emscripten.

#### Scenario: Counts are computed, not typed
- **WHEN** `doc/testing.adoc` is read
- **THEN** every count is an attribute reference, none a number typed in the file

### Requirement: Documentation: The reference of eight macros
The headers SHALL document every macro the reference lists with a `/** */` comment, and the reference MUST list eight: `WEBCPP_VERSION`, `WEBCPP_LIB_VERSION`, `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN`, `WEBCPP_TARGET_NATIVE`, `WEBCPP_HAS_EXCEPTIONS` and `WEBCPP_NO_EXCEPTIONS`, without the include guards. Because MrDocs parses webcpp natively and with exceptions, where only `WEBCPP_TARGET_NATIVE` and `WEBCPP_HAS_EXCEPTIONS` are defined, `config.hpp` MUST hold an `#ifdef __MRDOCS__` block that defines and at once undefines `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN` and `WEBCPP_NO_EXCEPTIONS` under their own comments. Each comment points to the page (`@see "Targets"`, `@see "Exceptions"`, `@see "Version"`), and a compiler macro in a comment is written with an escaped leading underscore (`\__wasip2__`) so that Asciidoctor keeps its underscores. The superproject's reference tooling treats config's prefix as `WEBCPP_` and requires no namespace of it.

#### Scenario: The reference lists the macros
- **WHEN** the reference is built
- **THEN** it lists `WEBCPP_VERSION`, `WEBCPP_LIB_VERSION`, the four `WEBCPP_TARGET_*`, `WEBCPP_HAS_EXCEPTIONS` and `WEBCPP_NO_EXCEPTIONS`, and neither include guard

#### Scenario: A predefined macro keeps its underscores
- **WHEN** the reference renders the comment of `WEBCPP_TARGET_WASIP2`
- **THEN** it shows `__wasip2__` with both leading underscores

### Requirement: Documentation: The changelog
`doc/changelog.adoc` SHALL be the page's last appendix (`[appendix]`, anchor `config-changelog`, title "Changelog"), with an "Unreleased" section (anchor `config-changelog-unreleased`) on top holding its entries between `// tag::unreleased[]` and `// end::unreleased[]`, newest first, one bullet per user-visible change. Its first entry MUST be "Added the library, with webcpp's target, exceptions and version macros.".

#### Scenario: The first entry
- **WHEN** `doc/changelog.adoc` is read
- **THEN** its Unreleased region has the single bullet "Added the library, with webcpp's target, exceptions and version macros."

### Requirement: Documentation: The README
`README.md` SHALL present config as webcpp's configuration (Boost.Config's counterpart) that runs natively, on wasm32-wasip2, on wasm32-wasip3 and on Emscripten, and carry a table of the eight macros with when each is defined, the library-config region of the example, the commands that build and test it (`git clone --recursive https://github.com/webcpporg/webcpp`, then `b2 libs/config/test libs/config/example`, the wasip2, wasip3 and Emscripten toolsets, and `/webcpp/config//config` for a b2 project), the documentation link https://webcpporg.github.io/webcpp/libs/config/, and the licence.

#### Scenario: The README table
- **WHEN** `README.md` is read
- **THEN** its table has the rows `WEBCPP_VERSION`, `WEBCPP_LIB_VERSION`, `WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN`, `WEBCPP_TARGET_NATIVE`, `WEBCPP_HAS_EXCEPTIONS` and `WEBCPP_NO_EXCEPTIONS`

### Requirement: CI: The superproject's reusable workflow
`.github/workflows/ci.yml` SHALL name the workflow `CI`, run on pushes to `main`, on pull requests and on `workflow_dispatch`, grant `contents: read`, use the concurrency group `${{ github.workflow }}-${{ github.ref }}` with `cancel-in-progress: ${{ github.ref != 'refs/heads/main' }}` (a run on main is the record of main and is never cut short by the next; a pull request's newer push replaces its older run), and have one job that uses `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: config`, which builds the commit inside a checkout of webcpp on every lane config declares, with its page, the lint and the test matrix.

#### Scenario: The job names the library
- **WHEN** `ci.yml` is read
- **THEN** the job `ci` has `uses: webcpporg/webcpp/.github/workflows/library.yml@main` and `with: library: config`
