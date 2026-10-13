---
title: "Boost regression matrix for webcpp"
tags: [testing, boost, regression, webcpp]
status: active
created: 2026-10-07
---

# Boost regression matrix for webcpp

How Boost produces its regression matrix
(<https://regression.boost.org/master/developer/summary.html>), whether a
small non-Boost collection can produce the same kind of page, and a spike
proving that Boost.Core's `lightweight_test` feeds it, natively and on
wasm32-wasip2.

Everything was done on 2026-10-07. A claim is either cited (`[Sn]`, list at
the end) or measured (the command and its output are named). Code facts cite
the file and line at a fixed commit. The work tree is
`~/.claude/jobs/fcf3a902/tmp/regression/` (`WORK` below):

| Path under `WORK` | What it is |
| --- | --- |
| `regression-repo/` | boostorg/regression at `f085a31` (2026-04-20), shallow clone |
| `boost-root/` | boostorg/boost at `boost-1.92.0` plus depinst, only to build the tools |
| `spike/` | the spike project (a git repo, 5 commits) |
| `spike/report/boost_report/site/` | the Boost-style matrix, unzipped from `report.zip` |
| `spike/report/library_status/` | library_status pages |
| `alt/matrix.html` | the matrix from the alternative script |
| `shots/*.png` | renders of the pages (`qlmanage -t`) |
| `run-tests.sh`, `run-library-status.sh`, `run-boost-report.sh`, `collect.py`, `b2_matrix.py`, `show_table.py` | the pipeline, as run |

Host: macOS (Darwin 27.0.0, arm64), Apple clang 21.0.0, Homebrew GCC 15.2.0,
wasi-sdk 34 (clang 23), wasmtime 47.0.3, Python 3.14.7, no Python 2 on PATH.
Boost headers came from `<private>/xstate-cpp/deps/boost`
(boost-1.92.0, read only) and its `b2` (B2 5.5.3). Every b2 run was prefixed
with `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH`. Nothing was
written there: `find deps/boost -newermt "2026-10-07 00:00"` printed 0 files
and `git status --short` in it printed nothing, after all runs.

## Recommendation

1. **Unit tests: `lightweight_test` first.** A test is a plain `main()` with
   `BOOST_TEST*` macros returning `boost::report_errors()`, declared with b2's
   own `run` rule. Measured: it compiles and reports correctly with clang 21,
   GCC 15, both with `-fno-exceptions -fno-rtti`, and on wasm32-wasip2 under
   wasmtime (where Boost.Test 1.92 does not compile, `XSTATE_CPP_BOOST_B2.md`
   4.2 attempt 15). Use Boost.Test only where its features earn it, and give
   such a target `<toolset>clang-wasi:<build>no` (or the webcpp equivalent).
2. **The matrix does not care about the framework.** Pass or fail comes from
   b2's actions and the program's exit status, never from parsing the test's
   output (section 2). A lightweight_test program and a Boost.Test program
   land in the same cells (measured).
3. **Generate the report with a small webcpp-owned script that reads b2's
   `--out-xml`**, one file per CI lane, merged in a final job and published
   to GitHub Pages. The measured script is
   `BOOST_REGRESSION_MATRIX_B2_MATRIX.py` (167 lines, Python 3 stdlib only,
   section 5); it writes Boost's layout (a library by toolset summary, then
   a test by toolset table per library, Boost's `r`/`cf` types and
   `pass`/`comp`/`link`/`run` labels). Two rules for its CI use: take the
   verdict from the script's exit status, because `--out-xml` makes b2 exit 0
   even when tests fail (measured); and build from scratch, because b2
   records only the actions it ran.
4. **Do not adopt boostorg/regression's tool chain as is.** It works for a
   non-Boost collection laid out as `libs/<name>/test` (webcpp's planned
   layout, `PLANS/WEBCPP_DESIGN.md` section 2), and `boost_report` renders a
   page identical in structure and style to regression.boost.org (measured).
   But it needs Python 2 or a rewrite of its collect step, a Boost source
   checkout with five compiled libraries to build the tools, the
   workarounds of section 6.6, and post-processing of hard-coded boost.org and
   github.com/boostorg links; its runner scripts are Python 2 only and its
   C++ was last changed in January 2024. Keep it as the fallback if the exact
   Boost page (developer and user views, release view, expected-failure
   markup with notes) is ever required; section 6.4 is the recipe that
   worked. `library_status` is not usable: it mislabels columns as soon as
   one test has requirements of its own (measured, section 6.5).

## 1. How Boost's matrix is made

### 1.1 The pipeline and its tools

