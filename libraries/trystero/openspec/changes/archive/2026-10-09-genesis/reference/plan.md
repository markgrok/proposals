# webcpp stage 4: trystero Implementation Plan (revised)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move trystero, serverless WebRTC rooms wire-compatible with
Trystero 0.26, into webcpp as `webcpporg/trystero`: `webcpp::trystero` under
`<webcpp/trystero/...>`, built by the superproject's b2 without CMake, born
with the allocation rule, tested natively and on emscripten, documented on the
shared doc toolchain with a MrDocs reference, its lanes green. On the way,
apply the owner's exceptions decision to the superproject and every published
library, make emscripten a tested target of the superproject and its CI, and
let pratt declare emscripten.

**Architecture:**
- **Exceptions on every target** (owner decision 2026-10-08): the wasip2 flags
  become wasip3's, under one Jamroot region `wasi-exceptions`. "Works without
  exceptions" stops being a lane and becomes what the lint's no-exceptions
  analysis checks, for every aggregate translation unit of every library.
  xactor, xstate, pratt and wasi change their texts in their own repositories.
- **The superproject learns emscripten as a tested target, generically:**
  - the Jamroot's mapping (wasm32, single-threaded, static); programs that run
    under node link `-sNODERAWFS=1`; b2's emscripten toolset runs them with
    node itself, so no `testing.launcher` is ever given;
  - `webcpp.link`, a program built and linked, never run;
  - `webcpp.drive`, a test of b2's that builds a program and runs a script of
    the original's language over it, lane-only like a served program;
  - in CI: `tools/ci/emsdk.jam`, the `emsdk` action, an emscripten lane in
    `matrix.py`, and emscripten accepted as an own-lane platform;
  - a lint and a reference that read headers that compile only for
    emscripten or only against a native dependency.
- **trystero is a Boost-style library** with a `build.jam` and no Jamroot:
  `/webcpp/trystero//trystero` (the core and, on emscripten, the browser
  backend) and `/webcpp/trystero//native` (the native backend over
  libdatachannel, Boost.Beast and OpenSSL). Its external dependencies stay as
  they are (spec section 3), found through `SECP256K1_ROOT`,
  `SECP256K1_EMSCRIPTEN_ROOT`, `LIBDATACHANNEL_ROOT` and `OPENSSL_ROOT`, until
  webcpp builds them from `third_party/` (AGENTS.md chapter 13).
- **Its evidence is the shared oracle:** `libs/trystero/test/oracle/` holds the
  pinned Trystero JS, `oracle.mjs` under `webcpp.cases`, and the interop tests
  under `webcpp.drive`, in the own lanes `oracle` and `interop`.
- **Allocation:** a `std::pmr::memory_resource` given in `room_config` reaches
  every allocation of a room, Boost.JSON's through a header-only adapter to its
  `storage_ptr`; a counting test proves it.

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (Homebrew): Asio, Beast, JSON (through
  `/webcpp//boost_json`), Hash2, System, Container (its pmr base class only);
- Emscripten 6.0.11 (emsdk, commit `a0014542110d6078c3a1a7941fa1ddb3a2281f16`),
  Node 26.7.0; wasi-sdk 34, wasmtime 47.0.3; libsecp256k1 0.8.0 (`6e2c8bc`),
  libdatachannel 0.24.6 (`6b1e2e6`), OpenSSL 3;
- Boost.Core lightweight_test; Trystero JS 0.26.0 pinned by
  `test/oracle/package-lock.json`; headless Chrome for the interop tests;
- MrDocs 2026.9.29; Asciidoctor.js; GitHub Actions and Pages.

**Spec:** `~/.buzz/PLANS/WEBCPP_DESIGN.md` (approved
2026-10-07), sections 1, 3 (trystero), 4, 5, 6, 7 and 9 (stage 4), and its
"Owner decisions after approval", which bind: allocation, own lanes on a
target, exceptions on wasip2, trystero and allocators. This plan replaces
`~/.buzz/PLANS/WEBCPP_STAGE4_PLAN.md` and adopts every proposed ruling of the
pre-flight `~/dev/webcpp/.superpowers/sdd/WEBCPP_STAGE4_PLAN/preflight.md`
(C1-C36, facts F1-F32), cited below by number, except where noted.

**The source** (read-only): SOURCE is the trystero repository at `main`
`d3f4f9a`, and WORKSPACE the directory of its dependencies' local builds. The
original plan gives both paths (its paragraph "The source"); they hold the
banned word, so this plan, every file of webcpp and every commit leave them
unspelled, and a shell sets `SOURCE` and `WORKSPACE` from there. SOURCE has
`include/trystero/` (25 headers: core, `protocol/`, `detail/`, `native/`,
`browser/`), six Boost.Test files under `test/unit/` (32 cases),
`test/oracle/oracle.mjs`, `test/interop/`, three examples and a page under
`doc/trystero/`. Every one of 58 of its 71 tracked files outside `spike/`
carries the banned word, each in its copyright line (F24). Dropped at the
move: `CMakeLists.txt`, `spike/`, `.obsidian/`, `.cache/`, `index.html`,
`example/chat/site/`, and `doc/Jamfile`'s PDF and Boost release targets.

## Global Constraints

**Repositories and paths:**
- `~/dev/webcpp` is the superproject, base `4f70819` (stage 3 complete).
  `~/dev/webcpp/libs/trystero` is trystero's own git repository; its remote
  `git@github.com:webcpporg/trystero.git` exists, public and empty, and is
  checked, never created (C25).
