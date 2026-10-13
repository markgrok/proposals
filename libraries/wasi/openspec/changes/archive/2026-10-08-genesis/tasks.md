# Tasks

There is no `reference/plan.md`: the library was moved into webcpp by the superproject's stage-3
plan (its Tasks 4 to 8 are the library's; Tasks 1 to 3 are the superproject's b2 modules and
Task 9 is the publication). Each task below points to that task and to the commits of the
library's repository (`reference/source/` is its tree at `6e3c529`). Every task is done.

## 1. Moving the header and the first test

- [x] 1.1 The repository, the headers `entrypoint.hpp` and `response.hpp` and `<webcpp/wasi.hpp>`, renamed `webcpp::wasi::http`, `build.jam`, the two standard worlds, `meta/libraries.json`, and the README's example served on wasip2 and wasip3 (stage-3 plan: Task 4; commit `640d6ae`)

## 2. The tests of every invariant

- [x] 2.1 `response_test`, the served components `http_methods`, `http_long_body`, `http_no_content_type` and `http_two_units`, and the two refusals `neither_target_refused` and `both_targets_refused` (stage-3 plan: Task 5; commit `c1cb4d7`)
- [x] 2.2 The refusals pass only on the guard's own error (`expected-error` of clang's `-verify`), and the response documents each member (commit `96a893d`)
- [x] 2.3 The request is freed only once the response is written, held by `http_content_type_of_target` (commit `6796fe5`)
- [x] 2.4 A status outside 100 to 599 answers an internal error instead of 200 OK, held by `http_bad_status` (commit `501839b`)
- [x] 2.5 A content type the host refuses answers an internal error instead of being dropped, held by `http_bad_content_type` (commit `d26dab1`)

## 3. Doc Comments and the page

- [x] 3.1 Every public symbol has a Doc Comment, and MrDocs builds the reference strict from the bindings of wasip2, the wasip3 branch checked with its own (stage-3 plan: Task 6; commit `61600b8`)
- [x] 3.2 The page `doc/wasi.adoc` and its chapters, with "Building for WASI with Boost", `example/boost_json_command` and `example/boost_json_http` (stage-3 plan: Task 7; commit `884537a`)
- [x] 3.3 The commands of a build by hand are a test, `example/by_hand/build.sh` run by `by_hand`, and the page's fixes (commit `304d8b8`)
- [x] 3.4 The README's build table as the page's, and listings that fit their boxes (commits `9e7fe14`, `bb50ebc`, `3a595fe`, `ce5a46d`, `00ec40d`)
- [x] 3.5 The transcript's curl commands render as written, not as a link and bold text (commit `3075cd0`)

## 4. Own lanes, AGENTS.md and CI

- [x] 4.1 The served tests run in an own lane on wasip2 and wasip3 (stage-3 plan: Task 7A; commit `505f2bb`), and as one own lane per target, `wasip2.wasi.http` (commit `659775a`)
- [x] 4.2 `AGENTS.md` (its build keeps every served program in its lane) and the CI through the superproject's library workflow (stage-3 plan: Task 8; commits `030651c`, `eb716fa`)
- [x] 4.3 The test report's address at the organisation's root (commit `6fe623d`)

## 5. Exceptions

- [x] 5.1 wasip2 builds with exceptions, as wasip3 does; the page names the wasmtime versions that need `-W exceptions=y` and says an exception escaping the main traps on wasip3 (commits `fe9ea92`, `59812a8`, `1a34e22`)
- [x] 5.2 `config.hpp` names `WEBCPP_WASI_NO_EXCEPTIONS`, defined without exceptions or by the developer, with the tests `config` and `config_developer_defined`; `AGENTS.md` states that the handler catches nothing, as decided (commits `f8168ec`, `fc78764`)
- [x] 5.3 `WEBCPP_NO_EXCEPTIONS` disables wasi's exceptions with every webcpp library's, with the test `config_all_off` (commit `c286001`)

## 6. Changelog and publication

- [x] 6.1 A changelog, the page's last appendix, its lines saying what a user sees (commits `9e2fbc3`, `551f742`)
- [x] 6.2 The maintainer's public address in `meta/libraries.json`; published at `webcpporg/wasi` `6e3c529` and as the superproject's submodule `libs/wasi` (stage-3 plan: Task 9; commit `6e3c529`)