| Tool | Language, status | Input | Output |
| --- | --- | --- | --- |
| `run.py` | Python 2 (`print '...'`); the documented requirement is "Python (2.3 ≤ version < 3.0)" [S2] | command line (`--runner`, `--toolsets`, `--tag`, `--bjam-options`, `--libraries`) [S2] | downloads `regression.py`, `process_jam_log.py`, `collect_and_upload_logs.py` from boostorg/regression `develop` and calls `regression.runner` (`testing/src/run.py`) |
| `regression.py` | Python 2 | a Boost checkout it clones itself | runs b2, then process_jam_log, then collect and upload (below) |
| `process_jam_log` | C++ (`common/src/process_jam_log.cpp`, 927 lines); built by `regression.py setup` | b2's text log, which must contain `--dump-tests` lines | one `test_log.xml` per test target directory |
| `process_jam_log.py` | Python, the `--pjl-toolset=python` path | b2's `--out-xml` file | a runner XML; compiles on Python 3 but fails at run time, `AttributeError: 'map' object has no attribute 'append'` at `testing/src/process_jam_log.py:135` (measured) |
| `collect_and_upload_logs.py` | Python 2 (`import xmlrpclib`, `os.path.walk`, lines 16, 108) | every `test_log.xml` under the results directory | `<runner>.xml` (one `<test-run>`), zipped, uploaded by FTP |
| `library_status` | C++, local tool for one library [S6] | `test_log.xml` files and the `bin.v2` directory tree | `library_status.html` and a links page |
| `boost_report` | C++ (`reports/src`, Steven Watanabe 2013); replaced the XSLT reports: "Now using a C++ program that is much faster than the old XSLT process" [S7]. No `.xsl` file is left in the repository (measured, `find -name "*.xsl*"`) | runner XML files, expected results, failures markup, a comment file | `report.zip`: `developer/` and `user/` summaries, per-library pages, per-test output pages, issues, runners (measured, 55 files) |
| `boost_wide_report.py` | Python 3 since 2026-04-09 (PR #66) [S9] | runner zips downloaded from `results.boost.org` by FTP | calls `boost_report` with the downloaded XML files |
| `ci_build_results_all.sh` | shell, on the branch `ci/test-reports`, run by CircleCI in the image `cppalliance/boost_regression_docker:1` [S8] | a fresh Boost `master` and b2 `develop` | builds `boost_report`, runs `boost_wide_report.py --reports=dd,ds,i,n` for `develop` and `master`, rsyncs `report.zip` to `regression.boost.io` and `original.boost.org`, unzips it there [S8] |
| `build_log.py` | Python 2 (`from __builtin__ import exit`) | b2's `--out-xml` | a console report, used by `ci/src/ci_boost_library_test.py` (2017) |

So `summary.html` today is made like this (sources: `testing/src/regression.py`
lines 385-441 and [S8]):

1. Each volunteer runner runs `python run.py --runner=... --toolsets=... --tag=develop` [S2].
2. `regression.py` runs, from `<boost-root>/status`:
   `b2 -d2 preserve-test-targets=off --dump-tests <bjam-options> "--build-dir=<results>" >> bjam.log 2>&1`
   (line 387).
3. `process_jam_log --locate-root <results> --input-file bjam.log` writes a
   `test_log.xml` beside every test's targets (line 436).
4. `collect_logs` wraps all of them in `<test-run runner= platform= tag= timestamp= source= run-type= revision=>`
   with a `<comment>`, zips it and uploads it to the FTP server.
5. CircleCI (`ci/test-reports`) downloads every runner's zip, runs
   `boost_report`, and rsyncs `report.zip` to `regression.boost.io`
   (unzipped into `/var/www/html/regression/<branch>`) and to
   `original.boost.org` [S8]; the pages are served at
   `regression.boost.org/<branch>/developer/summary.html` [S4].

### 1.2 What the published matrix shows (measured on the live pages)

- `master/developer/summary.html` is a frameset: `toc.html` (190 px) and
  `summary_.html`. `summary_.html` (1.3 MB) says "Report Time: Wed, 30 Sep
  2026 12:39:22 +0000" and "Unusable: 363 | Regressions: 0 | New failures:
  9906"; `develop` says 12:40:05 and 10498 new failures.
- Columns: 49 toolset columns under 49 runners (e.g.
  `teeks99-dkr-mc15-20`), grouped by platform (44 Linux, 5 Windows). The
  header rows are platform, runner (a link to its comment page), revision
  (`rev 105f31`), run timestamp, toolset (`clang-linux-17~c++20`), repeated
  at the bottom.
- Rows: one per library or sub-library (`algorithm`, `algorithm/minmax`),
  169 on master, 155 on develop, the name repeated at both ends.
- Cells, with the legend verbatim: `OK` "All expected tests pass."; `comp`
  "There are some failures on the newly added tests/compiler(s). The most
  significant is compilation failure."; likewise `link`, `run`, `fail`
  (other), `cerr` (compiler error), `time` (compile time limit), `file`
  (file too big); `broken` "Tests that the library author expects to pass
  are currently failing."; `n/a` "The library author marked it as unusable
  on particular platform/toolset."; `i` "An incremental run." [S4]
- A library cell links to the library page (`master/developer/json_.html`):
  rows are tests, a test-type column (`r`, `c`, `cf`, `l`, `lf`, `rf`), the
  same toolset columns, and cells `pass`, `fail`, `comp`, empty for missing
  [S4]. A failing cell links to a page with that test's compile, link and
  run output.
- [S3] describes the same page: "The columns represent different test
  runners ... and the rows represent individual libraries."

## 2. Where pass or fail comes from

**From b2, never from the test framework.** Measured and read in the code:

- b2's testing rules (`tools/build/src/tools/testing.jam` at `05b6b638b5`):
  `run` compiles, links and runs through `actions capture-output` (line 663),
  which writes the program's stdout and stderr to `<name>.output`, appends
  `EXIT STATUS: <n>` (line 669) and exits with that status; when every step
  succeeds, `**passed**` writes `passed` into `<name>.test` (rule at 425).
  `compile-fail` and friends go through `expect-failure` (line 410): a
  failed compile writes a `(failed-as-expected)` marker and `**passed**`
  follows. [S10] documents the same: "All rules described in this section,
  if executed successfully, create a special manifest file to indicate that
  the test passed ... target-name.test", and the output in
  "target-name.output".
- b2's exit status is 1 when any test failed (measured, every run in 6.2).
- `process_jam_log` turns the log into `<compile>`, `<link>`, `<run>`
  elements with `result="succeed"` or `"fail"`: an action line
  (`clang-darwin.compile.c++ <target>`, `gcc.compile.c++`, `.link`,
  `testing.capture-output`) opens an action, and `...failed <action>` or
  `**passed**` closes it; the `run` content is read from the `.output` file.
- `boost_report` (`reports/src/add_expected_results.cpp` lines 220-250):
  a test passes when no action failed; the category of a failure is the
  first failed action in the order compile, link, run. Only the compile
  output is searched, by regex, for `Internal compiler error`, `second time
  limit exceeded`, `File too big` (the `cerr`, `time`, `file` categories) and
  for `\bwarning\b` on a pass. The test program's own output is displayed on
  the output page and never parsed. Measured: lightweight_test's
  `1 error detected.` and exit status 1 became `run`.

**A `test_log.xml`** (measured, `spike/bin.v2/libs/demo/test/fail.test/clang-darwin-21/debug/cxxstd-20-iso/test_log.xml`,
from a `-d2` log; with `-d1` the compile and link elements are empty):

```xml
<test-log library="demo" revision="8bb136" test-name="fail" test-type="run" test-program="libs/demo/test/fail.cpp" target-directory="bin.v2/libs/demo/test/fail.test/clang-darwin-21/debug/cxxstd-20-iso" toolset="clang-darwin-21" show-run-output="false">
<compile result="succeed" timestamp="2026-10-07 06:58:29 UTC">
"clang++" -std=c++20 -fPIC -O0 ... -c -o ".../fail.o" "fail.cpp"
</compile>
<link result="succeed" timestamp="2026-10-07 06:58:29 UTC">
"clang++" -o ".../fail" ".../fail.o" -fPIC -std=c++20 -g
</link>
<run result="fail" timestamp="2026-10-07 06:58:29 UTC">
fail.cpp(7): test 'demo::add(40, 2) == 41' ('42' == '41') failed in function 'int main()'
1 error detected.

EXIT STATUS: 1
</run>
</test-log>
```

`revision` comes from `git rev-parse --short=6 HEAD`, which process_jam_log
runs in the test's source directory, leaving a `.short-sha` file there
(`common/src/process_jam_log.cpp:358`, measured). The timestamps are the
time process_jam_log ran, not the time of the action.

**The b2 options and conditions it needs** (each measured):

| Option or condition | Needed? | What happens without it |
| --- | --- | --- |
| `--dump-tests` | yes | prints `boost-test(RUN) "demo/pass" : "libs/demo/test/pass.cpp"` per test (testing.jam:322; [S10]). Without it `test-type=""`, `test-program=""`, and boost_report's summary has no row at all |
| tests under `libs/<name>/test` | yes | b2 names the library from `/(tools\|libs)/(.*)/(test\|example)` (testing.jam:238-254). A flat `test/` dir gives `library=""` and test names that are absolute paths; library_status printed an empty table, boost_report an empty summary (`WORK/flat/`) |
| target paths that start with `../` or are absolute | yes | process_jam_log strips `locate_root.size()+1` characters from a relative path (`common/src/process_jam_log.cpp:146`). Run from the root, it printed `can't open output file: .../rootrun/ebug/cxxstd-20-iso/test_log.xml`, wrote nothing and exited 0 (`WORK/rootrun/`). Boost runs from `status/`; an absolute `--build-dir` also works (its directory is `<build-dir>/<project name>/<Jamroot build-dir>` [S11], measured) |
| `import testing` in the directory b2 is run from | if `preserve-test-targets` is on the command line | `error: unknown feature "<preserve-test-targets>"` |
| `-d2` (or `-d+2`) | no | adds the command lines to the log. boost_report is unaffected; library_status shows every passing test as `Warn`, because any compile text counts as a warning there |
| `preserve-test-targets=off` | no | deletes the objects and executables of passing tests ([S10]) |
| `testing.capture-output` | not an option | the name of the run action that process_jam_log recognises |

## 3. A local or CI matrix for one library or a small collection

**`library_status`** [S6]: run from `libs/<name>/test`, it first runs
process_jam_log on its input (standard input, or `--input-file`, or a b2 it
starts itself with `--b2 ...`), then walks the `bin.v2` tree: rows are the
`*.test` directories, columns are the directory tree below them (toolset,
variant, then each non-default feature as a nested header row), cells
`Pass`, `Warn` or `Fail`, each linked to a page with the compiler, linker
and run output. Measured defects (section 6.5):

- columns are mislabeled as soon as one test has requirements of its own
  (`pass_noexcept`, `<exception-handling>off <rtti>off`): the header has five
  columns, every row three cells;
- a test b2 did not build (`<build>no`) shows a red `Fail`; the
  `missing_residue_msg` "Missing" of its documentation is defined (line 62)
  and never used;
- a passing `compile-fail` test shows `Warn`, because `do_cell` hard-codes
  `test_type("unknown")` (line 406) and the compile-fail exception never
  applies; `--no-warn` turns it into `Pass`, and also hides real warnings;
- running the same toolset twice with different command-line features
  (`exception-handling=off`) puts files and subdirectories in one directory
  and aborts: `exception(205): std::string: invalid bin directory structure`
  (line 151);
- `--b2` mode stops at b2's exit status (line 1014): with one failing test,
  b2 exits 1 and no page is written at all;
- without `--input-file`, its process_jam_log step reads standard input
  (read in the code; every measured run redirected it).

Robert Ramey's fork (<https://github.com/robertramey/library_status>, last
commit 2020-06-16, "Corrected header display for complex combinations of
build features" 2017-11-27 [S12]) renders those columns correctly (measured,
`spike/report/ramey/demo.html`), after one fix: it does not compile against
Boost 1.92 (`no member named 'directory_iterator' in namespace
'boost::filesystem'`; its process_jam_log includes the removed
`boost/filesystem/convenience.hpp`); `-include boost/filesystem/directory.hpp`
fixes its library_status. Its columns are feature combinations, not
toolsets.

