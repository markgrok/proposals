# Tasks

Every task of 0.1.0, retroactively. The library was built in a worktree of the superproject from
a design (`reference/design-source.md`) and an owner's draft (`reference/draft.md`); the
implementation plan of 2026-10-08 is not part of this proposal's reference, so each task points
to the commits of the library's repository (`git log` of https://github.com/webcpporg/pratt, 42
commits, 2026-10-08 to 2026-10-12, all as `R`) and to the sections of the design. A checked task
is done in `reference/source/`.

## 1. The skeleton and the errors (design 2.1 and 3)

- [x] 1.1 The repository, `build.jam`, `meta/libraries.json` (`"port-of": null`), `meta/include-boundaries.json`, the licence, the `test/Jamfile` and `test/require.hpp` (commit `3305002`)
- [x] 1.2 The errors 1 to 9, their category with the identity `0x7072617474000000`, `make_error_code`, `is_error_code_enum`, tested first in `test/errors_test.cpp` (commit `3305002`)
- [x] 1.3 The category's message into a caller's buffer, tested at sizes 0, 1, 9 and whole (commit `6a674ab`)
- [x] 1.4 Errors built in place, so that a number type an error converts to still works (commit `b01a235`)
- [x] 1.5 An example's standard input: the superproject's `webcpp.example` feeds `<name>.input` to the program (design 6.4; made in the superproject, which pratt's REPLs depend on) and the library's `.gitattributes` keeps `*.input` byte for byte

## 2. The engine (design 3)

- [x] 2.1 The concepts `token_stream` and `grammar_for`, the `parser`, `parse` and the six helpers, with the depth limit and `nesting_too_deep`, tested with the sexpr grammar and a table grammar (commits `9d9ce38`, `7cafa44`)
- [x] 2.2 The recursion exempt from `misc-no-recursion` on its own lines, and no directory hiding a recursion from the lint (commits `7cafa44`, `4bbf911`)
- [x] 2.3 The convenience header names the header it leaves out, the calculator (commit `58eff2a`)

## 3. The calculator's number concept and lexer (design 5)

- [x] 3.1 The `number` concept, the token kinds and the lazy lexer over `decimal64_t` and `double` (commit `bc0e924`)
- [x] 3.2 A long literal lexes to its nearest value, or near it, and the lexer says what it does past a number's range (commits `e86d013`, `c41da67`)
- [x] 3.3 A double literal at the top of the range is finite, scaled in two steps to twice the precision (commit `e06d46e`)
- [x] 3.4 The lexer test reads its reference doubles with `strtod` (commit `ea9c495`)
- [x] 3.5 A token's kind is its member `type`, not `k` (commit `1d07e68`)

## 4. The calculator (design 5)

- [x] 4.1 The grammar, the node, `environment_for`, `environment`, exact integral powers, `division_by_zero` and `evaluate`, decimal by default (commit `0f0bc86`)
- [x] 4.2 Squaring pinned for `double`, and the calculator says which error comes first (commit `c1b66a3`)
- [x] 4.3 Integral powers right at the edges of the range (commit `1d8fcca`)
- [x] 4.4 `calc::environment::get_allocator` (commit `a941fe1`)
- [x] 4.5 An environment's allocator needs no default constructor (commit `8987b62`)
- [x] 4.6 Allocation tests that no optimizer elides: a replaced global `operator new` and a counting allocator that calls it as a function (commit `f6af331`)

## 5. The default depth, measured (design 3, "The depth limit")

- [x] 5.1 The default depth measured on wasm's default stack, and `test/depth_test.cpp` (commit `4637b43`)
- [x] 5.2 A depth test's failure names its case, and the canary says the default must be measured again (commit `dd02706`)
- [x] 5.3 The default measured again on the smaller frames (commit `edd1e73`)
- [x] 5.4 The default rests on the shallowest shape measured: 203 calls, half 101, hence 64 (commit `3f02808`)

## 6. The proof grammars and the REPLs (design 6)

- [x] 6.1 The openui-lang micro-subset, its lazy lexer and its REPL, with `openui_repl.input` and `openui_repl_eof` (commit `63261c6`)
- [x] 6.2 The calculator's REPL and its session, ten thousand `(` included (commit `c85d3d1`)
- [x] 6.3 The REPLs skip a line of tabs, and print checks `to_chars` (commit `4385daf`)

## 7. Doc Comments and the page (design 8)

- [x] 7.1 Every public symbol has a Doc Comment, and MrDocs builds the reference strict (commit `240c1d3`)
- [x] 7.2 The reference renders whole (commit `c41da67`)
- [x] 7.3 The page with the MrDocs reference, the README and `AGENTS.md` (commit `c43798d`)
- [x] 7.4 Outputs that hold on every standard library, and listings without lint noise (commit `cf0de21`)
- [x] 7.5 An exemption encloses its functions alone, and two comments say all they do (commit `292fdf1`)
- [x] 7.6 The page says which compiler flags the same bits assume (commit `caac799`)

## 8. Exceptions and their configuration (design 1, decision 8)

- [x] 8.1 Every target builds with exceptions and the headers compile without them, which the lint checks (commit `544038d`)
- [x] 8.2 `config.hpp` names `WEBCPP_PRATT_NO_EXCEPTIONS`, defined without exceptions or by the developer, with the tests `config`, `config_developer_defined` and `config_all_off` (commit `32708c1`)
- [x] 8.3 The page says the lint compiles every test and example without exceptions too (commit `92dfcc1`)
- [x] 8.4 `config.hpp` includes `<webcpp/config.hpp>`, and `WEBCPP_NO_EXCEPTIONS` disables pratt's exceptions (commit `330c188`)

## 9. Emscripten, the changelog, CI (design 1 and 9)

- [x] 9.1 CI through the superproject's library workflow (commit `2b263af`)
- [x] 9.2 A changelog, the page's last appendix, its lines saying what a user sees, and those gathered before it existed (commits `ed66e32`, `2cb34c8`, `362bee4`)
- [x] 9.3 Emscripten: the tests and examples under node (commit `bbc45bf`)
- [x] 9.4 The test report's address at the organisation's root (commit `591a9dc`)
- [x] 9.5 The maintainer's public address in `meta/libraries.json` (commit `d78451b`)
