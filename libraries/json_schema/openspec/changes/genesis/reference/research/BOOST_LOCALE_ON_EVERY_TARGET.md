---
title: "Boost.Locale and Boost.Charconv 1.92 built from source on webcpp's targets"
tags: [webcpp, boost, locale, charconv, wasi, emscripten, measurement]
status: active
created: 2026-10-09
---

# Boost.Locale and Boost.Charconv 1.92 on webcpp's targets

Measured 2026-10-09 for the shared-infrastructure plan's `/webcpp//boost_locale` and
`/webcpp//boost_charconv` (`~/.buzz/PLANS/WEBCPP_SHARED_INFRA_PLAN.md`). Sources: Boost 1.92.0's
`libs/locale/src` and `libs/charconv/src`; headers: Homebrew's Boost 1.92.0. No ICU anywhere.
Scripts and probe: `BOOST_LOCALE_ON_EVERY_TARGET/` (`build.sh <target> posix-std|std`,
`locale_probe.cpp` with the `.mo` catalog under `locale/`, `charconv_probe.cpp`).

Configuration built: the 15 shared sources of `libs/locale/build/Jamfile.v2`, `util/gregorian`,
the std backend (5 sources) and the posix backend (5 sources), `BOOST_LOCALE_SOURCE`,
`BOOST_LOCALE_WITH_ICONV=1`, `BOOST_LOCALE_NO_WINAPI_BACKEND=1`; Charconv's `from_chars.cpp` and
`to_chars.cpp` with `BOOST_CHARCONV_SOURCE=1`, no quadmath.

| Target | Boost.Locale | Boost.Charconv |
| --- | --- | --- |
| macOS, Apple clang 21 | 28/28 compile; links with `-liconv`; probe runs: backends posix std, translate from `.mo`, plural, ICU-free `format` (`1,23457e+06` with pt_BR), `to_upper` `ÉCOLE`, iconv conversions (latin1, cp1252, Shift_JIS), date `31/12/1970` | compiles, probe `1234567.5` |
| macOS, g++-15 | 28/28 compile, links with `-liconv` | compiles |
| Linux x86-64, g++-14 and clang-18 (ubuntu 24.04, Docker) | 28/28 compile, iconv from glibc, no `-pthread` needed; probe runs (no pt_BR locale data in the image: `to_upper` leaves `é`) | compiles |
| wasip2, wasi-sdk 34, wasm exceptions (exnref) | 28/28 compile; iconv from wasi-libc; probe runs under wasmtime 47.0.3 with `--dir locale`: translate, plural, conversions; numbers and dates in the C locale's form (`1.23457e+06`, `01/01/71`) | compiles; probe runs |
| wasip3, same | same as wasip2 | compiles; probe runs |
| emscripten 6.0.11, single-threaded, `-fwasm-exceptions` | **does not build**: `shared/date_time.cpp`, `shared/generator.cpp`, `shared/localization_backend.cpp` include `<boost/thread/mutex.hpp>`, and Boost.Config stops: `boost/config/requires_threads.hpp:47:5: error: "Compiler threading support is not turned on. Please set the correct command line options for threading: -pthread ..."`. webcpp's emscripten is single-threaded (no `-pthread`, no SharedArrayBuffer) | compiles; probe runs under node |
| any target, `-fno-exceptions` | does not build (JSON_SCHEMA_ENGINE_SPIKE section 5: 61 `throw` and 14 `try` in its sources, 8 `throw` in its headers, no `BOOST_NO_EXCEPTIONS` path) | `from_chars.cpp`, `to_chars.cpp` compile natively and for wasip2 |
| Windows, MSVC | not measured here; Boost's own Jamfile builds winapi + std backends without iconv on Windows | not measured here |