**`boost_report`** is the tool that produces Boost's matrix, and it works on
a non-Boost collection (measured, 6.4): the summary has libraries by rows,
one column per runner and toolset; each library page has tests by rows and
the same columns. One runner per CI lane maps naturally onto GitHub
Actions jobs. What it takes:

- the `libs/<name>/test` layout, b2 run from a subdirectory with
  `--dump-tests`, process_jam_log per log (with the workarounds of 6.6);
- a Python 3 replacement for `collect_logs` (42 lines, 6.4);
- `boost_report --expected empty_expected_results.xml --markup <markup> --comment <html> --tag <branch> -rds -rdd -rus -rud -rl -ri -rn --css master.css <runner>.xml...`,
  then `unzip report.zip`;
- rewriting or patching the hard-coded links: `https://github.com/boostorg/<library>/blob/<tag>/...`
  for every test (`reports/src/result_page.cpp:217`),
  `http://www.boost.org/libs/<library>` (467), the Boost logo, titles
  "Boost regression", and `mailto:boost-testing@lists.boost.org`.

**Publishing.** The output is static HTML (frames for the summary), so it
can be published to GitHub Pages with `actions/upload-pages-artifact` and
`actions/deploy-pages`, which need `pages: write` and `id-token: write`
[S13]. Not measured in GitHub Actions here: the jobs below are a sketch.

