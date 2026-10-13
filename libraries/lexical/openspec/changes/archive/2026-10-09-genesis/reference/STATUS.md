# webcpp::lexical: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took lexical in as a work-in-progress specification.
`spec.md` and `plan.md` are the specification and plan of 2026-10-09, written when the library
was named formats; `../tasks.md` is the
checklist; `source/` is the library's tree at `e286ca4`.

## Where it stands

The library was developed from 2026-10-09 to 2026-10-13 in superproject worktrees, its
repository's history 34 commits, all as `R`, from `094b228` ("the library's skeleton...") to
`e286ca4` ("formats is renamed lexical..."). On 2026-10-13 it was published:

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/lexical`, branch `main` at `e286ca4` (the repository was created empty as `webcpporg/formats` and renamed) |
| The superproject | `github.com/webcpporg/webcpp` `main` at `05705ca`, holding `libs/lexical` at `e286ca4` and `libs/idna` |
| The page | built and published by that commit's CI at `webcpporg.github.io/libs/lexical/` |

`source/` is exactly `git archive e286ca4` of the library.

### What was measured

- Locally (macOS, Apple clang 21), 2026-10-13, from scratch: `b2 -a libs/lexical/test
  libs/lexical/example`: exit 0, 40 tests `**passed**`, the four examples run and compared with
  their `.expected` files; `b2 -a libs/lexical/test//generated`: exit 0; the superproject's lint
  in 8 shards, 0 failed rules; `b2 -a libs/lexical/doc` and html-validate on its page: exit 0;
  `b2 -a site`: exit 0.
- On CI, run 38356249625 of the superproject at `05705ca`: GCC 14 and 15, Clang 18 and 22,
  Apple Clang (macOS 15), wasm32-wasip2, wasm32-wasip3, Emscripten 6.0.11, the own lanes
  `lexical, generated` and `lexical, oracle`, the documentation and the site: green.
  MSVC 14.3 and 14.5 failed before building anything (below).

## Open items of 0.1.0

1. **MSVC.** b2 stopped loading `libs/idna/test/Jamfile` with "`webcpp.generated tables:
   libs/idna/test/../tools/generate_tables.py does not exist`". The superproject's
   `tools/generated/generated.jam` gave `path.exists`, b2's builtin, a path in b2's own form
   (`/D:/a/...`), which names no file on Windows; idna and lexical are the first libraries to
   use `webcpp.generated`, so no earlier MSVC run reached it. Fixed in the superproject: every
   `path.exists` of its Jam files takes `[ path.native ... ]`, and a lint rule, `path.exists
   native`, with its test, refuses another. To be pushed, and MSVC seen green for lexical.
2. **The tools' tests.** `tools/release/release_test.py` pins the SHA-256 of a tarball made
   from files `tools/release/scratch.py` writes, which changed when the commit identity became
   `R`; the pins are updated (the local run and CI agree on the new values). To be pushed with
   item 1.
3. **`AGENTS.md`** names `max_regex_alternatives` in its bullet on the regex bounds, a bound the
   validator no longer has (`regex.hpp` holds `max_regex_length` and `max_regex_nesting`).
4. **The regex validator's stack.** The bound `max_regex_nesting` (32) was chosen so that the
   deepest pattern fits wasm32-wasip2's 64 KiB stack at `-O0`; the figure measured on
   2026-10-12 (about 20 KiB) was for the previous implementation and must be measured again.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-lexical-<slug> -b
   lib/lexical-<slug> main`, then `git submodule update --init --recursive`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/lexical` is the published
   repository; `source/` is the same tree, for reading without a checkout.
2. Close the open items above first (the change `strategies`, tasks 0.1 to 0.3).
3. Take the owner's answers to the design's open questions O1 to O6 before tasks 3 to 5; record
   each as a decision in the superproject's design record.
4. Every change follows the library's own rules (`source/AGENTS.md`): tests first, every rule
   agreeing with its function, the suite's known-failure lists never lying, the generated files
   never edited, `regex_v8` run on every target after any change to the validator.
5. Before saying a task is done: the lint, `b2 -a libs/lexical/test libs/lexical/example`, the
   page, and the site, locally; then CI on every lane.

## What json_schema needs from lexical

`openspec/changes/json-schema` uses lexical twice: its default format set calls the format
functions (the entry point of the change `strategies` when it lands), and its regex policy (its task 1.4, to
redo) needs the matching interface of the change `strategies`, group 3: compile an ECMA-262 pattern for Boost.Regex and
search with a step budget. That task 1.4 waits for that group.
