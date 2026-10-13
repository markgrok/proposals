# The organization: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took the organization in as a retroactive specification.
`superproject/` is the superproject's tree at `c53f3f9` (without the libraries' and the site's
contents, which are submodules), `site/` is the site's repository at `7a4c03f`,
`shared-infra-plan.md` is the plan of the shared pieces the first libraries needed,
`release-design.md` the owner's decisions of 2026-10-08 on releases, version and config, and
`site-spec.md` and `site-plan.md` the site's specification and the plan of its increments 1 to 3
(Tasks 0 to 11 are built). `../tasks.md` is the checklist.

## Where it stands

The superproject was developed from 2026-10-07 to 2026-10-13: 261 commits, all as `R`, from
`bb8533b` ("Superproject: ignore what is local or built") to `c53f3f9` ("site.jam's path.exists is
given a native path, which the previous commit left out"). The site's repository has 21 commits,
from `6c47e8a` to `7a4c03f` (2026-10-11).

| What | Where |
| --- | --- |
| The superproject | `github.com/webcpporg/webcpp`, `main`; `reference/superproject/` is its tree at `c53f3f9` |
| The libraries | nine submodules of `libs/`: config, xactor, xstate, pratt, wasi, trystero, document, idna, lexical (`lexical` at `e286ca4`, the commit its own proposal records) |
| The site | the submodule `site/` at `7a4c03f`; the preview built from `main` is served from the branch `gh-pages` of `github.com/webcpporg/webcpporg.github.io` at https://webcpporg.github.io/ |
| The release channel | https://webcpp.org/ : the domain is bought, nothing is published there |
| The version | `WEBCPP_VERSION` 100 (0.1.0), in preparation: `git tag -l` of the superproject prints nothing |

### What was verified for this proposal

- From the repository (read only), 2026-10-13: the first commit's date (2026-10-07), the commit
  counts, `.gitmodules` (nine libraries, the site, two third-party submodules), the submodule
  pointers (`git submodule status`: the site at `7a4c03f`, the JSON Schema Test Suite at `5b0ee16`,
  nativejson-benchmark at `478d572`, lexical at `e286ca4`), the absence of any tag, and that every
  commit hash cited in `../tasks.md` exists (the site's two are in the site's own repository).
- By reading `reference/`: the requirements follow `AGENTS.md` chapters 1 to 13, the Jamroot, the
  Jam files and scripts of `tools/`, `.github/workflows/`, `doc/releases/0.1.0.adoc`, and, for the
  site, the pages, components, tests and tools of `reference/site/`.

### What was recorded by others and not re-run here

- The CI of the superproject at `05705ca` (run 38356249625, recorded in lexical's status): GCC 14
  and 15, Clang 18 and 22, Apple Clang, wasm32-wasip2, wasm32-wasip3, Emscripten 6.0.11, the own
  lanes, the documentation and the site green; MSVC 14.3 and 14.5 failed before building anything
  on a `path.exists` that was not given a native path. `1e33cbc` and `c53f3f9` are that fix (and
  the lint rule `path.exists native`); no run of CI at `c53f3f9` is recorded in the sources, so
  MSVC green on the whole superproject is not established here.
- The site went live at https://webcpporg.github.io/ on 2026-10-11 (the site plan's ledger records
  the Pages source switched to `gh-pages`, the CI run 38133640906 green for Site, Publish and Pages,
  and the old address redirecting `/webcpp/libs/pratt/#pratt-limits`).

No test, lint or build was run for this proposal; the requirements describe the tree, and the
scenarios are taken from the rulebook's examples, the tools' tests and the sources' own text.

## Open items

1. **The prose of `AGENTS.md` lags the tree in places.** Its table of the libraries (chapter 1)
   says lexical is "a submodule at `libs/formats`", where `.gitmodules` and the tree say
   `libs/lexical`; fix it in the superproject.
2. **"Every repository starts from one import commit."** `AGENTS.md` chapter 11 says so for every
   repository, and the published superproject has 261 commits. Whether the rule applies to the
   superproject was not established here; the rule or the history needs a decision.
3. **MSVC.** Boost.Locale's and Boost.Cobalt's behaviour on Windows is not measured; the first CI
   run that builds them is the measurement (`AGENTS.md` chapter 2 and 13).
4. **0.1.0 is not released.** `b2 check-release` has not been run on a final tree and `doc/releases/0.1.0.adoc`
   is still "In preparation"; its New Libraries section lists config, document, idna, lexical,
   pratt, trystero, wasi, xactor and xstate.
5. **The roadmap** (chapter 13): the CI's three levels, the review of the older libraries, the
   asynchronous design with Boost.Asio and Boost.Cobalt, webcpp::asio, beast, webrtc and crypto,
   external libraries built from `third_party/`, bundled toolchains, and the site's increments 2
   to 8 (increments 4 and 5 are the change `site-increments-4-5`).

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-<slug> -b <branch> main`,
   then `git submodule update --init --recursive`), following `AGENTS.md`.
2. Commit as `R <github.overboard312@passmail.net>` with no trailer, staging explicit paths.
3. Before saying a change is done: the lint (`tools/lint/lint.sh` with wasi-sdk's clang-format and
   clang-tidy), the tests of the tools that the change touches (`python3 tools/<dir>/<name>_test.py`),
   the lanes it affects from scratch (`b2 -a ...`), `b2 -a doc`, and, for the site, `b2 -a site`
   and `b2 -a /webcpp/site//test`; then CI on every lane.
4. A change to a rule changes `AGENTS.md`, the tool that enforces it and the test of that tool in the
   same commit, and adds its line to the release notes of the version in preparation under
   "Tools and build".
5. Nothing is pushed, merged, tagged or published without the owner.
