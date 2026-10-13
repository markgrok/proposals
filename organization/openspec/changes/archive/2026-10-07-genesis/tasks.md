# Tasks

Every task of the organization as built at `c53f3f9` (superproject) and `7a4c03f` (site), each
pointing to its commits in the superproject's history (`git log --reverse`, 261 commits, all by
`R`, 2026-10-07 to 2026-10-13) or to its task in `reference/shared-infra-plan.md`,
`reference/release-design.md` or `reference/site-plan.md`. A checked task is done in
`reference/superproject/` or `reference/site/`. What remains is on the roadmap (design.md,
Non-Goals) and in the change `site-increments-4-5`.

## 1. The build foundation (2026-10-07)

- [x] 1.1 The superproject's ignore file, the Jamroot that finds the installed Boost 1.92 and refuses an older one, clang's search order and the check keyed on the version (`bb8533b`, `07327d2`, `fa95146`)
- [x] 1.2 `tools/webcpp.jam`: each library declares its targets, a program is built only for those, and `webcpp.compile` (`d495a09`, `80e7708`)
- [x] 1.3 Boost's version read with `findstr` on Windows, and a version it cannot read stops the build (`78721f0`)
- [x] 1.4 `webcpp.boost-test` and `/webcpp//boost_json` (`2cffbe4`, `29fa0e8`, `8b70ab6`)
- [x] 1.5 A test program sees C++20, and a build without exceptions sees no RTTI; later every target builds with exceptions (`599e87c`, `961fe43`, `5512142`, `51b8ca6`)

## 2. The lint

- [x] 2.1 The shared lint with Pyright over every Python file, and the analysis of run-fail sources (`5fda274`, `a04d38a`)
- [x] 2.2 Doc Comment commands, the `@ref` colon and possessive rules, clang-tidy reporting the headers of tests and examples (`f51007d`, `a9682f5`, `202c73d`)
- [x] 2.3 Include boundaries (`b327445`, `0d3d95d`)
- [x] 2.4 The Jam comment width and the Python blank lines (`3186aa7`)
- [x] 2.5 The lint analyses every program and aggregate without exceptions too, the bare throw rule and the twin (`5fcf499`, `feb00d6`, `9a40c40`)
- [x] 2.6 `returns *this`, `lint-std` and the prefix operators (`569ba75`, `d76dd45`, `130a10c`, `6781768`)
- [x] 2.7 Doc Comment code spans with `__X__` and the world rule's names (`91ccb62`)
- [x] 2.8 The compilation database holds what every own lane builds (`42187dc`) and headers that build only on emscripten or against a native dependency (`41b7a1c`)
- [x] 2.9 `path.exists` given native paths, and a rule that refuses another (`1e33cbc`)

## 3. The report and the lanes

- [x] 3.1 The test matrix from b2's `--out-xml`, a lane named after its target or toolset directory, a phone reading every verdict (`96a38c3`, `5089bd7`, `01a4538`, `4dc16c4`)
- [x] 3.2 Own lanes on the targets they name, reaching the report, and every served program in a lane (`3d50575`, `e0f092a`, `bf725e3`)
- [x] 3.3 Each own lane's kind and the CI installing Node only where needed (`d91f6b7`, `7ba58f0`)
- [x] 3.4 `--shard K/N` for the tools' tests and the lint in eight shards each (`242adac`, `c945be2`)

## 4. The documentation toolchain