```yaml
# Sketch, not run: one lane per toolset, one report job.
jobs:
  test:
    strategy: { matrix: { lane: [gcc-15, clang-22, wasip2] } }
    steps:
      - run: b2 -j4 --dump-tests --out-xml=lane.xml toolset=... test || true
      - uses: actions/upload-artifact@v4
        with: { name: "lane-${{ matrix.lane }}", path: lane.xml }
  report:
    needs: test
    permissions: { pages: write, id-token: write }
    steps:
      - uses: actions/download-artifact@v4
      - run: python3 tools/b2_matrix.py --title webcpp --output site/index.html lane-*/lane.xml
      - uses: actions/upload-pages-artifact@v3
        with: { path: site }
      - uses: actions/deploy-pages@v4
```

## 4. Status of the tool chain

**Maintained?** Barely, and only for Boost's own server.

- Commits on `develop` (measured, `git log`): 2026-04 four commits by
  sdarwin (C++ Alliance), all in `reports/src/boost_wide_report.py` and its
  utilities (Python 3 port, FTP retry, runner filter) and a Docker image;
  before that 2024-01-15 (Peter Dimov, "Fix uses of deprecated Filesystem
  APIs") and 2023-11-03. The C++ of process_jam_log and library_status was
  last changed on 2024-01-15, before that on 2018-10-24 (`git log --
  library_status/src common/src`). The repository has 11 stars, 15 open
  issues and pull requests, and was last pushed 2026-04-19 (`gh api`).
- The report job itself is alive: CircleCI on `ci/test-reports` got a
  Python 3 port on 2026-04-10 (PR #67) and uploads to `regression.boost.io`
  since 2024 (PRs #58, #61) [S9]; the live summary is dated 2026-09-30 [S4].
- Issue #68 (2026-04-21) [S9]: "the scarcity of attention given to
  regression testing leaves the regression matrices essentially permanently
  neglected"; runners are filtered by name in `boost_wide_report.py`, so a
  renamed runner silently stops reporting.
- The runner side (`run.py`, `regression.py`, `collect_and_upload_logs.py`,
  `build_log.py`) is Python 2 only (measured, `python3 -m py_compile`
  fails on each with "Missing parentheses in call to 'print'"), as the
  Boost documentation still says [S2].
- Boost libraries' own CI is Boost.CI (GitHub Actions and others) [S14],
  which reports through the CI service, not through this matrix.

**Builds against Boost 1.92 with b2 5.x?** Yes, measured. With
`deps/boost/b2` (B2 5.5.3) and `WORK/boost-root` (boost-1.92.0, 74
submodules after depinst of filesystem, date_time, regex, program_options,
iostreams, test, foreach, property_tree, crc, format, variant; 280 MB):

```sh
cd WORK/boost-root && b2 -d0 -j8 headers                         # 1.1 s
cd WORK/regression-repo && b2 -j8 --boost-root=../boost-root build//install
```

The first build produced `process_jam_log` and `library_status` and failed
on `boost_report`: `reports/src/zip.hpp:25:10: fatal error: 'boost/crc.hpp'
file not found`, because my partial checkout lacked Boost.CRC (the
repository's own CI initialises every submodule [S15]). After adding crc,
format and variant, the second build (3.7 s) staged all three in
`regression-repo/stage/bin/`: `boost_report` 1075632 bytes, `library_status`
334784, `process_jam_log` 244432. Warnings only: `tmpnam` deprecated
(library_status.cpp:1005), `-Wignored-reference-qualifiers`, one unused
function. The Filesystem library (and for boost_report Date_Time, Regex,
Program_Options, Iostreams) is compiled, linked statically, into
`boost-root/bin.v2`. Building against an installed Boost instead of a source
tree would need Jamfiles of one's own: not measured.

**Dependencies:** a Boost source tree (the Jamfiles use
`use-project /boost : $(BOOST_ROOT)`); a C++11 compiler; Python 2 for the
runner scripts, Python 3 for `boost_wide_report.py`; FTP, rsync and ssh for
Boost's upload. No xsltproc: the XSLT reports are gone [S7].

**Non-Boost users:** none found. `gh search code process_jam_log` and
`gh search code "library_status --compiler"` returned only boostorg/regression,
Robert Ramey's fork and vendored copies of old Boost tarballs
(`boost_1_55_0/tools/regression/src/smoke.py` and the like); searches for
workflow files using the tools returned nothing.

## 5. The alternative: a small script over b2's `--out-xml`

b2's manual says the processing is left to others: "It is possible to
process the list of tests, B2 output and the presence/absence of the *.test
files created when test passes into human-readable status table of tests.
Such processing utilities are not included in B2." [S10]

Three inputs were considered:

| Input | Verdict |
| --- | --- |
| The text log (what process_jam_log parses) | heuristics on action names and paths, 927 lines of C++ upstream |
| The `bin.v2` tree (`<name>.test` exists, `<name>.output` ends with `EXIT STATUS: n`) | cumulative across runs, but cannot tell a compile from a link failure, keeps stale directories, and needs the test list from elsewhere |
| `b2 --out-xml=FILE` (`build-system.jam:53`, "Output build information to an XML file.") | structured: every `<test type name>` with its `<target>` and `<source>` (with `--dump-tests` they go to the XML instead of stdout, testing.jam:305-316), and every executed `<action status=...>` with its name, path, properties (toolset and version), command and output |

**`BOOST_REGRESSION_MATRIX_B2_MATRIX.py`** (beside this note, 167 lines,
Python 3 stdlib) reads one or more `--out-xml` files and writes one HTML
page: a summary (libraries by toolsets: `OK`, the worst failure, or `n/a`),
a table per library (tests by toolsets, with Boost's type column), and the
output and command of every failure; it prints the matrix as text and exits
1 when any cell failed. Cells: `pass` (b2's `testing.expect-success` or
`expect-failure` action for `<name>.test` succeeded), `comp`, `link`, `run`
(the first failed action), `compiled`, `linked`, `ran` (a `*-fail` test whose
step succeeded), `n/a` (no action). Measured on the spike (6.7).

What `--out-xml` costs, measured:

- **b2 exits 0 with failing tests** when `--out-xml` is given (all three
  runs; the log ends with a second update pass, `...updated 1 target...`
  for the XML file). The same runs without it exit 1. The CI verdict must
  come from the script.
- Only executed actions are recorded: an up-to-date test is `n/a`. Build
  in a fresh directory, or with `-a`.
- The XML format is not in b2's manual (only the changelog mentions
  `--out-xml`, as "Fixed broken --out-xml" [S10]); it is defined in
  `build-system.jam` lines 877-1050. Pin b2 and test the script against it.

What it would take to grow it into Boost's full feature set: an
expected-failures file (Boost's `explicit-failures-markup.xml` has
`<mark-expected-failures>` with `<test>`, `<toolset>` and `<note>`), a
comparison with the previous run to tell regressions from new failures,
and per-runner metadata (revision, timestamp) in the column headers.

## 6. The spike

### 6.1 The project (`WORK/spike`, verbatim)

`Jamroot`:

```jam
# Spike: a Boost-style collection (libs/<name>/test) tested with b2, its
# results fed to boostorg/regression's process_jam_log and library_status.
import feature ;
import type ;

# Boost 1.92, read only: headers through the existing link tree, nothing built.
path-constant BOOST_ROOT : <private>/xstate-cpp/deps/boost ;

# wasm32-wasip2 is no target-os b2 knows (see XSTATE_CPP_BOOST_B2.md 4).
if ! wasi in [ feature.values <target-os> ]
{
    feature.extend target-os : wasi ;
}
type.set-generated-target-suffix EXE : <target-os>wasi : wasm ;

project spike
  : requirements
      <cxxstd>20
      <warnings>extra
      <warnings-as-errors>on
      <include>libs/demo/include
      <include>libs/util/include
      <include>$(BOOST_ROOT)
      <toolset>gcc:<cxxflags>-isystem$(BOOST_ROOT)
      <toolset>clang:<cxxflags>-isystem$(BOOST_ROOT)
      <toolset>clang-wasi:<target-os>wasi
      <target-os>wasi:<exception-handling>off
      <target-os>wasi:<rtti>off
      <target-os>wasi:<link>static
  : default-build <variant>debug
  : build-dir bin.v2
  ;
```

No `use-project /boost` and no `/boost//headers`: the include is the
read-only tree's existing link directory, so b2 generates no link and writes
nothing there.

`status/Jamfile`:

```jam
# Every library's tests, as Boost's own status/Jamfile does. b2 is run from
# here, so the targets it prints start with ../, which process_jam_log needs.
# testing defines preserve-test-targets, which the command line names.
import testing ;

build-project ../libs/demo/test ;
build-project ../libs/util/test ;
```

`libs/demo/test/Jamfile`:

```jam
import testing ;

# (a) lightweight_test, passes
run pass.cpp ;
# (a') the same source without exceptions or RTTI, as a row of its own
run pass.cpp : : : <exception-handling>off <rtti>off : pass_noexcept ;
# (b) lightweight_test, one BOOST_TEST_EQ fails
run fail.cpp ;
# (c) a run test whose source does not compile
run broken.cpp ;
# (d) a compile-fail test: it must not compile, and does not
compile-fail compile_fail.cpp ;
# (d') a compile-fail test that compiles, so it fails
compile-fail compiles.cpp ;
# (e) Boost.Test (header-only form), passes; not on wasi (Boost.Test 1.92
# does not compile for wasip2) and not without exceptions. The wasi condition
# names the toolset: <target-os>wasi:<build>no, where target-os is itself set
# by a conditional, stops b2 with "Can not evaluate conditional properties".
run boost_test_pass.cpp
  : : : <toolset>clang-wasi:<build>no <exception-handling>off:<build>no ;
```

`libs/util/test/Jamfile` is `import testing ; run twice.cpp ;`. The sources
(`libs/demo/include/demo/add.hpp` is `constexpr int add(int a, int b)`):

```cpp
// pass.cpp
#include <demo/add.hpp>

#include <boost/core/lightweight_test.hpp>

int main() {
    BOOST_TEST(demo::add(1, 1) == 2);
    BOOST_TEST_EQ(demo::add(40, 2), 42);
    return boost::report_errors();
}

// fail.cpp: the same with BOOST_TEST_EQ(demo::add(40, 2), 41);
// broken.cpp: BOOST_TEST_EQ(demo::add("one", 1), 2);  (does not compile)
// compile_fail.cpp: int main() { return demo::add("one", 1); }
// compiles.cpp: int main() { return demo::add(1, 2) - 3; }
// boost_test_pass.cpp: #define BOOST_TEST_MODULE demo,
//   <boost/test/included/unit_test.hpp>, BOOST_AUTO_TEST_CASE(adds)
// twice.cpp: BOOST_TEST_EQ(util::twice(21), 42);
```

### 6.2 Running the tests (`WORK/run-tests.sh`)

From `spike/status`, each with `-j8 --dump-tests preserve-test-targets=off`
and its log in `spike/<lane>.log`:

| Lane | Extra arguments | b2 exit | Result |
| --- | --- | --- | --- |
| clang | none (`clang-darwin-21`) | 1 | pass, pass_noexcept, compile_fail, boost_test_pass, twice `**passed**`; fail `...failed testing.capture-output`; broken `...failed clang-darwin.compile.c++`; compiles: `...failed clang-darwin.compile.c++ .../compiles.o` (a compile that succeeds in a compile-fail test is reported as a failed action) |
| gcc15 | `--user-config=.../user-config-gcc.jam toolset=gcc-15` | 1 | the same |
| wasi | `--user-config=.../user-config-wasi.jam toolset=clang-wasi testing.launcher=wasmtime` | 1 | the same, boost_test_pass not built |

The three lanes take 7.6 s together from a clean `bin.v2`. Directories:
`bin.v2/libs/demo/test/<name>.test/clang-darwin-21/debug/cxxstd-20-iso/`,
`.../gcc-15/debug/cxxstd-20-iso/`, and
`.../clang-darwin-wasi/debug/cxxstd-20-iso/exception-handling-off/link-static/rtti-off/target-os-wasi/`;
`pass_noexcept` adds `exception-handling-off/rtti-off/`.

### 6.3 lightweight_test without exceptions and on wasm32-wasip2

- Compile lines (from `-d2` and `b2 -n -a`): wasi
  `<wasi-sdk>/bin/clang++ -std=c++20 -fno-exceptions -fno-rtti --target=wasm32-wasip2 ... -isystem<deps/boost> -c pass.cpp`,
  linked to `pass.wasm`, run as `wasmtime ".../pass.wasm" > ".../pass.output" 2>&1 < /dev/null`;
  GCC `g++-15 -std=c++20 -fno-exceptions -fno-rtti -D_Static_assert=static_assert ... -Wall -Wextra -Werror`.
  All compile with `-Werror`, because the Boost tree is also `-isystem`.
- Every `fail.output` is byte-identical across clang, clang without
  exceptions, GCC and wasm (`cat -e`, runs from `libs/demo/test`; from
  `status/` the file name reads `../libs/demo/test/fail.cpp(7)`):

```
fail.cpp(7): test 'demo::add(40, 2) == 41' ('42' == '41') failed in function 'int main()'$
1 error detected.$
$
EXIT STATUS: 1$
```

  and every `pass.output` reads `No errors detected.` then `EXIT STATUS: 0`.
  wasmtime passes the exit status through, so b2 sees the failure.
- Under `BOOST_NO_EXCEPTIONS`, `BOOST_TEST_THROWS` expands to nothing and
  `BOOST_TEST_NO_THROW` just evaluates its expression [S1]; the rest is
  unchanged. `report_errors()` returns the error count, capped at 255
  (`boost/core/lightweight_test.hpp`, `report_errors`), and `lwt_init` makes
  a program that forgets to call it end with a nonzero exit code [S1].
- Boost.Test (`boost_test_pass`) passed on clang 21 and on GCC 15 with the
  `user-config-gcc.jam` workaround.

### 6.4 boostorg/regression's tools on the spike (`WORK/run-library-status.sh`, `WORK/run-boost-report.sh`)

```sh
cd spike
process_jam_log placeholder --boost-root $PWD --locate-root $PWD --input-file $PWD/clang.log   # and gcc15.log, wasi.log
python3 ../collect.py --results bin.v2 --toolset clang-darwin-21 --runner spike-clang \
    --platform macOS --tag develop --revision 9258f4 --output runners/spike-clang.xml        # one per toolset
boost_report --expected reports/src/empty_expected_results.xml --markup markup.xml \
    --comment comment.html --tag develop -rl -rds -rdd -rus -rud -ri -rn \
    --css reports/src/master.css runners/*.xml
unzip report.zip
```

`markup.xml` is an empty `<explicit-failures-markup>`. `collect.py` writes
what `collect_logs` writes: an XML declaration, `<test-run tag platform
runner timestamp source run-type="full" revision>`, a `<comment>`, and every
`test_log.xml` whose `toolset="..."` matches, copied verbatim.

**A matrix came out**: `WORK/spike/report/boost_report/site/developer/summary.html`
(frameset; the table is `summary_.html`), rendered in
`WORK/shots/summary_.html.png` and `WORK/shots/demo_.html.png`. It is
Boost's page, with Boost's stylesheet:

- Summary: "Unusable: 0 | Regressions: 0 | New failures: 9"; header rows
  `macOS` / `spike-clang`, `spike-gcc15`, `spike-wasi` / `rev 9258f4` /
  timestamp / `clang-darwin-21`, `gcc-15`, `clang-darwin-wasi`; rows `demo`
  (`comp` in all three: the most significant failure is a compilation one)
  and `util` (`OK`, green).
- `developer/demo_.html`: rows with types `boost_test_pass r`, `broken r`,
  `compile_fail cf`, `compiles cf`, `fail r`, `pass r`, `pass_noexcept r`.
  Pass is a green `pass`; the failing lightweight_test is a yellow `run`;
  the test that does not compile is an orange `comp`; the expected
  compile-fail is a green `pass` (b2 already turned the failed compile into
  a success); the compile-fail test that compiles shows `comp` (b2 marks its
  compile step failed); boost_test_pass on wasi is an empty `missing` cell.
  `pass_noexcept` is one row with three cells, correctly.
- Each failing cell links to `output/<runner>-bin-v2-libs-demo-test-fail-test-...html`:
  "Compile [...]: succeed", "Link [...]: succeed", "Run [...]: fail", then
  the program's output.
- With `<mark-expected-failures>` naming `fail` on `gcc-15` (measured, a
  second run), that cell became `fail*` (class `library-fail-expected`).

### 6.5 library_status on the spike

`library_status --locate-root <spike> --status-file ... --links-file ... < /dev/null`
from `libs/demo/test` and `libs/util/test` wrote
`WORK/spike/report/library_status/{demo,util}.html` (`WORK/shots/demo.html.png`).
`util.html` is right (`twice: Pass Pass Pass`). `demo.html` is wrong: the
header has five columns (`clang-darwin-21` twice, `clang-darwin-wasi`,
`gcc-15` twice) and every row three cells, so GCC's results sit under the
wasm header; `boost_test_pass` on wasi is a red `Fail`; `pass_noexcept`
shows `Fail` under clang and GCC; `compile_fail` shows `Warn` (`Pass` with
`--no-warn`). From a `-d2` log every passing test is `Warn`. The first
attempt, with a second clang run using `exception-handling=off` on the
command line, aborted with `invalid bin directory structure`.

### 6.6 Every error and manual step on the official path

1. Building the tools needs a Boost source tree; the read-only `deps/boost`
   could not be used (it lacks Filesystem, Iostreams and Program_Options, and
   `use-project /boost` builds into its `bin.v2`). A separate clone was
   made: boost-1.92.0, `depinst` per library, `b2 headers`.
2. `boost/crc.hpp` missing in that partial checkout (above): add crc.
3. `run.py`, `regression.py`, `collect_and_upload_logs.py` are Python 2;
   `process_jam_log.py` fails on Python 3. `collect.py` replaces the collect
   step.
4. The standalone `process_jam_log` skips its first argument: its `main`
   passes `argv[1..]` (`testing/src/process_jam_log.cpp:26`) to a function
   that skips the first element as a program name (`common/src/process_jam_log.cpp:643`).
   From a directory outside the tree, `--boost-root X` was lost and it
   printed `Abort: not able to locate the boost root`; a placeholder first
   argument fixes it.
5. Run b2 from a subdirectory (`status/`), with `import testing` there.
6. `<target-os>wasi:<build>no`, where `<target-os>wasi` comes from another
   conditional (`<toolset>clang-wasi:<target-os>wasi`), failed with
   `error: Can not evaluate conditional properties`, as did
   `<exception-handling>off:<build>no` alone on the wasi lane; naming the
   toolset in the condition works. A b2 fact, not a regression-tool one.
7. process_jam_log writes `.short-sha` into each test source directory.
8. boost_report writes `report.zip` in the current directory; unzip it.

### 6.7 The alternative on the spike

```sh
cd spike/status
b2 -j8 --dump-tests preserve-test-targets=off --build-dir=WORK/alt/build --out-xml=WORK/alt/clang.xml   # and gcc15, wasi
python3 BOOST_REGRESSION_MATRIX_B2_MATRIX.py --title "webcpp spike" --output matrix.html clang.xml gcc15.xml wasi.xml
```

Each b2 exited 0 (see section 5). The script exited 1 and printed:

```
                     clang-darwin-21    gcc-15             clang-darwin-wasi
demo/boost_test_pass pass               pass               n/a
demo/broken          comp               comp               comp
demo/compile_fail    pass               pass               pass
demo/compiles        compiled           compiled           compiled
demo/fail            run                run                run
demo/pass            pass               pass               pass
demo/pass_noexcept   pass               pass               pass
util/twice           pass               pass               pass
```

`WORK/alt/matrix.html` (14.9 KB, `WORK/shots/matrix.html.png`): the summary
`demo: comp comp comp`, `util: OK OK OK`; the two library tables above with
the type column; nine failure sections, each with the output (for `fail`,
the lightweight_test message and `EXIT STATUS: 1`) and the command folded
under `<details>`. Colours come from CSS variables with a dark-mode variant.

## Sources

- [S1] Boost.Core, lightweight_test: <https://www.boost.org/doc/libs/latest/libs/core/doc/html/core/lightweight_test.html>
- [S2] Boost contributor guide, Local Regression Tests (the old URL <https://www.boost.org/development/running_regression_tests.html> redirects here): <https://www.boost.org/doc/contributor-guide/testing/regression-tests.html>
- [S3] Boost contributor guide, Test Matrix: <https://www.boost.org/doc/contributor-guide/testing/boost-test-matrix.html>
- [S4] Live reports, fetched 2026-10-07: <https://regression.boost.org/master/developer/summary.html>, <https://regression.boost.org/master/developer/summary_.html>, <https://regression.boost.org/develop/developer/summary_.html>, <https://regression.boost.org/master/developer/json_.html>
- [S5] boostorg/regression, at the commit used: <https://github.com/boostorg/regression/tree/f085a31cc8b7dde85e3fedd7128bc3b972fbcb71> (every `path:line` above without another source is in this tree)
- [S6] library_status documentation: <https://github.com/boostorg/regression/blob/develop/library_status/doc/library_status.html>
- [S7] Running Regression Test Reports: <https://github.com/boostorg/regression/blob/develop/reports/doc/reports.html>
- [S8] Report job: <https://github.com/boostorg/regression/blob/ci/test-reports/ci_build_results_all.sh>, <https://github.com/boostorg/regression/blob/ci/test-reports/.circleci/config.yml>
- [S9] Issues and pull requests: <https://github.com/boostorg/regression/issues/68>, <https://github.com/boostorg/regression/issues/63>, <https://github.com/boostorg/regression/pull/66>, <https://github.com/boostorg/regression/pull/67>, <https://github.com/boostorg/regression/pull/58>, <https://github.com/boostorg/regression/pull/61>
- [S10] B2 manual, testing: <https://www.bfgroup.xyz/b2/manual/release/index.html#b2.builtins.testing> (the changelog entry on `--out-xml` is in the same page)
- [S11] B2 manual, command-line options (`--build-dir`, `-o`, `-d`): <https://www.bfgroup.xyz/b2/manual/release/index.html#b2.overview.invocation.options>
- [S12] Robert Ramey's library_status: <https://github.com/robertramey/library_status>
- [S13] GitHub Pages with custom workflows: <https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages>
- [S14] Boost.CI: <https://github.com/boostorg/boost-ci>
- [S15] boostorg/regression CI: <https://github.com/boostorg/regression/blob/develop/.github/workflows/ci.yml>
- b2 source at the commit Boost 1.92 pins (`testing.jam`, `build-system.jam`): <https://github.com/boostorg/build/tree/05b6b638b54e1f013fb8b281f5020e337be31363/src>

One correction to [S11], measured: in B2 5.5.3, `-o file` copies the log
into the file and still runs the actions (`b2 -a -o file` on `libs/util/test`
compiled, ran and printed `**passed**`; the file held the same 15 lines as
standard output), while the manual says "Write the updating actions to the
specified file instead of running them."
