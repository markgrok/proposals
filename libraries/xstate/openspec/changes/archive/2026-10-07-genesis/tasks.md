# Tasks

Every task of 0.1.0, each pointing to its commits in the library's repository (27 commits, all as
`R`, from `631e737` on 2026-10-07 to `5299db2` on 2026-10-12) and, where there is one, to its task
in the plan of the move into webcpp (written when the library was moved, with Tasks 0 to 14; it
is not in `reference/`). A checked task is done in `reference/source/`. The library's history
before the move is in an earlier private project and is not part of this proposal.

## 1. The move: headers, build, examples, metadata (plan: Tasks 0 and 6)

- [x] 1.1 Pre-flight: the repository, the worktree, the licences and the counts the move had to
  keep (the example programs, the twins, the fixtures) checked (plan: Task 0; no commit in the
  library)
- [x] 1.2 The headers, `build.jam`, the 146 examples with their `.expected` files, `meta/`,
  `README.md`, `AGENTS.md`, `LICENSE_1_0.txt` and `LICENSE-XSTATE.txt`, renamed to
  `webcpp::xstate` (`<webcpp/xstate...>`, `WEBCPP_XSTATE_`, the category `webcpp.xstate`) and under
  webcpp's licence notice (`631e737`, 2026-10-07)
- [x] 1.3 The agents' notes wrapped and the examples listed in byte order in `example/Jamfile`
  (`5646ce3`)

## 2. The oracle (plan: Task 7)

- [x] 2.1 XState 5.33.2 pinned in `test/oracle/package.json` and `package-lock.json`; `oracle.mjs`,
  `actors.mjs`, `serialize.mjs`, `vocabulary.mjs` and `development.mjs`; the 136 twins under
  `test/oracle/twins/` and `without-twin.txt`; the fixtures (38 machine and 31 actor case files,
  each with XState's expected results); `test/oracle/Jamfile` on the superproject's
  `webcpp.original`, `webcpp.twins`, `webcpp.cases` and `webcpp.lane` (`e82950b`)
- [x] 2.2 The oracle's `node_modules` ignored as the link the shared installer makes (`506cc39`)

## 3. The tests (plan: Tasks 8 and 9)

- [x] 3.1 The core's tests on lightweight_test (`values`, `machine`, `cursor`), the cases on
  `webcpp.boost-test`, `core_alone`, `user_aggregates` and the headers alone (`f3d64c2`)
- [x] 3.2 The actor layer's 73 cases on lightweight_test, split into five programs by
  responsibility, and the actor cases on `webcpp.boost-test` (`676c81d`)
- [x] 3.3 A cursor helper names its row, and the tests' tidy notes say what they do (`967bc82`)
- [x] 3.4 The tests that run without exceptions on wasip2 only, and the actor cases' objects big on
  MSVC (`ec7053b`); later, every target built with exceptions and the headers compiled without
  them by the lint (`ceef5a9`, `9dcb4d4`)
- [x] 3.5 `test/CONVERSION.md`: the conversion record, with the fixture rename named, three test
  notes stating their guarantee, and the tests that replace the old ones named (`92eea1a`,
  `64da946`, `9a30164`)

## 4. Doc Comments and the reference (plan: Tasks 10 and 11)

- [x] 4.1 Every public symbol of the machine core has a Doc Comment, and MrDocs builds the core's
  reference (`e76605b`, `0315b0e`)
- [x] 4.2 Every public symbol of the actor layer has a Doc Comment, and MrDocs builds its
  reference strict (`a1e5585`, `b8cdaa9`)

## 5. The page (plan: Task 12)

- [x] 5.1 The documentation page, with the MrDocs reference and links to xactor's: introduction,
  state machines, actors, testing, guarantees, appendices A to D (`e680096`)
- [x] 5.2 The page counts what it counts, from `doc/counts.py`, and its links open without a stray
  space (`2ac2d04`)
- [x] 5.3 The testing chapter names the oracle's job as the continuous integration names it
  (`d7c4e58`)
- [x] 5.4 The changelog, the page's last appendix, with the lines saying what a user sees
  (`acb57b7`, `bb89d91`)

## 6. CI (plan: Task 13)

- [x] 6.1 CI through the superproject's library workflow (`235c44c`)
- [x] 6.2 The test report's address at the organisation's root (`3ce0a97`)

## 7. The configuration and exceptions

- [x] 7.1 `config.hpp` names `WEBCPP_XSTATE_NO_EXCEPTIONS`, defined without exceptions or by the
  developer, with the three `config` tests (`c57afce`)
- [x] 7.2 `config.hpp` includes `<webcpp/config.hpp>`, and `WEBCPP_NO_EXCEPTIONS` disables xstate's
  exceptions with every webcpp library's (`57540f3`)

## 8. Publication (plan: Task 14)

- [x] 8.1 The maintainer's public address in `meta/libraries.json` (`5299db2`, 2026-10-12)
- [x] 8.2 The library is the repository `webcpporg/xstate` and the submodule `libs/xstate` of the
  superproject, in release 0.1.0 (`reference/STATUS.md`)