- [x] 4.1 One documentation toolchain and the reference from MrDocs, strict (`6e34540`, `46a3759`, `c1218f4`, `63074ab`)
- [x] 4.2 The page's style: listings at 100 columns, tables on a phone, identifiers breaking between parts, tables scrolling in a box (`5fbc2cf`, `d6a6903`, `533917a`, `8b262a1`)
- [x] 4.3 The index links each page in the tree, or where the site serves it with `-sWEBCPP_INDEX=site`, and takes each page from the build (`214d191`, `948e02d`)
- [x] 4.4 Twins and computed counts on a page, links between library pages checked (`c12c010`, `099cc30`, `3ee2e67`)
- [x] 4.5 Pages include the superproject's and a library's tracked files by tag (`fe5de6f`, `c539348`, `84fef92`, `7755b04`)
- [x] 4.6 The rendered page's checks: no em dash, no link inside inline code, no `<wbr>`, a name breaking at a zero-width space, thousands separators (`8158917`, `7692933`, `0107b1a`, `55ea3d7`)
- [x] 4.7 A reference for a header built for emscripten alone, read for Emscripten's target (`e526cd1`)
- [x] 4.8 One installer of Node packages, locked and pruned, for the documentation, the oracles and the lint (`ef125d5`, `7099917`, `575efb9`)

## 5. The rulebook

- [x] 5.1 AGENTS.md as the porting rulebook, README.md and the index page (`4c094fa`)
- [x] 5.2 The oracle and a library's own lanes in the rulebook; WebAssembly components; the published state with a roadmap (`995d064`, `bdcc44f`, `06cca7b`)
- [x] 5.3 CONTRIBUTING.md and the rules for external contributors, allocators, asynchronous code (`2178b31`)
- [x] 5.4 What a library builds on and in what order, why webcpp and for whom, each target's purpose (`178bfc6`, `8cbf1b7`)
- [x] 5.5 Bringing a C library into webcpp (`3b879cf`)
- [x] 5.6 Every library's `config.hpp` includes `<webcpp/config.hpp>` first (`a369f7a`)

## 6. The CI

- [x] 6.1 One reusable workflow per library, the superproject's lanes, the report and Pages (`ee70d71`, `8b82ce6`, `1eb5ff7`)
- [x] 6.2 The Boost action: the gzip archive, the standard layout, a checked prefix (`3f4ff36`, `a787c65`, `34dd76b`)
- [x] 6.3 One own-lane job per library, lane and target (`749566b`, `390a5f9`)
- [x] 6.4 The wit-bindgen and WASI WIT actions (`66856a0`)
- [x] 6.5 The emscripten lane and the emsdk action, with the commit pinned and the cache warmed (`087bc86`, `4b6595b`)
- [x] 6.6 A pinned chrome-headless-shell, the container image from a snapshot built once, external libraries only where needed (`6cedb62`, `fc2e9db`)
- [x] 6.7 The actions' shared functions in `helpers.sh`, and the container lane's exports (`bf67928`, `bc8526a`)
- [x] 6.8 The container image's apt retries a snapshot outage (`f3d84ce`)
- [x] 6.9 The tests of the tools pass on the CI's Linux runner (`45a79e3`)

## 7. The oracle, components and drivers

- [x] 7.1 The twin runner and the tree comparison, the rules of a library's oracle, its stamp and `update-expected` (`5fac33e`, `d6a6903`, `741c5a9`)
- [x] 7.2 WIT bindings generated by wit-bindgen for a WASI target alone, `webcpp.serve` and `webcpp.serve-script` (`d49f681`, `ebb19cc`, `199a3b7`)
- [x] 7.3 `webcpp.compile-diagnostic` and transcripts a shell runs as written (`117a8cc`)
- [x] 7.4 Emscripten tests under node, link-only programs and driven tests in own lanes, and a driven test bounded and leaving no process (`382f65f`, `b7e3ad2`)
- [x] 7.5 WIT bindings generated safely by concurrent loads (`2e5044e`)

## 8. Libraries webcpp does not build

- [x] 8.1 `webcpp.external`, `webcpp.external-found` and `webcpp.left-out`, from b2's command line or `.local/`, left out unless required, a relative `-s` read from b2's working directory, and the index naming a library left out (`e2a937d`, `5bcc51b`)

## 9. Shared infrastructure (reference/shared-infra-plan.md)