- SOURCE and WORKSPACE are read-only: nothing is written in them, and no build
  runs in them. Emscripten's cache is `EM_CACHE`, an absolute directory under
  `~/dev/webcpp/.local/` (or a tools test's scratch), never the emsdk's own
  `upstream/emscripten/cache` (C9).
- `.local/` (git-ignored) holds from Task 0: `emsdk` (a link to WORKSPACE's
  emsdk), `emscripten/em++` (a wrapper that sets `EM_CACHE`),
  `emscripten-cache/`, `secp256k1-native/`, `secp256k1-emscripten/`,
  `libdatachannel/` and `env.sh`, which exports the four dependency variables
  and `EM_CACHE`.
- xactor, xstate, pratt and wasi change in this stage (Tasks 1, 4 and 13), each
  with commits in its own repository; the superproject records each with a
  submodule bump (`git add libs/<name>`, never `git add libs`). Everything is
  published in Task 14.

**Commits and text:**
- Commits are made with `git -c user.name=R -c
  user.email=github.overboard312@passmail.net commit -- <paths>`, plain subject, no trailer
  of any kind. A body paragraph never starts with `word:` (`std::`).
- Everything is written in English. Never the em dash (U+2014).
- No file, name or commit holds the banned word, in any case. The lint's
  banned-word rule checks every repository, what git would track included, so
  it also reads a new repository before it tracks anything.
- "stage N" and "Task N" never appear in a tracked file or a commit message:
  what is still to come is named in AGENTS.md chapter 13's words ("until
  webcpp builds them from `third_party/`").

**Licence notice.** Every new or moved source file (`.hpp .cpp .py .mjs .js
.sh .jam .yml .html .adoc`, `Jamroot`, `Jamfile`, `build.jam`) opens with the
WebCpp.org notice, `//` for C++, JavaScript and AsciiDoc, `#` for Jam,
Python, shell and YAML, `<!-- -->` for HTML; a `#!` line stays first. From
Task 4 the lint enforces it on `.js`, `.html` and `.adoc` too (C22).

**Licence and attribution:**
- BSL-1.0 only: `LICENSE_1_0.txt`.
- `LICENSE-TRYSTERO.txt` is Trystero's `LICENSE` at its tag `0.26.0`,
  unchanged: `gh api 'repos/dmotz/trystero/contents/LICENSE?ref=0.26.0'`,
  SHA-256 `bbf91fd979faac0def9551c570e2e9f92b4e02d22f38ca5d98860b6284e1ea25`,
  byte-identical to the pinned package's `LICENSE` (C25). The README names the
  derived parts: the wire protocol, its constants, message layouts and default
  relays.

**Names:**
- the namespace `trystero` becomes `webcpp::trystero`, with its nested
  `protocol`, `detail`, `native`, `browser` and `test`; `<trystero/...>`
  becomes `<webcpp/trystero/...>`, and `<trystero/trystero.hpp>` becomes
  `<webcpp/trystero.hpp>`;
- include guards and macros `WEBCPP_TRYSTERO_*`; the error category
  `webcpp.trystero`;
- the examples' app ids are `webcpp-trystero-quick-start` and
  `webcpp-trystero-chat` (C30);
- **what Trystero JS or a page sees never changes:** the wire strings
  (`"Trystero@"`, `"__trystero_pw"`, every protocol constant), the names
  embind exports (`trystero_dispatch`, `join`, `send`, `start`, `leave`), and
  the oracle's inputs. Renames use exact C++ patterns, never a replacement of
  the word `trystero`.

**Running b2:**
- `B2` is `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2
  --user-config=.local/user-config.jam ...`, from `~/dev/webcpp`, in a shell
  that has run `source .local/env.sh`. A verdict is built from scratch
  (`B2 -a`).
- An action of more than one command opens with `set -e`; `$(` inside actions
  is b2's (backquotes); a `<build>no` is conditioned only on `<toolset>` and
  its version.
- **Parallel lanes:** each concurrent b2 run has its own `--build-dir`
  (`bin/lane-clang`, `bin/lane-gcc-15`, `bin/lane-wasip2`, `bin/lane-wasip3`,
  `bin/lane-emscripten`, `bin/lane-container`); start them in the background,
  wait for all, read each log; at most one doc build at a time; every lane
  from scratch.
- On emscripten, never `testing.launcher=...`: b2's emscripten toolset runs a
  test's `.js` with node itself, and a launcher doubles it (`node "node"
  x.js`, F3); from Task 2 webcpp refuses it by name (C5).

**Targets:**
- trystero declares `native emscripten`. A test of the protocol or of a room
  over the memory relays runs on both; a test of the native backend runs
  natively only; a program that needs public relays or a browser is a
  `webcpp.link`; the interop tests are `webcpp.drive` tests in the own lane
  `interop`, on native and emscripten (C10).
- `emscripten` is b2's `emscripten` toolset, registered from the region
  `emscripten` of `tools/ci/emsdk.jam` (`using emscripten : :
  <emsdk>/upstream/emscripten/em++ ;`, C33), wasm32, single-threaded, static;
  every program that runs under node (`webcpp.run`, `run-fail`, `example`)
  links `-sNODERAWFS=1`, and no other does (C6, C34).

**Exceptions and RTTI** (provisional, owner to confirm (C3)):
- Whether a program uses exceptions is its developer's choice on every target
  (owner decision 2026-10-08). webcpp builds every target with exceptions on:
  native as the compiler does; wasip2 and wasip3 with `-fwasm-exceptions
  -mllvm -wasm-use-legacy-eh=false` and, on the link, `-fwasm-exceptions
  -lunwind`; emscripten as b2's toolset does, `-fwasm-exceptions` in the
  legacy encoding (C4). No variant without exceptions and none without RTTI is
  built. A user's `exception-handling=off` gets `-fno-exceptions` and the
  handler `tools/throw_exception.cpp`, on every target.
- **The rule of this plan:** every library keeps compiling without
  exceptions, which the lint checks by analysing every aggregate translation
  unit a second time with `exception-handling=off`. The one stated exception
  is trystero's native backend, because libdatachannel reports errors by
  throwing. It is declared in one place, the usage requirement
  `<exception-handling>on` of `/webcpp/trystero//native` in
  `libs/trystero/build.jam`, with that reason in its comment; the lint leaves
  the headers whose units carry it out of the no-exceptions analysis, and each
  of those headers opens with `#ifdef BOOST_NO_EXCEPTIONS` / `#error
  "<header> needs exceptions: libdatachannel reports errors by throwing"`.
  Changing the rule later means changing that one line, its comment, the
  `#error` guards, and AGENTS.md chapter 6's paragraph that Task 1 writes.
- wasi's behaviour does not change in this plan: its handler still catches
  nothing, and an exception that escapes the main traps the instance on wasip2
  as on wasip3. Only its texts follow the decision (C2). Whether wasi should
  catch such an exception stays an open question for the owner.

**Allocation:** trystero is born with the allocation rule, through `std::pmr`,
header-only: `room_config::memory`, a `std::pmr::memory_resource*` (default
`std::pmr::get_default_resource()`), reaches every allocation of a room;
Boost.JSON allocates through `webcpp::trystero::json_resource`, a header-only
adapter to `boost::container::pmr::memory_resource`, never through
`boost::container::pmr`'s compiled global resources (F30); the payload types
gain pmr forms; what libdatachannel, OpenSSL and the browser allocate is
stated on the page; a counting test proves it natively and on emscripten
(C23). The allocators stage may adjust the mechanism later.

**Tests and documentation:** lightweight_test; no test reaches beyond the
loopback, the interop peers' ICE included (C27); trystero's oracle uses the
shared `tools/oracle` (`webcpp.original`, `webcpp.cases`, `update-expected`);
trystero has no twins, since no example of it runs (all are link-only, C12,
C29); Doc Comments complete (a one-sentence brief, `@param`, `@tparam`,
`@return`, each data member and enumerator its own comment, a deleted
function no `@return`, a `detail` symbol a brief, a `@see` title naming a
section of the page, no forbidden command); `doc/mrdocs.yml` presentation keys
only; MrDocs strict; a reference needs its page from its first build (C16);
the page is accepted at every width from 320 to 1440 px; each repository has
`.gitignore` (`doc/html/`, `node_modules`, `.node-modules/`) and
`.gitattributes` (C13). Apostrophes are written straight: the build curls them
(C18).

**Outward actions:** subagents never push, merge, create repositories, cancel
or start CI runs, or change GitHub settings (Task 14 is the controller's), and
never touch SSH keys or credentials.

## Review Focus

1. **A rename that reaches the wire.** A string literal or an embind name that
   holds the word `trystero` changes with the namespace, and C++ peers stop
   meeting Trystero JS peers, or the browser page stops finding its functions.
   Only C++ names change; the oracle's expected result stays byte-identical,
   and the interop lane fails on a planted wire change. Pinned in Tasks 5
   and 7.
2. **An emscripten lane that reads green while it ran nothing, ran the wrong
   program, or lost a failure.** A test built for the browser
   (`-sENVIRONMENT=web`) cannot run under node; a build that inherits the
   host's address model becomes memory64; a test that reads its input file,
   or an example its `.input`, finds no file system; a launcher doubles node;
   a failing `main` must still turn the cell red. Pinned in Tasks 2, 6 and 13.
3. **Exceptions turned on, and "without exceptions" silently unchecked.** A
   wasip2 program that throws and catches must run under wasmtime 47 and serve
   as a component; a user's `exception-handling=off` must still build with
   `-fno-exceptions` alone and link the handler; and a header that throws
   outside the declared native backend must fail the lint's no-exceptions
   analysis on every target. Pinned in Tasks 1 and 4.
4. **An allocation that escapes the user's resource.** A posted handler with
   Asio's default allocator, a `std::function`, a `make_shared`, a JSON parse
   with default storage, or a decoded payload reaches the global `operator
   new`. A counting resource and a replaced `operator new` show it, natively
   and on emscripten. Pinned in Task 8.
5. **A test that needs the network, or flakes.** A unit test that reaches a
   public relay or a public STUN server, an interop peer that keeps Trystero's
   default ICE servers, or two interop lanes that share Chrome's debugging
   port. Unit tests use the memory relays or the loopback; the interop peers
   get loopback ICE and Chrome a port the system chooses; the container lane
   runs with no network. Pinned in Tasks 6 and 7.

---

## File structure

```
~/dev/webcpp/                          superproject
  Jamroot                              region wasi-exceptions; region emscripten-target; emscripten-headers
  tools/webcpp.jam                     + webcpp.link, webcpp.drive; lane-only programs; NODERAWFS; no launcher
  tools/drive/drive.jam                the driven test's type and action, loaded by webcpp.jam
  tools/example/run_example.py         (unchanged; run-and-compare passes it node on emscripten)
  tools/lint/compile_commands.py       per-target aggregates; em++ mapped; a twin per aggregate
  tools/lint/lint.sh, rules.py         io_context alias and lint-run:; entropy; notices on .js .html .adoc
  tools/doc/reference.py, doc.jam      the reference's emscripten headers
  tools/doc/counts.py, doc-check.py    kinds link and drive; --linked; {library-root}
  tools/report/record_samples.py, samples/   wasip2 re-recorded; emscripten samples
  tools/test/emscripten_test.py        the emscripten target's tests
  tools/test/harness.py                link_emsdk; a scratch EM_CACHE
  tools/test/fixtures/browser_demo/    a fixture library declaring native and emscripten
  tools/ci/emsdk.jam                   regions emsdk and emscripten
  tools/ci/matrix.py                   the emscripten lane; emscripten own lanes; external dependencies
  tools/ci/actions/{emsdk,secp256k1,libdatachannel,openssl}/{action.yml,install.sh}
  AGENTS.md, README.md, .github/workflows/library.yml
libs/xactor, libs/xstate, libs/pratt, libs/wasi    texts of the exceptions decision; pratt + emscripten
libs/trystero/                         library (repo webcpporg/trystero)
  build.jam  README.md  AGENTS.md  LICENSE_1_0.txt  LICENSE-TRYSTERO.txt  .gitignore  .gitattributes
  meta/libraries.json  meta/include-boundaries.json
  include/webcpp/trystero.hpp  include/webcpp/trystero/{,protocol/,detail/,native/,browser/}*.hpp
  include/webcpp/trystero/json_resource.hpp  include/webcpp/trystero/handler.hpp
  example/Jamfile  example/native_quick_start.cpp  example/quick_start.cpp
  example/chat/Jamfile  example/chat/chat.cpp  example/chat/index.html
  test/Jamfile  test/*_test.cpp  test/require.hpp  test/support/memory_backend.hpp  test/CONVERSION.md
  test/fixtures/cases/protocol.json  test/fixtures/expected/expected.json
  test/oracle/{Jamfile,package.json,package-lock.json,oracle.mjs}
  test/oracle/interop/{peer.cpp,native_peer.cpp,run.mjs,relay.mjs,cpp.html,js.html}
  doc/Jamfile  doc/trystero.adoc  doc/<sections>.adoc  doc/mrdocs.yml
  .github/workflows/ci.yml
```

---

### Task 0: Machine-local dependencies (controller)

**Files:**
- Create: `~/dev/webcpp/.local/emsdk` (a symbolic link),
  `.local/emscripten/em++`, `.local/emscripten-cache/`,
  `.local/secp256k1-native/`, `.local/secp256k1-emscripten/`,
  `.local/libdatachannel/`, `.local/env.sh`,
  `~/.buzz/WORK_LOGS/WEBCPP_STAGE4_LOG.md` (the ledger)
- Modify: `.local/user-config.jam`

- [ ] **Step 1: Confirm stage 3 is complete.** `tail -5
  ~/.buzz/WORK_LOGS/WEBCPP_STAGE3_LOG.md` ends with "Final: stage 3
  complete", and `git -C ~/dev/webcpp rev-parse --short HEAD` prints
  `4f70819`. Create the ledger with the knowledge-file frontmatter (title
  `"webcpp stage 4: trystero ledger"`, status active) and record the base.
- [ ] **Step 2: Emscripten, read-only.** In a shell with `WORKSPACE` set:

```bash
cd ~/dev/webcpp
ln -s "$WORKSPACE/emsdk" .local/emsdk
mkdir -p .local/emscripten .local/emscripten-cache
cat > .local/emscripten/em++ <<'EOF'
#!/bin/sh
# Emscripten writes its cache where EM_CACHE names, else inside the emsdk,
# which is read-only here.
EM_CACHE="${EM_CACHE:-~/dev/webcpp/.local/emscripten-cache}"
export EM_CACHE
exec ~/dev/webcpp/.local/emsdk/upstream/emscripten/em++ "$@"
EOF
chmod +x .local/emscripten/em++
```

  Prove that no em++ run writes into the emsdk, with a sandbox that refuses
  every write there:

```bash
deny="(version 1)(allow default)(deny file-write* (subpath \"$(cd .local/emsdk && pwd -P)\"))"
scratch=$(mktemp -d)
printf '#include <cstdio>\nint main() { try { throw 1; } catch (int) { std::puts("caught"); } }\n' > "$scratch/t.cpp"
sandbox-exec -p "$deny" .local/emscripten/em++ --version | head -1
sandbox-exec -p "$deny" .local/emscripten/em++ -fwasm-exceptions "$scratch/t.cpp" -o "$scratch/t.js"
node "$scratch/t.js"
```

  Expected: `em++ ... 6.0.11 (a0014542110d6078c3a1a7941fa1ddb3a2281f16)`,
  the link exits 0, node prints `caught`, and `.local/emscripten-cache/` now
  holds `sysroot/`. If the sandbox refuses a write, em++ fails naming the
  path: stop, record it, and copy the emsdk into `.local/emsdk` instead
  (reading WORKSPACE only); the ledger records which.
- [ ] **Step 3: Register the toolset.** Add to `.local/user-config.jam`:
  `using emscripten : : $(root)/emscripten/em++ : <archiver>$(root)/emsdk/upstream/emscripten/emar ;`.
  `B2 -d0 declared-targets` still prints the four libraries' lines.
- [ ] **Step 4: The native dependencies,** copied from WORKSPACE's installed
  trees with `cp -R` (never built in place): `secp256k1-native/`,
  `secp256k1-emscripten/` (libsecp256k1 0.8.0, static, schnorrsig and
  extrakeys on), `libdatachannel/` (0.24.6, shared, `NO_WEBSOCKET`,
  `NO_EXAMPLES`, `NO_TESTS` on, `NO_MEDIA` off, F6). Each has `include/` and
  `lib/`. OpenSSL is Homebrew's `openssl@3`.
- [ ] **Step 5: `.local/env.sh`:**

```bash
export SECP256K1_ROOT=~/dev/webcpp/.local/secp256k1-native
export SECP256K1_EMSCRIPTEN_ROOT=~/dev/webcpp/.local/secp256k1-emscripten
export LIBDATACHANNEL_ROOT=~/dev/webcpp/.local/libdatachannel
export OPENSSL_ROOT=/opt/homebrew/opt/openssl@3
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
```

  Record in the ledger the versions, the sandbox's result, that `B2` now runs
  after `source .local/env.sh`, and that Playwright's
  `chromium_headless_shell-1243` is in `~/Library/Caches/ms-playwright` (F22).

### Task 1: Exceptions on every target

The owner's decision of 2026-10-08, applied across the superproject and the
published libraries (C2, C3). No trystero in it.

**Files** (lines are F29's, read at `4f70819`; read each in context):
- Superproject, modify: `Jamroot` 249-264; `tools/webcpp.jam` 84-89;
  `tools/component/component.jam` 450-460; `tools/lint/compile_commands.py`
  docstring 47-63 and the twin (around 450-470); `tools/lint/lint.sh` 25-40;
  `README.md` 115-121; `AGENTS.md` 72, 343, 549-578, 590, 1035, 1057, 1142;
  `tools/test/webcpp_jam_test.py` (docstring 8-20, cases 96, 802, 816);
  `tools/test/component_jam_test.py` 388-395 and the script at 545-560;
  `tools/lint/lint_test.py` 303-339 and 619-649;
  `tools/report/record_samples.py` 116-120 and 163-165,
  `tools/report/samples/wasip2-*.xml` (re-recorded), `report_test.py`
  615-642; fixtures `demo/test/pass.cpp` 7-25, `demo/test/parses_json.cpp`
  7, `demo/example/Jamfile` 13, `component_demo/test/answers.cpp` 14 and
  56-58
- xactor: `AGENTS.md` 18-21, `doc/guarantees.adoc` 75-80,
  `doc/introduction.adoc` 44-50, `doc/testing.adoc` 145-157,
  `test/CONVERSION.md` 27-30
- xstate: `AGENTS.md` 29-32, `README.md` 22, `doc/appendices/rationale.adoc`
  33, 165-187, 317, `doc/guarantees.adoc` 145, `doc/introduction/overview.adoc`
  134-143 and 539-545, `doc/testing.adoc` 573-576, 606-615, 1011-1012,
  `test/CONVERSION.md` 43-46
- pratt: `AGENTS.md` 70-71, `doc/testing.adoc` 150-182
- wasi: `AGENTS.md` 38-42 and 94-98, `README.md` 18-24, 85-96, 175-180,
  238-248, `doc/boost.adoc` 66-124, `doc/introduction.adoc` 79-81,
  `doc/main.adoc` 54-61, `doc/testing.adoc` 160-165, `doc/wasip3.adoc`
  29-44, `example/by_hand/build.sh` 40-50,
  `include/webcpp/wasi/http/entrypoint.hpp` 56-59

**Interfaces:**
- Produces: the Jamroot region `wasi-exceptions`, which replaces
  `wasip2-exceptions` and `wasip3-exceptions`:

```
      # Every WASI target builds with exceptions, as native does: whether a
      # program uses them is its developer's choice. They are encoded as the
      # standard exception handling of WebAssembly (exnref): the clang of
      # wasi-sdk 34 emits the legacy encoding by default, which wasmtime 47
      # refuses to run ("legacy_exceptions feature required"). The runtime is
      # libunwind. A user's exception-handling=off builds with -fno-exceptions
      # alone, and webcpp.jam links tools/throw_exception.cpp.
      # tag::wasi-exceptions[]
      <toolset>clang-wasip2,<exception-handling>on:<cxxflags>"-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false"
      <toolset>clang-wasip2,<exception-handling>on:<linkflags>"-fwasm-exceptions -lunwind"
      <toolset>clang-wasip3,<exception-handling>on:<cxxflags>"-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false"
      <toolset>clang-wasip3,<exception-handling>on:<linkflags>"-fwasm-exceptions -lunwind"
      # end::wasi-exceptions[]
```

- `.handler` in `tools/webcpp.jam` stays (`<exception-handling>off:<source>/webcpp//throw_exception`),
  its comment: "What links tools/throw_exception.cpp, the handler Boost calls
  in place of a throw, into a program a user builds with
  exception-handling=off, on any target. webcpp builds no variant of its own
  without exceptions, nor any without RTTI."
- `compile_commands.py`: every aggregate entry is followed by its twin, the
  same target's command from the dry run with `exception-handling=off` and
  `/webcpp//throw_exception` (`WITHOUT_EXCEPTIONS`, `HANDLER`): natively as
  today, and now also for a library whose aggregate builds only for WASI, on
  wasip2 and on wasip3. Task 4 extends the twin to emscripten and leaves out
  headers that declare they need exceptions.
- AGENTS.md chapter 6, "Exceptions and RTTI", whole:

```
### Exceptions and RTTI

- **Every target builds with exceptions on.** Whether a program uses them is
  the choice of whoever builds it, never an imposition of a library or of
  webcpp. Natively the compiler's default holds. On wasip2 and wasip3 the
  Jamroot compiles with `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`
  and links with `-fwasm-exceptions -lunwind` (its region
  `wasi-exceptions`): wasi-sdk 34 emits the legacy encoding by default, which
  wasmtime 47 refuses to run ("legacy_exceptions feature required"). A
  program that uses exceptions on wasip2 needs a host that runs them:
  wasmtime 47 or later (older ones with `-W exceptions=y`), jco with
  `jco transpile --bindgen-enable-wasm-exnref`, or another Wasm 3.0 engine.
- **A user's build without exceptions.** `exception-handling=off` compiles
  with `-fno-exceptions` on any target, Boost.Config then defines
  `BOOST_NO_EXCEPTIONS`, and `webcpp.jam` links `tools/throw_exception.cpp`,
  a `boost::throw_exception` handler that prints and aborts; MSVC also gets
  `_HAS_EXCEPTIONS=0`. webcpp builds no such variant of its own.
- **A library works without exceptions.** Its headers compile with
  `-fno-exceptions`: what throws, tries or catches sits behind
  `#ifndef BOOST_NO_EXCEPTIONS`, and a failure that cannot be returned goes
  through `boost::throw_exception`. The lint checks it, by analysing every
  aggregate translation unit a second time as b2 compiles it with
  `exception-handling=off` (below).
- **RTTI** is never restricted by webcpp, on any target, and no variant is
  built without it; a user imposes their own.
```

  Task 4, which adds the mechanism to the lint, appends to the third bullet:
  "The one exception is a header that wraps a dependency which reports
  errors by throwing: its library declares it with the usage requirement
  `<exception-handling>on` on the target that brings the dependency, with the
  reason in a comment, the lint leaves that header out of the analysis
  without exceptions, and the header opens with `#ifdef BOOST_NO_EXCEPTIONS`
  and an `#error` that names the dependency."

- [ ] **Step 1: Write the failing tests** (rewritten red first):
  - `webcpp_jam_test.py`:

```python
def test_wasip2_skips_native_only_and_has_exceptions(root): ...
    # B2 toolset=clang-wasip2 testing.launcher=wasmtime libs/demo/test libs/demo/example -> 0;
    # pass.output starts with built_with(exceptions=True, rtti=True); the example catches (now
    # declared for every target: the fixture's `: : native wasip3` goes) prints its catch under
    # wasmtime; with exception-handling=off added, aborts.output holds 'throw_exception: planted'
    # and every compile line of the run holds -fno-exceptions and no -fwasm-exceptions
def test_every_program_sees_cxx20_and_exceptions_on_every_target(root): ...
    # toolchain.output starts with 'exceptions\n' natively, on WASIP2 and on WASIP3; with
    # exception-handling=off it starts with 'no exceptions\n' natively and on WASIP2
def test_a_users_build_without_exceptions_links_the_handler(root): ...
    # unchanged natively, plus the same on WASIP2: aborts passes, the handler printed
```

  - `component_jam_test.py`: `test_served_component_green_on_wasip2_and_wasip3`
    asserts `'throw_exception.o' not in line` on both versions and
    `'-fwasm-exceptions' in line`; the by-hand script compiles with
    `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false` and links
    `-fwasm-exceptions -lunwind` on both versions, without
    `-DBOOST_NO_EXCEPTIONS` or `throw_exception.cpp`.
  - `lint_test.py`: `test_compile_database_reads_what_only_wasi_builds`
    expects component_demo's aggregate four times, `(wasip2, with),
    (wasip2, without), (wasip3, with), (wasip3, without)`, each without
    holding `-fno-exceptions` exactly when "without";
    `test_clang_tidy_reads_what_only_a_build_without_exceptions_compiles`
    plants its `#ifdef BOOST_NO_EXCEPTIONS` finding in component_demo's
    WASI-only header too, and the lint fails naming it.
  - `report_test.py`: the re-recorded `wasip2-pass` holds no
    `exception-handling-off` in any build path; `wasip2-dependency` is a link
    failure of `demo/parses_json` that names `boost_json`.
- [ ] **Step 2: Run them and watch them fail.** `python3
  tools/test/webcpp_jam_test.py`, `component_jam_test.py`,
  `tools/lint/lint_test.py`, `tools/report/report_test.py`. Expected: each
  rewritten case FAILS (wasip2 still without exceptions).
- [ ] **Step 3: Implement the superproject.**
  - The Jamroot region above, in place of lines 249-264.
  - The comments of `tools/webcpp.jam` 84-89 (above),
    `tools/component/component.jam` ("and the handler of a program a user
    builds with exception-handling=off"), `compile_commands.py`'s docstring,
    `lint.sh` 25-40 (the twin on every aggregate).
  - `compile_commands.py`: the twin for WASI-only aggregates.
  - `record_samples.py`: `plant_broken_handler` becomes
    `plant_broken_boost_json`, which appends `#error "planted: Boost.JSON's
    definitions do not compile"` to `tools/boost_json.cpp`, and the sample is
    `'wasip2-dependency': Sample(WASIP2, ('libs/demo/test//parses_json',),
    plant_broken_boost_json)`, its comment "a dependency outside every test
    fails: Boost.JSON's definitions, which parses_json links". Re-record
    every `wasip2-*` sample with `python3 tools/report/record_samples.py`
    (never edited by hand).
  - Fixtures: `demo/example/Jamfile` declares `webcpp.example catches.cpp ;`;
    the comments of `pass.cpp`, `parses_json.cpp` and `answers.cpp` say "a
    user's build without exceptions" where they say "wasip2".
  - `README.md` 115-121: "Every target builds with exceptions: whether a
    program uses them is the choice of whoever builds it, and webcpp builds
    no variant without them, and none without RTTI."
  - AGENTS.md: chapter 6's section above; line 72 "what Boost calls in place
    of a throw, in a user's build without exceptions"; 343 "so a user's build
    without exceptions gets them built without exceptions too"; 590's lint
    row (the twin on every aggregate, natively and on each WASI target);
    1035 `webcpp.run`'s note "built once per target it declares; a user's
    `exception-handling=off` links `tools/throw_exception.cpp`"; 1057
    `webcpp.serve`'s "linking `tools/throw_exception.cpp` in a user's build
    without exceptions"; 1142 "a user may build a test without exceptions".
- [ ] **Step 4: The libraries' texts,** each saying what is true. The
  replacements, by file:
  - xactor `AGENTS.md` 18-21 and xstate `AGENTS.md` 29-32: "...throws
    nothing of its own, so its headers compile without exceptions, which the
    lint checks. Whether a program uses exceptions is its user's choice, on
    every target."
  - xactor `guarantees.adoc` invariant 20 and xstate `guarantees.adoc` 145:
    "...and the library compiles without exceptions, which the lint checks
    for every public header" in place of "compiles and runs without
    exceptions, as its wasip2 build does". The anchors and numbers stay.
  - xactor `introduction.adoc` 44-50 and xstate `overview.adoc` 539-545: "it
    is built and tested with exceptions on, on every target, and its headers
    compile without them".
  - The testing chapters' wasip2 rows (xactor 150-155, xstate 610-615, pratt
    157-161): "The toolset `clang-wasip2`, the Clang of wasi-sdk for
    wasm32-wasip2, run by wasmtime. With exceptions, as WebAssembly's
    exception handling, as on wasip3." The native rows drop "as on wasip2".
    The commands' prose (xstate 573, pratt 179) drops "without exceptions".
    One sentence after each table: "That <name> works without exceptions is
    checked by the lint, which compiles every public header with
    `exception-handling=off`."
  - xstate `rationale.adoc` 33, 165-187, 317 and `testing.adoc` 1011-1012:
    the examples built for wasm32-wasip2 are built with exceptions; the
    rationale for errors as values stays (a user's build without exceptions).
  - xactor and xstate `CONVERSION.md`: "\"Without exceptions\" is now checked
    by the lint, which compiles every public header with
    `exception-handling=off`" in place of "is now checked on wasip2 only".
  - pratt `AGENTS.md` 70-71: "**No exceptions in a header.** The headers
    compile without exceptions, which the lint checks: no `throw`, `try` or
    `catch` under `include/`."
  - wasi `doc/boost.adoc`: the sections "No exceptions on wasip2" and
    "Exceptions on wasip3" become one, `[#wasi-boost-exceptions] === Exceptions
    on wasip2 and wasip3`, which includes
    `include::{webcpp-root}/Jamroot[tag=wasi-exceptions,indent=0]`, keeps the
    explanation of `-fwasm-exceptions`, `-lunwind` and
    `-wasm-use-legacy-eh=false`, says what a wasip2 program that uses
    exceptions requires of its host (the three requirements of chapter 6's
    text above), and keeps the paragraph on `BOOST_NO_EXCEPTIONS` and
    `tools/throw_exception.cpp` for a user's build without exceptions. Every
    `<<wasi-boost-exceptions-wasip2>>` and `<<wasi-boost-exceptions-wasip3>>`
    (`grep -rn 'wasi-boost-exceptions' libs/wasi`) names the new anchor.
  - wasi `AGENTS.md` 38-42: "Every program is built with exceptions on
    wasip2 and wasip3, as the superproject's Jamroot builds any"; 94-98 names
    the region `wasi-exceptions` in place of the two.
  - wasi `README.md` 18-24, 175-180, `doc/main.adoc` 54-61,
    `doc/introduction.adoc` 79-81 and `entrypoint.hpp`'s `@note`: "On wasip2
    and wasip3, where webcpp builds a program with exceptions, an exception
    that escapes the main traps the instance: wasmtime 47 prints the trap and
    answers the client with its own `500 Internal Server Error`." The
    sentence "webcpp builds wasip2 without exceptions" goes everywhere.
  - wasi `example/by_hand/build.sh`'s p2 region builds as webcpp does:
    `"$WASI_SDK/bin/clang++" --target=wasm32-wasip2 -std=c++20
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -mexec-model=reactor
    ... -fwasm-exceptions -lunwind -o hello.wasm`; the README's copy of it
    follows (doc-check compares them); `doc/wasip3.adoc` 29-44 and README
    238-248 say that both regions build with exceptions, and that a build
    with `-fno-exceptions` in their place answers the same (measured once by
    hand on wasip3, as today).
  - wasi `doc/testing.adoc` 160-165: "Builds and runs, for wasm32-wasip2 and
    with exceptions, every test that...".
- [ ] **Step 5: Run them and watch them pass,** in parallel lanes, from
  scratch: every `tools/**/*_test.py`; for each of xactor, xstate, pratt and
  wasi, `B2 -a libs/<name>/test libs/<name>/example` natively, on
  `toolset=clang-wasip2 testing.launcher=wasmtime` and on `clang-wasip3`;
  wasi's own lanes `B2 -a toolset=clang-wasip2 testing.launcher=wasmtime
  libs/wasi/test//http libs/wasi/example//http` and on wasip3 (the served
  components with exceptions, served by wasmtime with `-S cli` alone); each
  lane's XML through `report.py`, exit 0; `B2 -a doc` (one doc build); the
  lint clean. Record in the ledger the size of `http_hello`'s component on
  wasip2 before and after (the spike measured +21%).
- [ ] **Step 6: Commit** in each library (`libs/xactor`, `libs/xstate`,
  `libs/pratt`, `libs/wasi`): "<name>: every target builds with exceptions,
  and the headers compile without them, which the lint checks" (wasi's: "wasi:
  wasip2 builds with exceptions, as wasip3 does"); then in the superproject,
  with the four submodule bumps: "Jamroot: every WASI target builds with
  exceptions, and the lint analyses every aggregate without them too".

### Task 2: Emscripten as a tested target: the build

The plan's original Task 1, without the `-noexcept` opt-out (C1).

**Files:**
- Modify: `Jamroot` (region `emscripten-target`), `tools/webcpp.jam`,
  `tools/lint/rules.py` and `lint_test.py` (raw rules `link`, `link-fail`,
  C35), `tools/doc/counts.py` and `counts_test.py` (kinds `link` and
  `drive`, C19), `tools/doc/doc-check.py`, `tools/doc/doc.jam` and
  `doc_check_test.py` (`--linked`), `tools/test/harness.py` and
  `harness_test.py`, `tools/test/webcpp_jam_test.py`,
  `tools/report/record_samples.py`, `report_test.py`, `tools/report/lanes.py`
  if a link test needs a step name, `AGENTS.md` chapters 1, 8, 9, 10, 12
- Create: `tools/drive/drive.jam`, `tools/test/emscripten_test.py`,
  `tools/report/samples/emscripten-pass.xml`,
  `tools/report/samples/emscripten-driven-failure.xml`, and the fixture
  `tools/test/fixtures/browser_demo/` (C7): `build.jam`,
  `include/webcpp/browser_demo.hpp`, `meta/libraries.json`, `README.md`,
  `test/Jamfile`, `test/reads_test.cpp`, `test/input.txt`,
  `test/catches_test.cpp`, `test/pointer_test.cpp`, `test/json_test.cpp`,
  `test/fails_test.cpp`, `test/driver/Jamfile`, `test/driver/drive.mjs`,
  `test/driver/package.json`, `test/driver/package-lock.json`,
  `test/driver/driven.cpp`, `example/Jamfile`, `example/hello.cpp`,
  `example/hello.input`, `example/hello.expected`, `example/page.cpp`,
  `doc/Jamfile`, `doc/browser_demo.adoc`

**Interfaces:**
- **The Jamroot mapping** (C34), beside `wasi-target`:

```
      # b2's emscripten builds wasm32 (browsers do not all run memory64, and
      # an address-model=64 request would add -sMEMORY64=1), without pthreads
      # (they need cross-origin isolation in a page), and static.
      # tag::emscripten-target[]
      <toolset>emscripten:<address-model>32
      <toolset>emscripten:<threading>single
      <toolset>emscripten:<link>static
      # end::emscripten-target[]
```

- **`webcpp.run`, `webcpp.run-fail` and `webcpp.example`** add
  `<toolset>emscripten:<linkflags>-sNODERAWFS=1` (C6): a program that node
  runs reads the host's files, its `<testing.input-file>` and its standard
  input. Nothing else gets it.
- **No launcher on emscripten** (C5): a program's conditional requirement
  stops a build whose toolset is emscripten and that has
  `<testing.launcher>`: "webcpp: b2's emscripten toolset runs a program with
  node itself; give no testing.launcher on emscripten". `run-and-compare`
  sets `LAUNCHER_ARGS` to `--launcher node` when `webcpp-target.target-of`
  is emscripten.
- **`webcpp.link <name> : <sources> + : <requirements> * : <targets> * ;`**
  declares the test `<name>`, b2's `link` (listed by `--dump-tests`, a link
  test in the report), and the explicit program `<name>-program`, an `exe`
  of the same sources and requirements, for a target that needs the files
  (trystero's chat `site`). Recorded with the kind `link`. It never gets
  `-sNODERAWFS=1` and never runs.
- **`webcpp.drive <name> : <sources> + : <requirements> * : <script> :
  <arguments> * : <targets> * ;`** (C10, loaded from `tools/drive/drive.jam`
  as the module `webcpp-drive`): the test `<name>` builds a program from the
  sources, as `webcpp.serve` builds its component, and runs `<original words>
  <script> <arguments> <program>`, where the original's words are the
  Jamfile's `webcpp.original` and `<program>` is the linked program's path
  (its `.js` on emscripten); it passes when that exits 0. Types:
  `type.register DRIVEN : driven ;`, `type.register DRIVE : : TEST ;`,
  `generators.register-standard webcpp-drive.drive-and-check : EXE : DRIVEN
  ;`, `generators.register-standard testing.expect-success : DRIVEN : DRIVE
  ;`, made with `testing.make-test drive`. It depends on the Jamfile's
  `node-modules`; a Jamfile without `webcpp.original` stops at it by name. It
  is always run again. Recorded with the kind `drive`.
- **Lane-only programs:** `served-program` becomes `lane-only-program found +
  : name : shown : on +`, which `webcpp.serve`, `webcpp.serve-script` and
  `webcpp.drive` call; `lanes` stops `b2 declared-lanes` naming a lane-only
  program that no lane of its Jamfile names, or whose lanes leave out one of
  its targets ("webcpp.drive driven in <jamfile> is in no own lane; name it in
  a webcpp.lane that names its targets").
- **Counts** (C19): `KINDS` gains `link` and `drive`; `{n-tests}` and
  `{n-tests-<target>}` count both; new `{n-linked}`, `{n-linked-<target>}`,
  `{n-driven}`, `{n-driven-<target>}`.
- **doc-check** `--linked <program.cpp>` (repeated), which `doc.jam` passes
  from the `link` records of the library's example Jamfiles: each such program
  must be shown by an include of its code, and an `.expected` beside it is a
  fault ("<program> is linked, never run: it has no output to compare").
- **Raw rules** (C35): `RAW_RULES` gains `'link': 'webcpp.link'` and
  `'link-fail': 'webcpp.link'`.
- **The harness:** `harness.link_emsdk(root)` links `$EMSDK_ROOT`, else this
  checkout's `.local/emsdk`, into the scratch's `.local/emsdk`, as
  `link_wasi_tools` does; `harness.b2_environment` always sets `EM_CACHE`,
  over the shell's, to `<scratch parent>/emscripten-cache`, one per test
  script run, made once and shared by its cases (Emscripten locks its
  cache), so that no test writes `.local/emscripten-cache` (C8).
- **The fixture `browser_demo`** (`webcpp.targets native emscripten`):

```
# test/Jamfile
project : requirements <library>/webcpp/browser_demo//browser_demo ;
import webcpp ;
webcpp.targets native emscripten ;
webcpp.headers-alone browser_demo : ../include ;
webcpp.run reads : reads_test.cpp : <testing.input-file>input.txt ;
webcpp.run catches : catches_test.cpp ;
webcpp.run pointer : pointer_test.cpp ;
webcpp.run json : json_test.cpp : <library>/webcpp//boost_json ;
webcpp.run-fail fails : fails_test.cpp ;

# test/driver/Jamfile
import webcpp ;
webcpp.targets native emscripten ;
webcpp.original node ;
webcpp.drive driven : driven.cpp : : drive.mjs : --program ;
webcpp.lane driver : driven : native emscripten ;

# example/Jamfile
project : requirements <library>/webcpp/browser_demo//browser_demo ;
import webcpp ;
webcpp.targets native emscripten ;
webcpp.example hello.cpp ;
webcpp.link page : page.cpp : <linkflags>-sENVIRONMENT=web : emscripten ;
```

  `reads_test.cpp` reads `argv[1]` and checks it holds `hello`;
  `catches_test.cpp` throws and catches; `pointer_test.cpp` checks
  `sizeof(void*) == 4` under `__EMSCRIPTEN__`; `fails_test.cpp` returns 1;
  `hello.cpp` reads a line from standard input and prints `hello, <line>`
  (`hello.input` holds `node`); `page.cpp`'s `main` returns 1; `driven.cpp`
  prints `ready`; `drive.mjs --program <path>` runs the program (with node
  when it ends in `.js`) and exits 0 when it printed `ready`.
  `package.json` has no dependency, and `package-lock.json` is what `npm
  install --package-lock-only` writes for it.

- [ ] **Step 1: Write the failing tests.** `tools/test/emscripten_test.py`,
  on the harness (scratch copies whose path holds a space, `browser_demo` in
  `libs/`):

```python
def test_emscripten_lane_runs_under_node(root): ...
    # B2 -a toolset=emscripten libs/browser_demo/test libs/browser_demo/example -> 0; passed ==
    # {alone-browser_demo, reads, catches, pointer, json, fails, hello, page}; the run lines are
    # `"node" "<...>/<name>.js"`, never `node "node"`; hello.output == 'hello, node\n'
def test_a_launcher_is_refused_on_emscripten(root): ...
    # the same with testing.launcher=node -> != 0, naming testing.launcher and emscripten
def test_input_file_is_read_from_the_host(root): ...
    # reads passes on emscripten; its link line holds -sNODERAWFS=1; page's does not
def test_programs_are_wasm32(root): ...
    # pointer passes on emscripten, also with address-model=64 requested; no build directory
    # holds address-model-64
def test_a_failure_reaches_the_report(root): ...
    # reads_test.cpp planted to expect 'goodbye'; B2 -a --dump-tests --out-xml=x.xml
    # toolset=emscripten libs/browser_demo/test; report.py --lane emscripten=x.xml exits 1 naming
    # browser_demo/reads as a run failure
def test_link_only_builds_and_never_runs(root): ...
    # page (main returns 1, -sENVIRONMENT=web) passes; page.js and page.wasm exist; no line of
    # b2's output runs it
def test_link_only_unresolved_symbol_fails(root): ...
    # page.cpp planted with a call of an undeclared extern function -> != 0, naming it
def test_drive_runs_its_program_on_each_target(root): ...
    # B2 -a toolset=emscripten libs/browser_demo/test/driver//driver -> 0, driven passed; the same
    # natively; drive.mjs planted to exit 1 -> the driven test fails in --out-xml as a run
def test_a_drive_outside_every_lane_is_refused(root): ...
    # the lane line removed: B2 -d0 declared-lanes -> != 0, naming webcpp.drive driven
def test_native_build_needs_no_emsdk(root): ...
    # with no `using emscripten` in the scratch's user-config: native B2 -a libs/browser_demo/test -> 0
```

  `webcpp_jam_test.py`: `programs-of` records `link page emscripten` and
  `drive driven native emscripten`. `lint_test.py`: a raw `link` in a
  library's test Jamfile fails, naming the file and line. `counts_test.py`:
  `{n-linked}`, `{n-linked-emscripten}`, `{n-driven}` and `{n-tests}` of
  `browser_demo`. `doc_check_test.py`: a page that does not show `page.cpp`
  fails naming it; a `page.expected` beside it fails; shown, it passes
  `--complete`. `report_test.py`: the sample `emscripten-pass` gives green
  cells, `page` a link test; labelled `wasip2` it exits 2;
  `emscripten-driven-failure` gives a red run cell for `driven`.
  `harness_test.py`: `link_emsdk` links `$EMSDK_ROOT` and fails naming it when
  it holds no `upstream/emscripten/em++`.
- [ ] **Step 2: Run them and watch them fail.** `python3
  tools/test/emscripten_test.py`. Expected: FAIL (`webcpp.link` and
  `webcpp.drive` unknown; `reads` reads an empty file, F4).
- [ ] **Step 3: Implement,** the rules and the module above. Measure first,
  and record in the ledger: what b2's `exe` on emscripten lists as its first
  target (the `.js`, beside the `.wasm`, F3); whether node under
  `-sNODERAWFS=1` reads standard input (`hello`); and the step names b2's
  `link` test writes to `--out-xml`.
- [ ] **Step 4: Record the samples** with `python3
  tools/report/record_samples.py` (`EMSCRIPTEN = ('toolset=emscripten',)`,
  `'emscripten-pass': Sample(EMSCRIPTEN, ('libs/browser_demo/test',
  'libs/browser_demo/example'), fixture='browser_demo')`, and
  `'emscripten-driven-failure'` with `drive.mjs` planted to exit 1).
- [ ] **Step 5: AGENTS.md,** for what this task adds: chapter 1's layout
  (`tools/drive/`, the fixture `browser_demo`); chapter 8's counts
  (`{n-linked}`, `{n-driven}`) and doc-check's `--linked`; chapter 9's
  targets table (emscripten: "b2's `emscripten`, wasm32, single-threaded,
  static | node, run by the toolset itself; no `testing.launcher`"), the API
  table rows of `webcpp.link` and `webcpp.drive`, and the lane-only rule;
  chapter 10's test list gains `tools/test/emscripten_test.py`; chapter 12
  gains "**b2's emscripten toolset runs a test with node itself:**
  `testing.launcher=node` makes the command `node "node" x.js`, and webcpp
  refuses it".
- [ ] **Step 6: Run them and watch them pass,** every `tools/**/*_test.py`
  green in parallel, and the lint clean.
- [ ] **Step 7: Commit:** "tools: emscripten tests under node, link-only
  programs, and driven tests in own lanes".

### Task 3: Emscripten in CI: emsdk.jam, the emsdk action, the plan

**Files:**
- Create: `tools/ci/emsdk.jam`, `tools/ci/actions/emsdk/action.yml`,
  `tools/ci/actions/emsdk/install.sh`
- Modify: `tools/ci/matrix.py`, `tools/ci/matrix_test.py`,
  `tools/ci/actions_test.py`, `.github/workflows/library.yml`, `AGENTS.md`
  chapter 9 (targets table, CI, actions) and chapter 13 (the emscripten lane
  leaves the roadmap)

**Interfaces:**
- `tools/ci/emsdk.jam` (no build loads it; C17):

```
# tag::emsdk[]
local emsdk = /path/to/emsdk ;
# end::emsdk[]

# tag::emscripten[]
using emscripten : : $(emsdk)/upstream/emscripten/em++ ;
# end::emscripten[]
```

- `matrix.py`: `Lane` gains `emsdk: bool = False` ("whether the lane needs
  emsdk and Node"); `EMSDK_JAM = ROOT / 'tools/ci/emsdk.jam'`;
  `EMSCRIPTEN_TOOLSET = '{emsdk.jam}'`, which `register` expands to the
  regions `emsdk` and `emscripten` with the action's install directory,
  quoted, in place of `/path/to/emsdk` and `$(emsdk)`; the lane
  `Lane(id='emscripten', name='Emscripten 6.0.11 (node)', os='ubuntu-24.04',
  target='emscripten', lane='emscripten', toolset='emscripten',
  using=EMSCRIPTEN_TOOLSET, emsdk=True)`; `OWN_LANE_BASES['emscripten'] =
  'emscripten'`; an own-lane entry carries `emsdk` from its base. The
  messages that refuse emscripten go.
- The action `emsdk` (`install.sh` beside `action.yml`): downloads the emsdk
  repository's archive at the commit of its tag for 6.0.11 with
  `tools/ci/download.sh`, checked by the SHA-256 measured in Step 3, into
  `.local/emsdk`; runs `./emsdk install 6.0.11` and `./emsdk activate 6.0.11`;
  checks that `upstream/emscripten/emcc --version` names `6.0.11
  (a0014542110d6078c3a1a7941fa1ddb3a2281f16)`, else fails naming both; writes
  `EM_CACHE=$GITHUB_WORKSPACE/.local/emscripten-cache` to `$GITHUB_ENV`;
  caches `.local/emsdk` and the cache directory, keyed on the version and the
  runner. It refuses a runner other than Linux and macOS, by name.
- `library.yml`: a step `&emsdk` (`if: matrix.emsdk`, `uses:
  ./tools/ci/actions/emsdk`) in the lanes and own-lanes jobs, with the Node
  action under the same condition (b2 runs emscripten's tests with node); the
  own-lanes job's Node condition becomes `${{ !matrix.platform ||
  matrix.emsdk }}`; the lint job installs emsdk and runs `matrix.py register
  clang-18 wasip2 wasip3 emscripten`; the docs and tools jobs install emsdk.

- [ ] **Step 1: Write the failing tests.** `matrix_test.py`: `plan` for a
  library declaring emscripten emits the lane `emscripten` with `emsdk: true`
  (today exit 2, F15); `own-lanes` for an own lane on emscripten emits
  `{platform: emscripten, id: emscripten.<library>.<lane>, emsdk: true}`;
  `register emscripten` writes the two regions with the directory quoted.
  `actions_test.py`: the emsdk script installs into `.local/emsdk`, refuses
  an unknown runner by name, and fails naming the version when `emcc
  --version` differs (with a stand-in emsdk). Then `actionlint` on
  `library.yml`.
- [ ] **Step 2: Run them and watch them fail.**
- [ ] **Step 3: Implement.** Read the commit of emsdk's tag `6.0.11` with `gh
  api repos/emscripten-core/emsdk/git/ref/tags/6.0.11`, download its archive,
  and record its SHA-256 (`shasum -a 256`) in the script; never invent one.
  Pin any third-party action by its full commit SHA.
- [ ] **Step 4: Run them and watch them pass;** `actionlint` clean; `python3
  tools/ci/matrix.py plan --library browser_demo` in a scratch copy with the
  fixture emits its native lanes and the emscripten lane; the lint clean.
- [ ] **Step 5: Commit:** "tools/ci: an emscripten lane, its emsdk action,
  and emscripten own lanes".

### Task 4: The lint and the reference read emscripten-only and native-dependency headers

The plan's original Task 2, with C3, C14, C15, C20, C21 and C22.

**Files:**
- Modify: `tools/lint/compile_commands.py`, `tools/lint/lint.sh`,
  `tools/lint/rules.py`, `tools/lint/lint_test.py`, `tools/doc/reference.py`,
  `tools/doc/doc.jam`, `tools/test/doc_test.py`, `Jamroot` (the target
  `emscripten-headers`), `tools/doc/docinfo.html` (its notice), `AGENTS.md`
  chapters 6, 7, 11
- Modify the fixture `browser_demo`: `include/webcpp/browser_demo/page.hpp`
  (includes `<emscripten/val.h>`, builds only on emscripten),
  `include/webcpp/browser_demo/native.hpp` (includes `<fake_dependency.hpp>`
  from the fixture's `deps/include`, builds only natively), `deps/include/fake_dependency.hpp`,
  `build.jam` (`alias native : : : : <library>browser_demo
  <include>deps/include <exception-handling>on ;` with the comment "the fake
  dependency reports errors by throwing"), `test/Jamfile` (three
  `webcpp.headers-alone` calls, as trystero's), `doc/Jamfile`
  (`webcpp.reference browser_demo : <library>/webcpp/browser_demo//native
  <library>/webcpp//emscripten-headers ;`)

**Interfaces:**
- **Aggregates** (`compile_commands.py`): stage 3's rule holds for a library
  whose public headers one target builds whole. A library whose headers no
  single target builds gets one aggregate per target its headers-alone units
  run on, each of the headers that target builds:
  `bin/aggregate/<name>-<target>.cpp`, written by `write_aggregate(root,
  library, source, headers)` (the new optional `headers`, every public header
  when absent). trystero's: `-native` (core, `protocol/`, `detail/`,
  `native/`) and `-emscripten` (core, `protocol/`, `detail/`, `browser/`).
- **The twin of every aggregate:** its target's command from the dry run with
  `exception-handling=off`, holding the aggregate's headers less those whose
  headers-alone unit still compiles with exceptions in that run (a call whose
  requirements, or whose target's usage requirements, give
  `<exception-handling>on`). A twin left with no header is no entry.
- **em++ commands** (C14): an emscripten command is kept with its compiler
  replaced by wasi-sdk's `clang++`, after the words of `em++ --cflags` (read
  once per run; target, sysroot, `-iwithsysroot` compat and fakesdl), b2's own
  words last so that b2's `-fwasm-exceptions` follows `-fignore-exceptions`;
  `lint.sh` gives such a command no host `--target`. A WebAssembly command of
  any other compiler still fails by name.
- **`/webcpp//emscripten-headers`**, an explicit alias in the Jamroot whose
  usage requirements put Emscripten's `upstream/emscripten/system/include` on
  the include path, looked up as MrDocs is: `-sEMSDK=<dir>`, else
  `.local/emsdk`; missing, the build stops naming `EMSDK` and both places, only
  when a target uses it (C15).
- **The reference** stays one native parse of every public header (C15):
  `webcpp.reference <name> : <requirements> ;` with the native backend's
  target and `/webcpp//emscripten-headers`. MrDocs does not parse an
  emscripten command on macOS (F11); no `also-checked` (no `__EMSCRIPTEN__`
  branch in the browser headers).
- **The io_context rule** (C20) also finds an `io_context` declared through a
  namespace alias (`asio::io_context io;`), and accepts a line that says why
  after `lint-run:` (`io.run_for(timeout);  // lint-run: the memory relays
  post to it`).
- **The world rule** (C21) also names `getentropy`, `getrandom`,
  `arc4random`, `RAND_bytes` and `<sys/random.h>`.
- **The licence rule** (C22): `comment_marker` gives `//` for `.js` and
  `.adoc`, and `<!--` for `.html`; `tools/doc/docinfo.html` gains the notice.

- [ ] **Step 1: Write the failing tests.**
  - `lint_test.py`: the lint analyses `page.hpp` (a planted
    `readability-*` finding in it fails, naming it); the database holds
    `browser_demo-native.cpp`, `browser_demo-emscripten.cpp` and their twins,
    and the native twin includes neither `page.hpp` nor `native.hpp`; a
    `throw` planted in `browser_demo.hpp` fails the lint (the native twin),
    one planted in `page.hpp` fails it too (the emscripten twin), and one
    planted in `native.hpp` does not; an alias-declared `io.run_for()` fails,
    and passes with `lint-run:`; `getentropy` in a header fails without
    `lint-world:`; a `.js`, an `.html` and an `.adoc` without the notice each
    fail, naming the file.
  - `lint_test.py`, a regression pin that passes before and after (record it
    so; `.clang-tidy`'s `HeaderFilterRegex` already limits findings, F13): a
    warning planted in `fake_dependency.hpp` does not fail the lint.
  - `doc_test.py`: `B2 libs/browser_demo/doc//reference` documents
    `page.hpp`'s and `native.hpp`'s functions; an undocumented one in either
    fails, naming it; with `-sEMSDK=/nonexistent` and no `.local/emsdk`, it
    stops naming `EMSDK`.
- [ ] **Step 2: Run them and watch them fail.**
- [ ] **Step 3: Implement.** Measure, and record: whether b2 lets the usage
  requirement `<exception-handling>on` of `browser_demo//native` override a
  request's `exception-handling=off` for its dependents (if it does not, the
  rule of Global Constraints is written on each headers-alone call, test and
  program of the native backend instead, and AGENTS.md says so); that
  wasi-sdk's clang 23.1 reads Emscripten's clang 24 headers (F10).
- [ ] **Step 4: Run them and watch them pass;** xactor's, xstate's, pratt's
  and wasi's databases and references are unchanged except for the twins Task
  1 added (compare before and after); the lint clean over the superproject.
  A line of another library that the extended io_context, world or licence
  rule now flags gets its `lint-run:`, `lint-world:` or notice in that
  library's repository, committed in this task with its submodule bump.
- [ ] **Step 5: AGENTS.md:** chapter 6's "Exceptions and RTTI" gains the
  sentence Task 1 left for this task; chapter 6's lint table (the clang-tidy row:
  per-target aggregates, the twin of each, em++ mapped; the io_context row
  with `lint-run:`; the world row with the entropy names; the licence row);
  chapter 7 (the reference's emscripten headers, `-sEMSDK`); chapter 11 (the
  notice on `.js`, `.html`, `.adoc`).
- [ ] **Step 6: Commit:** "tools: the lint and the reference read headers
  that build only on emscripten or against a native dependency".

### Task 5: trystero moved: headers, build.jam, examples

The plan's original Task 3, with C13, C30, C36 and the GCC 15 fix (F13).

**Files:**
- Create the repository `~/dev/webcpp/libs/trystero` (`git init -b main`).
- Move from SOURCE: `include/trystero/**` to `include/webcpp/trystero/**`,
  except `trystero.hpp`, which becomes `include/webcpp/trystero.hpp`;
  `example/native_quick_start.cpp`, `example/quick_start.cpp`,
  `example/chat/chat.cpp`, `example/chat/index.html`;
  `test/support/memory_backend.hpp`.
- Create: `build.jam`, `example/Jamfile`, `example/chat/Jamfile`,
  `test/Jamfile` (headers-alone only), `meta/libraries.json`,
  `meta/include-boundaries.json`, `README.md`, `AGENTS.md`,
  `LICENSE_1_0.txt`, `LICENSE-TRYSTERO.txt`, `.gitignore`, `.gitattributes`
  (as xactor's)

**Interfaces:**
- `build.jam`:

```
project /webcpp/trystero ;

# libsecp256k1, libdatachannel and OpenSSL are found where SECP256K1_ROOT,
# SECP256K1_EMSCRIPTEN_ROOT, LIBDATACHANNEL_ROOT and OPENSSL_ROOT say, until
# webcpp builds them from third_party/; a variable that is not set leaves
# the library to the compiler's default search path.
import os ;
local secp256k1-root = [ os.environ SECP256K1_ROOT ] ;
local secp256k1-emscripten-root = [ os.environ SECP256K1_EMSCRIPTEN_ROOT ] ;
local libdatachannel-root = [ os.environ LIBDATACHANNEL_ROOT ] ;
local openssl-root = [ os.environ OPENSSL_ROOT ] ;
lib secp256k1 : : <name>secp256k1 <toolset>emscripten <search>$(secp256k1-emscripten-root)/lib
  : : <include>$(secp256k1-emscripten-root)/include ;
lib secp256k1 : : <name>secp256k1 <search>$(secp256k1-root)/lib
  : : <include>$(secp256k1-root)/include ;
lib datachannel : : <name>datachannel <search>$(libdatachannel-root)/lib
  : : <include>$(libdatachannel-root)/include <dll-path>$(libdatachannel-root)/lib ;
lib ssl : : <name>ssl <search>$(openssl-root)/lib : : <include>$(openssl-root)/include ;
lib crypto : : <name>crypto <search>$(openssl-root)/lib : : <include>$(openssl-root)/include ;

alias trystero : : : : <include>include <library>/boost//headers <library>/webcpp//boost_json
  <library>secp256k1
  <toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS
  <toolset>emscripten:<embind>on
  <conditional>@trystero-dependencies ;

# The native backend needs exceptions: libdatachannel reports errors by
# throwing, and data_channel.hpp and peer_connection.hpp catch them.
alias native : : : : <library>trystero <library>datachannel <library>ssl <library>crypto
  <exception-handling>on ;
```

  `trystero-dependencies` stops the build, naming the variable, when a
  variable is set and its directory lacks the header the library needs
  (`include/secp256k1.h`, `include/rtc/rtc.hpp`, `include/openssl/ssl.h`),
  only when a target that needs it is built.
- Every native header (`native/*.hpp`) opens, after its include guard, with:

```cpp
#ifdef BOOST_NO_EXCEPTIONS
#error "<webcpp/trystero/native/...> needs exceptions: libdatachannel reports errors by throwing"
#endif
```

- `browser/src.hpp` keeps only the embind registration of
  `trystero_dispatch`; its Boost.JSON part goes, since `/webcpp//boost_json`
  builds on emscripten (F5), and its comment says why (C36).
- `test/Jamfile`:

```
project : requirements <library>/webcpp/trystero//trystero ;

import webcpp ;

webcpp.targets native emscripten ;

webcpp.headers-alone trystero : ../include
  : trystero.hpp trystero/*.hpp trystero/protocol/*.hpp trystero/detail/*.hpp ;
webcpp.headers-alone trystero : ../include : trystero/native/*.hpp
  : <library>/webcpp/trystero//native : native ;
webcpp.headers-alone trystero : ../include : trystero/browser/*.hpp : : emscripten ;
```

- `example/Jamfile`:

```
project : requirements <library>/webcpp/trystero//trystero ;

import webcpp ;

webcpp.targets native emscripten ;

# Each needs the public relays or a browser, so it is linked and never run.
# tag::native[]
webcpp.link native_quick_start : native_quick_start.cpp
  : <library>/webcpp/trystero//native : native ;
# end::native[]
# tag::browser[]
webcpp.link quick_start : quick_start.cpp
  : <linkflags>-sENVIRONMENT=web <linkflags>-sALLOW_MEMORY_GROWTH : emscripten ;
# end::browser[]
```

  `example/chat/Jamfile` declares `webcpp.link chat : chat.cpp
  : <linkflags>-sENVIRONMENT=web <linkflags>-sALLOW_MEMORY_GROWTH :
  emscripten ;` and the explicit `make site` whose action copies
  `chat-program`'s `.js` and `.wasm` and `index.html` into
  `<build-dir>/site/` and prints that directory (the site stays out of the
  tree, C13).
- `meta/include-boundaries.json`:

```json
{
    "boundaries": [
        {
            "headers": [
                "include/webcpp/trystero.hpp",
                "include/webcpp/trystero/*.hpp",
                "include/webcpp/trystero/protocol/*.hpp",
                "include/webcpp/trystero/detail/*.hpp"
            ],
            "must-not-include": [
                "webcpp/trystero/native",
                "webcpp/trystero/browser",
                "rtc/",
                "emscripten/",
                "openssl/"
            ],
            "why": "the core builds on both targets, without libdatachannel, OpenSSL or Emscripten"
        }
    ]
}
```

- `.gitignore`: `doc/html/`, `node_modules`, `.node-modules/` (C13).

- [ ] **Step 1: Make the build the failing test.** Write `build.jam` and the
  Jamfiles first, then run `B2 libs/trystero/example`. Expected: FAIL,
  `<webcpp/trystero.hpp>` not found.
- [ ] **Step 2: Move and rename.**
  - First list every name to change and every string that must not, and
    record both in the ledger: `grep -rn 'namespace trystero\|trystero::'`,
    `grep -rhoE 'TRYSTERO_[A-Z0-9_]+' | sort -u`, `grep -rn '<trystero/'`,
    and every string literal and embind name holding `trystero` in any case
    (`grep -rn '"[^"]*[Tt]rystero'`; F24 lists them).
  - Rename with exact C++ patterns: `namespace trystero` to `namespace
    webcpp::trystero` (or the nested form the code uses), `trystero::` in
    code (never in a string literal) to `webcpp::trystero::` where it names
    the namespace from outside, `<trystero/` to `<webcpp/trystero/`,
    `TRYSTERO_` to `WEBCPP_TRYSTERO_`, the category name `"trystero"` to
    `"webcpp.trystero"`; the app ids to `webcpp-trystero-quick-start` and
    `webcpp-trystero-chat`.
  - Keep byte for byte every wire string and embind name of the second list.
  - Add `#include <utility>` to `detail/action_wire.hpp` and
    `detail/peer_link.hpp` (`std::exchange`, F13).
  - Replace every licence notice with the WebCpp.org notice.
  - `tools/lint/lint.sh ...`: its banned-word section names nothing in
    `libs/trystero` (it reads the untracked files of a repository that
    tracks nothing yet).
- [ ] **Step 3: Run it and watch it pass,** in parallel lanes, from scratch:
  `B2 -a libs/trystero/example libs/trystero/test` natively with clang and
  with gcc-15 (the gcc-15 headers-alone units are the test of the
  `std::exchange` fix), and with `toolset=emscripten`; then `B2
  libs/trystero/example/chat//site`, which prints the site's directory with
  three files. Every program links on its targets; `B2 -d0 declared-targets`
  lists `trystero emscripten` and `trystero native`; a user's
  `exception-handling=off` build of `libs/trystero/test` natively still
  compiles the native headers with exceptions (Task 4's measurement holds).
- [ ] **Step 4: Write the metadata and the texts.**
  - `meta/libraries.json`: key and name `trystero`, authors `["R"]`,
    maintainers `["R <github.overboard312@passmail.net>"]`, description "Serverless
    WebRTC rooms for C++20, wire compatible with Trystero.", category
    `["Concurrent", "IO"]`, cxxstd `"20"`, `"port-of": {"name": "Trystero",
    "language": "JavaScript", "version": "0.26.0", "url":
    "https://github.com/dmotz/trystero", "licence": "MIT"}`.
  - `LICENSE-TRYSTERO.txt` (Global Constraints), checked by its SHA-256.
  - `README.md`: what it is; building inside the superproject, with the
    external dependencies and their variables; the examples and how to build
    them (copies of the programs, checked by doc-check); the tests and the two
    own lanes; the derived parts; the link to its page. SOURCE's
    from-an-empty-directory walkthrough is rewritten for webcpp and run, as
    written, in a scratch directory; the README says what it printed.
  - `AGENTS.md`: only what is specific to trystero (the wire is Trystero's and
    never changes; the two backends and their targets; the native backend's
    exceptions, and where that is declared; the external dependencies; the
    memory relays; the own lanes; no twins, since no example runs), pointing
    to `../../AGENTS.md`.
- [ ] **Step 5: Commit in `libs/trystero`:** "trystero: moved as
  webcpp::trystero, built by webcpp's b2 without CMake". Record in the ledger
  that the root `b2 doc` stays red until Task 11 (`libraries.py` needs a page,
  C18) and that the lint reports trystero's findings until Task 9.

### Task 6: trystero's tests on lightweight_test

The plan's original Task 4, with C1 and C26.

**Files:**
- Convert SOURCE's `test/unit/{protocol,room,native_sealer,native_websocket,native_peer_connection,native_room}_test.cpp`
  into `libs/trystero/test/*_test.cpp`; move
  `test/unit/fixtures/expected.json` to
  `libs/trystero/test/fixtures/expected/expected.json` (byte-identical).
- Create: `libs/trystero/test/require.hpp` (as xactor's, in
  `webcpp::test`), `libs/trystero/test/CONVERSION.md`
- Modify: `libs/trystero/test/Jamfile`

**Interfaces:**
- `test/Jamfile` gains:

```
webcpp.run protocol : protocol_test.cpp : <testing.input-file>fixtures/expected/expected.json ;
webcpp.run room : room_test.cpp ;
webcpp.run native_sealer : native_sealer_test.cpp
  : <library>/webcpp/trystero//native <testing.input-file>fixtures/expected/expected.json : native ;
webcpp.run native_websocket : native_websocket_test.cpp : <library>/webcpp/trystero//native : native ;
webcpp.run native_peer_connection : native_peer_connection_test.cpp
  : <library>/webcpp/trystero//native : native ;
webcpp.run native_room : native_room_test.cpp : <library>/webcpp/trystero//native : native ;
```

- [ ] **Step 1: Record what the old tests check.** For each file, every
  `BOOST_AUTO_TEST_CASE` and every assertion, in `CONVERSION.md`, anchored at
  `d3f4f9a`: 32 cases (protocol 9, room 9, native_websocket 5,
  native_sealer 3, native_peer_connection 3, native_room 3), 178
  `BOOST_TEST`, 21 `BOOST_TEST_REQUIRE`, 3 `BOOST_TEST_DONT_PRINT_LOG_VALUE`
  (F25). None needs Boost.Test.
- [ ] **Step 2: Convert:** each case a `void <case>()` called from `main`,
  which returns `boost::report_errors()`; `BOOST_TEST(a == b)` becomes
  `BOOST_TEST_EQ(a, b)` where the type prints, else `BOOST_TEST(a == b)` (the
  three `DONT_PRINT_LOG_VALUE` types); `BOOST_TEST_REQUIRE(x)` in a case
  becomes `if (!BOOST_TEST(x)) { return; }`, and in a helper
  `require(BOOST_TEST(x))`; `master_test_suite().argv[1]` becomes `main`'s
  `argv[1]`. Each `io.run_for(...)` says why after `lint-run:`.
- [ ] **Step 3: Prove the equivalence:** case and assertion counts match
  `CONVERSION.md`, and three planted defects fail the same cases in the old
  suite (built by hand in a scratch directory, as F12 did) and the new one:
  1. the root topic built from `"Trystero:"` instead of `"Trystero@"`
     (protocol);
  2. a room that does not answer the password challenge (room);
  3. a sealer whose `open` skips the GCM tag check
     (`another_key_is_a_wrong_password`, native_sealer; C26).
- [ ] **Step 4: Run from scratch** in parallel lanes: native clang and gcc-15
  (all six), emscripten (protocol and room under node, F12), and the
  libstdc++ container lane (clang-18 and g++-14 on ubuntu-24.04, with
  Boost 1.92 and the container's own libsecp256k1, libdatachannel and
  OpenSSL built there, under `bin/lane-container`). Each lane's XML through
  `report.py` exits 0. No test reaches beyond 127.0.0.1: the container lane
  runs again with `docker run --network none` (the loopback stays), and every
  test passes.
- [ ] **Step 5: Commit in `libs/trystero`:** "trystero: the tests on
  lightweight_test, natively and on emscripten".

### Task 7: trystero's own lanes: the oracle and the interop tests

The plan's original Task 6, with C10, C12 and C27.

**Files:**
- Move from SOURCE: `test/package.json` and `test/package-lock.json` to
  `libs/trystero/test/oracle/` (name `webcpp-trystero-tests`; the lock
  regenerated only by `npm install --package-lock-only` after the rename, the
  three Trystero packages still at 0.26.0 with the same integrity hashes),
  `test/oracle/oracle.mjs` to `test/oracle/oracle.mjs`, `test/interop/**` to
  `test/oracle/interop/**`.
- Create: `libs/trystero/test/oracle/Jamfile`,
  `libs/trystero/test/fixtures/cases/protocol.json`
- Modify: `oracle.mjs`, `interop/run.mjs`, `interop/peer.cpp`,
  `interop/native_peer.cpp`, `interop/js.html`, `libs/trystero/AGENTS.md`

**Interfaces:**
- `test/oracle/Jamfile`:

```
import webcpp ;

webcpp.targets native emscripten ;

webcpp.original node ;
webcpp.cases protocol : oracle.mjs : ../fixtures/cases : ../fixtures/expected ;
webcpp.lane oracle : cases-protocol ;

# Trystero JS meets each C++ peer over a relay of its own on 127.0.0.1: the
# browser's peer in headless Chrome, and the native peer as a process.
webcpp.drive interop_browser : interop/peer.cpp
  : <linkflags>-sENVIRONMENT=web <linkflags>-sALLOW_MEMORY_GROWTH
  : interop/run.mjs : --browser : emscripten ;
webcpp.drive interop_native : interop/native_peer.cpp : <library>/webcpp/trystero//native
  : interop/run.mjs : --native : native ;
webcpp.lane interop : interop_browser interop_native : native emscripten ;
```

  The interop tests live beside the pinned Trystero JS, so `run.mjs` keeps
  serving `../node_modules` and the drives share the Jamfile's
  `webcpp.original` and `node-modules`.
- `oracle.mjs <cases> <output-directory>` reads its inputs from
  `<cases>/protocol.json` and writes `<output-directory>/expected.json`,
  byte-identical to today's. `protocol.json` holds exactly the values the
  script holds today:

```json
{
    "topics": [
        ["trystero-cpp-probe", "lobby", "00000000000000000000"],
        ["my-app", "room-1", "zzzzzzzzzzzzzzzzzzzz"],
        ["café ☕ app", "sala ünïcode 🎉", "AbCdEfGhIjKlMnOpQrSt"],
        ["a", "b", "c"]
    ],
    "str_to_num": ["", "a", "trystero-cpp-probe", "café ☕", "🎉🎉", "585q4w3ia403j31626l2p3k1p3kh6u2v2g5n4v"],
    "relays": ["trystero-cpp-probe", "my-app", "café ☕ app", "x", "a-much-longer-application-identifier-for-testing"],
    "relay_redundancy": 5,
    "shuffle": {"seeds": [0, 1, 1234567, 9007199254740991], "length": 29},
    "handshake": [["ch4ll3ng3", "secret", "my-app", "room-1"], ["x", "", "a", "b"]],
    "sealed": {
        "password": "secret",
        "app_id": "my-app",
        "room_id": "room-1",
        "plaintext": "v=0\r\no=- 4611731400430051336 2 IN IP4 127.0.0.1\r\n",
        "random": {"multiplier": 17, "offset": 3}
    },
    "nostr_events": {
        "pubkey": "4f355bdcb7cc0af728ef3cceb9615d90684bb5b2ca5f859ab0f0b704075871aa",
        "created_at": 1759700000,
        "contents": [
            "{\"peerId\":\"abc\",\"nonce\":\"12345678\"}",
            "quote \" backslash \\ newline \n tab \t unicode é ☕ 🎉 control \u0001",
            ""
        ]
    },
    "actions": {
        "text": ["chat", "hello from Trystero JS"],
        "json": ["drink", {"drink": "negroni", "withIce": true, "n": [1, 2.5, null]}],
        "binary": ["pic", [0, 1, 2, 253, 254, 255]],
        "text_with_metadata": ["chat", "hi", {"from": "js", "n": 7}],
        "empty_text": ["@_hsready", ""],
        "thirty_two_byte_name": ["abcdefghijklmnopqrstuvwxyz012345", "x"],
        "chunked": {"type": "file", "length": 40000, "multiplier": 7, "metadata": {"name": "big.bin"}}
    },
    "controls": {"max_receive_bytes": 30000},
    "envelope": {"app_id": "my-app", "room_id": "room-1", "frame": [2, 0, 99, 104, 97, 116], "timeout": 60000}
}
```

- `run.mjs`: `--browser <interop_peer.js>` (the directory it serves is the
  file's) and `--native <native_peer>`; Chrome starts with
  `--remote-debugging-port=0`, and the port is read from
  `<profile>/DevToolsActivePort`; every peer gets loopback ICE: the C++ peers
  an argument `ice=stun:127.0.0.1:9` that `peer.cpp` and `native_peer.cpp`
  put in `room_config::ice_servers`, and `js.html` `rtcConfig: {iceServers:
  []}` (C27). Chrome is `CHROME`, `--chrome`, or Playwright's headless shell
  on macOS; on Linux `CHROME` is required, and its absence fails naming it.
- `B2 -d0 declared-lanes` prints `trystero interop libs/trystero/test/oracle
  emscripten`, `trystero interop libs/trystero/test/oracle native` and
  `trystero oracle libs/trystero/test/oracle`.

- [ ] **Step 1: The failing check.** Write the Jamfile, then `B2
  libs/trystero/test/oracle//oracle`. Expected: FAIL (no `oracle.mjs`, no
  cases).
- [ ] **Step 2: Move,** changing only licence notices, paths, the C++ names
  of `peer.cpp` and `native_peer.cpp`, the inputs (moved into
  `protocol.json`), the ICE and the debugging port; the pages' JavaScript
  calls the same embind names. The protocol test reads
  `fixtures/expected/expected.json`.
- [ ] **Step 3: Run them and watch them pass:** `B2 -a
  libs/trystero/test/oracle//oracle`; `B2 -a --dump-tests --out-xml=n.xml
  libs/trystero/test/oracle//interop` natively and `B2 -a --dump-tests
  --out-xml=e.xml toolset=emscripten libs/trystero/test/oracle//interop`, both
  XML through `report.py` as the lanes `native.trystero.interop` and
  `emscripten.trystero.interop`, exit 0; both lanes at once, with their own
  build directories, pass. Then prove each bites: one changed byte in
  `expected.json` fails `oracle`; a wire string changed in a scratch copy
  (`"__trystero_pw"` to `"__webcpp_pw"`) fails both interop tests; the
  interop lanes pass again in the container with `--network none`.
- [ ] **Step 4: Commit in `libs/trystero`:** "trystero: the oracle on the
  shared tools, and the interop tests, as the library's own lanes".

### Task 8: trystero's allocation through std::pmr

C23 with controller ruling 2. Specification first, under the converted suites
and the interop lane.

**Files:**
- Create: `include/webcpp/trystero/json_resource.hpp`,
  `include/webcpp/trystero/handler.hpp`, `test/allocation_test.cpp`
- Modify: every header that allocates (22 of 25, F31), heaviest
  `room.hpp`, `detail/signaling.hpp`, `detail/action_wire.hpp`,
  `backend.hpp`; `test/support/memory_backend.hpp`; `test/Jamfile`;
  `libs/trystero/AGENTS.md` (a section "Allocation")

**Interfaces:**
- `room_config` gains, documented: `std::pmr::memory_resource* memory =
  std::pmr::get_default_resource();`, "the resource every allocation of the
  room goes through".
- `<webcpp/trystero/json_resource.hpp>`:

```cpp
namespace webcpp::trystero {
class json_resource final : public boost::container::pmr::memory_resource {
public:
    explicit json_resource(std::pmr::memory_resource* upstream) noexcept;
    [[nodiscard]] std::pmr::memory_resource* upstream() const noexcept;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(
        const boost::container::pmr::memory_resource& other) const noexcept override;

    std::pmr::memory_resource* upstream_;
};
}  // namespace webcpp::trystero
```

  The room's JSON storage is `boost::json::make_shared_resource<json_resource>(memory)`,
  made once when the room is constructed: a value a handler keeps may outlive
  the room. That one counted object is allocated by Boost.JSON with
  `operator new`, outside the resource; the page states it.
- `<webcpp/trystero/handler.hpp>`: `template <class Signature> class
  handler;` and `template <class R, class... Args> class handler<R(Args...)>`,
  a move-only callable: constructed with `handler(std::allocator_arg_t,
  std::pmr::polymorphic_allocator<> allocator, F&& f)`, it stores `f` inline
  when it fits in three pointers and is nothrow-movable, else through the
  allocator; `R operator()(Args...)`; `explicit operator bool() const
  noexcept`. Every `std::function` of `backend.hpp` (the handlers of
  `timer`, `websocket`, `data_channel`, `peer_connection`, `sealer`), of the
  room and of `detail::logger::sink` becomes a `handler`.
- `backend::factory`'s methods take the room's `std::pmr::memory_resource*`
  as their last parameter, and a backend allocates its own objects through it
  (`std::allocate_shared` with `std::pmr::polymorphic_allocator<>`).
- Inside the room: `std::pmr::string`, `std::pmr::vector`, `std::pmr::deque`,
  `std::pmr::map` and `std::pmr::unordered_map` over its resource; its shared
  objects with `std::allocate_shared`; every `boost::asio::post`,
  `dispatch`, timer wait and `any_completion_handler` it makes carries
  `boost::asio::bind_allocator(std::pmr::polymorphic_allocator<>(memory),
  ...)`. A user's completion handler is allocated as its own associated
  allocator says; the page shows `bind_allocator` with the room's resource.
- `detail::payload_traits` gains `std::pmr::string` and
  `std::pmr::vector<std::uint8_t>` (alias `webcpp::trystero::pmr_bytes`),
  and `decode(incoming& message, std::pmr::memory_resource* memory)` takes
  the resource for every type: the pmr types and `boost::json::value` (the
  room's JSON storage) are built in it; `std::string` and `bytes` stay as they
  are, the user's choice.
- Stated on the page, outside the resource: libdatachannel's and OpenSSL's
  own allocations, `emscripten::val`'s JavaScript heap, the JSON storage's
  counter (above), and Asio's own once-per-context allocations.

- [ ] **Step 1: The specification.** Write `AGENTS.md`'s section
  "Allocation" (the interfaces above, as rules) and record in the ledger the
  page section "Allocation" Task 11 writes from it.
- [ ] **Step 2: Write the failing test,** `test/allocation_test.cpp`,
  declared `webcpp.run allocation : allocation_test.cpp ;` (both targets):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// While two rooms over the memory relays exchange a message, every allocation
// they make goes through the resources of their configurations: the counting
// resources see them, and the global operator new, replaced here, sees none
// (doc: #trystero-allocation).

#include <webcpp/trystero.hpp>

#include "support/memory_backend.hpp"

#include <boost/asio/bind_allocator.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <string_view>
#include <vector>
#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace trystero = webcpp::trystero;

namespace {

// What the replaced operator new counts while counting is set: global and
// mutable, since an operator new can reach nothing else.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t global_allocations = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool counting = false;

void* allocate(std::size_t size, std::size_t alignment) {
    if (counting) {
        ++global_allocations;
    }
    const std::size_t rounded = (size + alignment) / alignment * alignment;
#ifdef _MSC_VER
    void* pointer = _aligned_malloc(rounded, alignment);
#else
    void* pointer = std::aligned_alloc(alignment, rounded);
#endif
    if (pointer == nullptr) {
        std::abort();
    }
    return pointer;
}

void release(void* pointer) noexcept {
#ifdef _MSC_VER
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

/** A resource that counts what it gives, from a buffer made before the exchange. */
class counting_resource final : public std::pmr::memory_resource {
public:
    std::size_t allocations = 0;

private:
    std::vector<std::byte> storage_ = std::vector<std::byte>(std::size_t{64} << 20);
    std::pmr::monotonic_buffer_resource buffer_{storage_.data(), storage_.size(),
                                                std::pmr::null_memory_resource()};

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++allocations;
        return buffer_.allocate(bytes, alignment);
    }
    void do_deallocate(void* /*pointer*/, std::size_t /*bytes*/, std::size_t /*alignment*/) override {}
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

/** Runs io in slices of 10 ms until done holds, for 5 s at most. */
template <class Predicate>
bool run_until(boost::asio::io_context& io, Predicate done) {
    for (int slice = 0; slice < 500 && !done(); ++slice) {
        io.restart();
        io.run_for(std::chrono::milliseconds(10));  // lint-run: the memory relays post to it
    }
    return done();
}

/** Two rooms exchange two payloads that make builds; the second is counted. */
template <class Payload, class Make>
void exchange_allocates_through_the_rooms_resources(Make make) {
    boost::asio::io_context io;
    trystero::test::memory_network network{io};
    counting_resource a_memory;
    counting_resource z_memory;
    const boost::json::storage_ptr json_storage =
        boost::json::make_shared_resource<trystero::json_resource>(&a_memory);
    trystero::room a(io.get_executor(), network,
                     {.app_id = "allocation", .self_id = "AAAAAAAAAAAAAAAAAAAA", .memory = &a_memory},
                     "room");
    trystero::room z(io.get_executor(), network,
                     {.app_id = "allocation", .self_id = "zzzzzzzzzzzzzzzzzzzz", .memory = &z_memory},
                     "room");
    std::size_t received = 0;
    auto from_a = a.make_action<Payload>("chat");
    z.make_action<Payload>("chat").on_message(
        [&](const Payload& /*payload*/, const trystero::message_info& /*info*/) { ++received; });
    const auto send = [&] {
        from_a.async_send(make(&a_memory, json_storage),
                          boost::asio::bind_allocator(std::pmr::polymorphic_allocator<>(&a_memory),
                                                      [](boost::system::error_code /*ec*/) {}));
    };
    // Joined, connected and one payload through, before the window opens.
    send();
    if (!BOOST_TEST(run_until(io, [&] { return received == 1; }))) {
        return;
    }
    const std::size_t before = a_memory.allocations + z_memory.allocations;
    counting = true;
    send();
    const bool arrived = run_until(io, [&] { return received == 2; });
    counting = false;
    BOOST_TEST(arrived);
    BOOST_TEST_EQ(global_allocations, 0u);
    BOOST_TEST_GT(a_memory.allocations + z_memory.allocations, before);
}

}  // namespace

void* operator new(std::size_t size) { return allocate(size, alignof(std::max_align_t)); }
void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocate(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* pointer) noexcept { release(pointer); }
void operator delete(void* pointer, std::size_t /*size*/) noexcept { release(pointer); }
void operator delete(void* pointer, std::align_val_t /*alignment*/) noexcept { release(pointer); }
void operator delete(void* pointer, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept {
    release(pointer);
}

int main() {
    exchange_allocates_through_the_rooms_resources<std::pmr::string>(
        [](std::pmr::memory_resource* memory, const boost::json::storage_ptr& /*storage*/) {
            return std::pmr::string("a text that no small string holds", memory);
        });
    exchange_allocates_through_the_rooms_resources<std::pmr::vector<std::uint8_t>>(
        [](std::pmr::memory_resource* memory, const boost::json::storage_ptr& /*storage*/) {
            return std::pmr::vector<std::uint8_t>(4096, 7, memory);
        });
    exchange_allocates_through_the_rooms_resources<boost::json::value>(
        [](std::pmr::memory_resource* /*memory*/, const boost::json::storage_ptr& storage) {
            return boost::json::value(std::string_view("a text that no small string holds"), storage);
        });
    return boost::report_errors();
}
```

  `json_resource.hpp` and `handler.hpp` are compiled alone by the existing
  glob `trystero/*.hpp` of `test/Jamfile`.
- [ ] **Step 3: Run it and watch it fail.** `B2 -a
  libs/trystero/test//allocation`. Expected: FAIL to compile (`room_config`
  has no `memory`), then, once that member exists, FAIL with
  `global_allocations` above 0.
- [ ] **Step 4: Implement** the interfaces above, header by header, until the
  test passes natively and on emscripten; `std::aligned_alloc` and the
  replaced operators are measured on emscripten first and recorded.
- [ ] **Step 5: Run every lane** of Task 6 and both own lanes of Task 7 from
  scratch: nothing else moved, and the interop lanes still meet Trystero JS.
- [ ] **Step 6: Commit in `libs/trystero`:** "trystero: every allocation of a
  room through the memory resource of its configuration".

### Task 9: trystero lint-clean

The plan's original Task 5, with C20, C21 and C22 in place.

**Files:**
- Modify: every C++ file of `libs/trystero`, its Jam files, and its
  JavaScript, HTML and AsciiDoc for the licence notice.

- [ ] **Step 1: The failing check.** `tools/lint/lint.sh --clang-format
  .local/wasi-sdk/bin/clang-format --clang-tidy
  .local/wasi-sdk/bin/clang-tidy`. Expected: FAIL; record each rule's count
  of findings in the ledger (clang-format would change 36 of SOURCE's 37 C++
  files, 709 lines over 100 columns, F27).
- [ ] **Step 2: Fix by rule, one commit each:**
  1. clang-format: SOURCE is written at 120 columns, the shared format is
     100; Jam comments at 80;
  2. clang-tidy findings, each fixed in the code, or, where the check is
     wrong for this code, a `NOLINT(<check>)` with its reason in the comment
     above (AGENTS.md chapter 6); never a check disabled for the library;
  3. the world rule: each line that names a socket, a clock, a thread or the
     entropy (`getentropy`, `RAND_bytes`), which is trystero's purpose, says
     so after `lint-world:`; each blocking `run`, `run_one` or `run_for`
     after `lint-run:`;
  4. fluent chains, `return *this`, JSON literals laid out, Doc Comment
     commands, the notices, the include boundaries.
- [ ] **Step 3: Run the lint and watch it pass,** then every lane of Task 6
  and both own lanes from scratch, to confirm no behaviour moved.
- [ ] **Step 4: Commits in `libs/trystero`:** "trystero: formatted to the
  shared style", "trystero: clean under the shared clang-tidy", "trystero:
  the world it reaches, marked", "trystero: the remaining rules of the
  lint".

### Task 10: trystero's Doc Comments, complete

The plan's original Task 7, with C15 and C16.

**Files:**
- Modify: every header under `include/webcpp/`
- Create: `libs/trystero/doc/Jamfile`, `libs/trystero/doc/mrdocs.yml`
  (presentation keys only), `libs/trystero/doc/trystero.adoc` (a skeleton)

**Interfaces:**
- `doc/Jamfile`:

```
import webcpp ;

webcpp.doc trystero : trystero.adoc ;
webcpp.reference trystero : <library>/webcpp/trystero//native <library>/webcpp//emscripten-headers ;
```

- The skeleton page: the title `= trystero: serverless WebRTC rooms for
  {cpp}`, the attributes of AGENTS.md chapter 8, and every section Task 11
  keeps, with these exact titles and anchors: `[#trystero-overview] ==
  Overview`, `[#trystero-quick-start] == Quick start`, `[#trystero-building]
  == Building`, `[#trystero-usage] == Rooms and actions`,
  `[#trystero-allocation] == Allocation`, `[#trystero-browser] == The browser
  backend`, `[#trystero-native] == The native backend`,
  `[#trystero-protocol] == The protocol`, `[#trystero-design] == Design`,
  `[#trystero-guarantees] == Guarantees`, `[#trystero-testing] == Testing`,
  `[#reference] == Reference` (`include::{reference}[leveloffset=+1]`), and
  the appendices `[#trystero-differences] == Differences from Trystero`,
  `[#trystero-roadmap] == Roadmap`, `[#trystero-acknowledgements] ==
  Acknowledgements`.

- [ ] **Step 1: The failing check.** `B2 libs/trystero/doc//reference`.
  Expected: FAIL; record the count of findings, by directory (measured on
  SOURCE before the rename: 418, core 132, `protocol/` 135, `detail/` 55,
  `native/` 34, `browser/` 62, F11).
- [ ] **Step 2: Document** every public function, class, struct, enum,
  enumerator, alias, concept, data member and macro of the core,
  `protocol/`, `native/` and `browser/`: a one-sentence brief, then details
  that say which part of Trystero's protocol or API it implements; `@param`,
  `@tparam`, `@return`, `@pre`, `@throws` (the native backend's) where they
  apply; `@see` with one of the skeleton's titles; a brief for every `detail`
  symbol. Change no behaviour: the comment-stripped diff is empty, except
  the trailing commas `readability-trailing-comma` asks for once enumerators
  have comments.
- [ ] **Step 3: Run it and watch it pass.** `B2 -a
  libs/trystero/doc//reference` exits 0 and `doc_comments.py` is clean; the
  reference documents all five directories.
- [ ] **Step 4: Run the tests** on one native lane and the emscripten lane;
  the lint is clean.
- [ ] **Step 5: Commit in `libs/trystero`:** "trystero: every public symbol
  has a Doc Comment, and MrDocs builds its reference strict".

### Task 11: trystero's page on the shared doc toolchain

The plan's original Task 8, with C17 and C29.

**Files:**
- Move and adapt from SOURCE: `doc/trystero.adoc` and `doc/trystero/*.adoc`
  into `libs/trystero/doc/` (one file per section of Task 10's skeleton).
- Not moved: `doc/trystero/reference.adoc` (MrDocs replaces it) and
  `copyright.adoc` (the notice is `LICENSE_1_0.txt`).
- Modify in the superproject: `tools/doc/doc-check.py`, `tools/doc/doc.jam`
  and `doc_check_test.py` (`{library-root}`), `AGENTS.md` chapter 8
- Modify in `libs/trystero`: `build.jam` and `test/Jamfile` (tag regions
  only)

**Interfaces:**
- `{library-root}`: `webcpp.doc` gives a page its library's directory, and
  doc-check (`--library-root <dir>`) accepts
  `include::{library-root}/<path>[tag=<name>]` when `<path>` is a file the
  library's own git tracks and holds that tag, with `{webcpp-root}`'s
  refusals (an untracked file, a missing tag, a whole file, lines by number).

- [ ] **Step 1: The failing check.** Add the `{library-root}` cases to
  `doc_check_test.py` first (a tagged include of a tracked library file
  passes; an untracked file or a missing tag fails), then `B2
  libs/trystero/doc`. Expected: FAIL (the skeleton, then doc-check's
  findings: listings that are no compiled example, `{include}` paths into
  headers, the rouge highlighter, the private docinfo).
- [ ] **Step 2: Write the page,** one file per section:
  - **Overview, quick start** (browser and native, each listing an include of
    the example by tag), **building** (inside webcpp: the dependency lines of
    `build.jam` and the test Jamfile's lines through `{library-root}`, the
    emscripten toolset through
    `include::{webcpp-root}/tools/ci/emsdk.jam[tag=emscripten]`, the
    variables), **rooms and actions**, **allocation** (Task 8's rules, the
    `bind_allocator` listing from `allocation_test.cpp` by tag, and what
    stays outside the resource), **the browser backend**, **the native
    backend** (its exceptions and why), **the protocol**, **design**.
  - **Guarantees,** numbered and anchored `trystero-invariant-1` onward, each
    naming the test that holds it: the wire is Trystero 0.26's (interop);
    every protocol value equals Trystero JS's (oracle, protocol); a peer with
    another password never joins (room); payloads over the chunk size travel
    in chunks both ways (room); every allocation of a room goes through its
    resource (allocation); no unit test reaches beyond the loopback (the
    container lane).
  - **Testing:** the unit tests on two targets (counts from `{n-tests}`,
    `{n-tests-native}`, `{n-tests-emscripten}`, `{n-linked}`), the memory
    relays, the `oracle` and `interop` lanes with their commands, and the
    matrix.
  - **Differences from Trystero:** each feature the port leaves out (audio
    and video tracks, among others) as not ported, with the reason, and no
    `diff_*` example (C29); a behaviour that differs is reported to the owner
    in the ledger, since no example of trystero runs and none can have a twin.
  - **Roadmap, acknowledgements** (Trystero, libsecp256k1, libdatachannel),
    naming `LICENSE-TRYSTERO.txt`.
  - Every listing includes a compiled example or a tracked file by tag; no
    output is shown that no `.expected` holds (the link-only examples show
    none); the shared attributes replace SOURCE's (`hljs-static`, the shared
    docinfo, `webfonts!`), `:sectnums:` goes; apostrophes straight.
- [ ] **Step 3: Run it and watch it pass.** `B2 -a libs/trystero/doc` exits 0
  with doc-check `--complete` and the rendered check; then `B2 -a doc` (the
  index lists trystero, porting Trystero 0.26.0). Open the page at each width
  from 320 to 1440 px in steps of 80 (WebKit): no horizontal overflow outside
  code blocks and tables. Record the screenshot paths.
- [ ] **Step 4: Commit** in the superproject: "tools/doc: a page includes its
  library's tracked files by tag"; in `libs/trystero`: "trystero: its
  documentation page on webcpp's doc toolchain, with the MrDocs reference".

### Task 12: AGENTS.md, README, the index, and CI with the external dependencies

The plan's original Task 9, with C11, C28 and C32.

**Files:**
- Modify: `~/dev/webcpp/AGENTS.md`, `README.md`,
  `.github/workflows/library.yml`, `tools/ci/matrix.py`, `matrix_test.py`,
  `actions_test.py`
- Create: `tools/ci/actions/secp256k1/{action.yml,install.sh}`,
  `tools/ci/actions/libdatachannel/{action.yml,install.sh}`,
  `tools/ci/actions/openssl/{action.yml,install.sh}`,
  `libs/trystero/.github/workflows/ci.yml`

**Interfaces:**
- `matrix.py`: `EXTERNAL_DEPENDENCIES = {'trystero': ('secp256k1',
  'libdatachannel', 'openssl')}`, its comment "until webcpp builds them from
  third_party/ (AGENTS.md chapter 13)"; a lane or own-lane entry gains
  `external: true` when one of its libraries is there; `matrix.py external
  [--library <name>]` prints `true` or `false` for the docs and lint jobs.
- **The actions,** each with its script, installing where the build looks
  (`.local/`) and exporting its variables to `$GITHUB_ENV`, cached per
  runner, on Linux, macOS and Windows:
  - `secp256k1` builds libsecp256k1 at `6e2c8bc` (v0.8.0, its archive checked
    by SHA-256) with CMake, static, schnorrsig and extrakeys on, natively
    and, with input `emscripten: true`, with `emcmake`; exports
    `SECP256K1_ROOT` and `SECP256K1_EMSCRIPTEN_ROOT`;
  - `libdatachannel` clones v0.24.6 with its submodules, checks `git
    rev-parse HEAD` against the full commit read with `gh api`, builds it
    shared with `NO_WEBSOCKET`, `NO_EXAMPLES`, `NO_TESTS` on and `NO_MEDIA` off
    (P3) against the runner's OpenSSL; exports `LIBDATACHANNEL_ROOT`;
  - `openssl` exports `OPENSSL_ROOT`: the runner's own on Linux and macOS, and
    on Windows the runner's preinstalled OpenSSL 3 if it has one, else a pinned
    installer checked by its SHA-256.
- **library.yml:** the dependency actions run, under `if: matrix.external`,
  in the lanes and own-lanes jobs, and under `needs.plan.outputs.external ==
  'true'` in the docs and lint jobs; the emscripten lane passes `emscripten:
  true` to `secp256k1`; an interop own lane sets `CHROME=/usr/bin/google-chrome`
  (ubuntu-24.04).
- **trystero's ci.yml:** `uses:
  webcpporg/webcpp/.github/workflows/library.yml@main` with `library:
  trystero`, as xactor's.
- **AGENTS.md** (C28): chapter 1's libraries table (trystero's row: "a port
  of Trystero 0.26.0, serverless WebRTC rooms | native, emscripten; its own
  lanes `oracle` and `interop` | a submodule at `libs/trystero`; born with the
  allocation rule through `std::pmr`"), the layout (`tools/ci/emsdk.jam`,
  `tools/ci/actions/{emsdk,secp256k1,libdatachannel,openssl}`, `tools/drive/`,
  the fixture `browser_demo`), the prerequisites (Emscripten 6.0.11 and Node
  for emscripten; libsecp256k1, libdatachannel and OpenSSL for trystero, with
  their variables), machine-local setup (`.local/emsdk`, `EM_CACHE`,
  `.local/env.sh`); chapter 9's lanes table (the emscripten row), CI (the
  external dependencies, Chrome); chapter 13 (trystero leaves the roadmap;
  allocators: trystero born with `std::pmr`).

- [ ] **Step 1:** `actionlint` on the changed workflows, and the new
  `matrix_test.py` and `actions_test.py` cases (an entry's `external`; each
  script's refusals: an unpinned digest, a runner it has no build for). Watch
  them fail.
- [ ] **Step 2: Write** them. Pin every third-party action and download by
  full commit SHA or SHA-256, read with `gh api`; never invent one.
- [ ] **Step 3: Measure on Windows before publishing** (C32): in a
  `workflow_dispatch` run of a scratch branch of the controller's choosing,
  or by reading the runner image's manifest, record OpenSSL's MSVC library
  names (`libssl.lib`, `libcrypto.lib`), libsecp256k1's CMake library name on
  MSVC, and that a native test finds libdatachannel's DLL on `PATH`; adjust
  `build.jam` and the actions so that the MSVC lanes can link, or record the
  cause for the owner.
- [ ] **Step 4: Run** `actionlint` (clean), every `tools/**/*_test.py`, `B2
  -a doc`, the lint (clean), and the plan job's scripts locally: `matrix.py
  plan --library trystero` emits its native lanes and the emscripten lane, and
  `own-lanes` emits `oracle`, `native.trystero.interop` and
  `emscripten.trystero.interop`.
- [ ] **Step 5: Commit** in the superproject: "AGENTS.md and CI: emscripten,
  driven and link-only programs, and external dependencies until webcpp
  builds them"; in `libs/trystero`: "trystero: CI through the superproject's
  library workflow".

### Task 13: pratt on emscripten

Approved when pratt was designed: pratt joins emscripten "once stage 4 brings
its lane" (`PRATT_DESIGN.md` decision 1, C24).

**Files:**
- Modify in `libs/pratt`: `test/Jamfile`, `example/Jamfile`
  (`webcpp.targets native emscripten wasip2 wasip3 ;`), `README.md` 25-26 and
  160-163, `AGENTS.md` (the depth measurements, 40-46), `doc/testing.adoc`
  (an emscripten row and command), `doc/introduction.adoc` if it lists
  targets
- Modify in the superproject: `AGENTS.md` chapter 1's pratt row

- [ ] **Step 1: The failing check.** Declare emscripten, then `B2 -a
  toolset=emscripten libs/pratt/test libs/pratt/example`. Expected: either
  every test and example passes (then the check is the page's counts: `B2
  libs/pratt/doc` fails until `{n-tests-emscripten}` is shown), or a program
  fails, which is recorded and investigated before anything else.
- [ ] **Step 2: Measure,** and record in the ledger and pratt's `AGENTS.md`:
  that Boost.Decimal builds with em++; that the examples with a `.input`
  read it from standard input under node; the depth the depth tests reach
  under Emscripten's default stack, as `AGENTS.md` 40-46 records it for
  wasip2. If a program cannot pass on emscripten without a change of pratt's
  code or a link flag of its own, stop and report to the controller: a target
  is never declared without a lane that proves it.
- [ ] **Step 3: The texts.** README "It runs natively, on emscripten, on
  wasm32-wasip2 and on wasm32-wasip3" and the emscripten command;
  `testing.adoc`'s row "| emscripten | b2's toolset `emscripten`, wasm32, run
  by node: {n-tests-emscripten} tests and {n-examples-emscripten} examples";
  AGENTS.md chapter 1: pratt's targets "native, emscripten, wasip2, wasip3".
- [ ] **Step 4: Run every lane of pratt from scratch,** in parallel: native
  clang and gcc-15, emscripten, wasip2, wasip3; each XML through `report.py`,
  exit 0; `B2 -a libs/pratt/doc` and `B2 -a doc`; `matrix.py plan --library
  pratt` emits the emscripten lane; the lint clean.
- [ ] **Step 5: Commit** in `libs/pratt`: "pratt: emscripten, its tests and
  examples under node"; in the superproject, with the bump: "libs/pratt:
  emscripten".

### Task 14: Publish (controller)

Needs no owner approval beyond the spec: the owner approved public
repositories in `webcpporg` (spec section 1), and `webcpporg/trystero` exists
(F20). Runners are scarce: one repository's CI at a time.

- [ ] **Step 1: Check the repository.** `gh repo view webcpporg/trystero
  --json isEmpty,visibility` prints `"isEmpty":true` and
  `"visibility":"PUBLIC"`; otherwise stop and report.
- [ ] **Step 2: Push the libraries,** in order xactor, xstate, pratt, wasi:
  `git -C ~/dev/webcpp/libs/<name> push origin main`; then trystero: `git -C
  ~/dev/webcpp/libs/trystero remote add origin
  git@github.com:webcpporg/trystero.git && git -C ~/dev/webcpp/libs/trystero
  push -u origin main`. Each push starts that library's CI against the
  superproject's old `main`: cancel each such run (`gh run list --repo
  webcpporg/<name> --limit 1 --json databaseId`, `gh run cancel <id> --repo
  webcpporg/<name>`).
- [ ] **Step 3: The superproject.** `git submodule add ../trystero.git
  libs/trystero`, commit "libs/trystero: the submodule", and push `main`
  (the bumps of xactor, xstate, pratt and wasi are already committed). Watch
  its CI until green: every native lane (GCC 14 and 15, Clang 18 and 22,
  Apple Clang, MSVC 14.3 and 14.5), wasip2, wasip3, emscripten, the docs, the
  lint shards, the tools, every own lane (`oracle` of xstate and trystero,
  wasi's `http`, trystero's `interop` on native and emscripten), and the
  report. Fix what fails, in the right repository, with a test. A lane that
  cannot be made green within the spec (for example libdatachannel on a
  Windows runner) is reported to the owner with its failure, never removed
  from the matrix.
- [ ] **Step 4: Each library's CI,** one at a time, in the order of Step 2:
  `gh workflow run ci.yml --repo webcpporg/<name> --ref main`, watched until
  green before the next.
- [ ] **Step 5: Open the published Pages:** the index lists trystero, its page
  shows the reference, wasi's page shows "Exceptions on wasip2 and wasip3",
  and the matrix has a trystero row on native and emscripten and a pratt
  column for emscripten.
- [ ] **Step 6:** Ledger the stage as complete, with the open questions for
  the owner: the exceptions rule (C3), whether wasi should catch an exception
  that escapes the main, and any difference from Trystero recorded in Task 11.
  Stage 5 (archiving) waits for the owner's go.