- [x] 9.1 `third_party/` and the JSON Schema Test Suite (plan: Task 1; `ddd3fb2`, `018e54d`)
- [x] 9.2 `webcpp.generated`, the check of generated files (plan: Task 2; `ca4a925`, `16f0d0b`)
- [x] 9.3 `/webcpp//boost_url`, built from Boost's sources (plan: Task 3; `6a4e2a2`, `6913812`, `ca280b4`)
- [x] 9.4 `/webcpp//boost_charconv` and `/webcpp//boost_locale` (plan: Task 3B; `be474ad`, `b279258`, `2e994f3`)
- [x] 9.5 `webcpp.data`, test data on WASI (plan: Task 4; `8955e5f`, `e57f88a`)
- [x] 9.6 `nativejson-benchmark` (`1aaa098`) and Boost.Cobalt built from Boost's sources (`901b933`) and Boost's sources found on Windows (`3ec3d16`)
- [x] 9.7 Plan Tasks 5 to 7 (the SRELL mirror and the upstream drafts) are superseded: the owner abandoned SRELL and webcpp::regex on 2026-10-12, and the rules left with them (`34ecf7e`)

## 10. Releases, version and config (reference/release-design.md)

- [x] 10.1 Each version's release notes on the index page, with the notes of 0.1.0 (`c6082c7`)
- [x] 10.2 The changelog's form, which the lint checks on every run (`2a9a606`, `f0340bb`)
- [x] 10.3 The version `libs/config` writes, read by the Jamroot and checked (`a0064af`, `45471f0`)
- [x] 10.4 `check-release` (`071c6f8`, `336d91c`)
- [x] 10.5 `whats-new` (`1811984` and following)
- [x] 10.6 `dist`, a reproducible archive (`3a4130f`, `d52f4c9`, `11b9949`)
- [x] 10.7 The config library, the changelog rule and the release tooling in the rulebook (`19d1b8f`, `1d35672`)
- [x] 10.8 Each library's changelog, and `libs/config` as a submodule (`f11986d`, `b4dccc2`)
- [x] 10.9 The libraries move onto `<webcpp/config.hpp>` (`ee70f95`, `cecd436`)

## 11. The libraries join (submodules)

- [x] 11.1 xactor, xstate, pratt, wasi, trystero, config, document, idna and lexical as submodules (`d8aa7c8`, `63d2183`, `cffd6a6`, `e8a8b32`, `bf97897`, `f11986d`, `25e332b`, `dac26e2`, `05705ca`)

## 12. The site (reference/site-spec.md, reference/site-plan.md Tasks 0 to 11)

- [x] 12.1 `tools/site/catalogue.py`, `libraries.json` from every library's meta and declared targets (plan: Task 3; `1531d6a`, `76dce6a`)
- [x] 12.2 The site's project in the Jamroot, and the Node installer for any directory (`79d7f91`, `f0fec44`)
- [x] 12.3 `b2 site` and `b2 site-serve`: the whole site into `bin/site/`, the npm scripts through `site.npm` (plan: Task 5; `571141f`)
- [x] 12.4 The site's tests as a b2 target and a CI lane (plan: Task 6; `2c3f71b`)
- [x] 12.5 `valid-html` in two passes (`1a091d3`)
- [x] 12.6 CI builds the site and publishes it to `gh-pages`; the old address redirects (plan: Task 9; `9bc9496`, `0794ecc`)
- [x] 12.7 `b2 site` swaps `bin/site` as a link in one rename (`e8d482e`)
- [x] 12.8 The report's page headed with the site's header (`c8be9c4`, `5bc046f`, `28d2535`)
- [x] 12.9 The site's repository as a submodule (plan: Task 10; `f2e7cab`)
- [x] 12.10 The preview is not indexed (`01132bd`)
- [x] 12.11 The site's addresses at the root, and the rulebook (plan: Task 11; `5d17b54`, `f4654a5`)
- [x] 12.12 In the site repository, the project, the components and the renderer, the pages, the cards, the swap script, the preview's noindex, and the bundles replaced whole (`6c47e8a` to `7a4c03f`, 21 commits)
