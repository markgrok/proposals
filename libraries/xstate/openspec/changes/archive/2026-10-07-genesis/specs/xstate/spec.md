## Purpose

webcpp::xstate is a header-only C++20 port of XState 5.33.2's state machines and actors. A machine
is loaded from the JSON config XState's `createMachine` takes, with its actions, guards, delays and
actors registered by name in C++, and is run by XState's pure functions and a macrostep cursor (the
machine core), or as XState's actors on webcpp::xactor (the actor layer). It reports every error as
a value, throws nothing, reads no clock and holds no global state, and it is proven against XState
itself: the oracle runs the ported test cases and a twin of each example on XState 5.33.2. These
requirements describe 0.1.0 as built (`reference/source/`).

## ADDED Requirements

### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::xstate`, with internals in `webcpp::xstate::detail`, public headers under `<webcpp/xstate/...>` and the two convenience headers `<webcpp/xstate.hpp>` and `<webcpp/xstate/actors.hpp>`, one header per responsibility, include guards `WEBCPP_XSTATE_<PATH>_HPP` (`WEBCPP_XSTATE_HPP`, `WEBCPP_XSTATE_ACTORS_ACTOR_SYSTEM_HPP`) and macros prefixed `WEBCPP_XSTATE_`; the one macro it defines is `WEBCPP_XSTATE_NO_EXCEPTIONS`, and the macros of its tests are prefixed `WEBCPP_TEST_` (`WEBCPP_TEST_XSTATE_FIXTURES`). Nothing of it is compiled on its own. The error category it reports is named `webcpp.xstate`, and the company and the site of the original (Stately, `stately.ai`) are named only where the text means them.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/xstate//xstate` and includes `<webcpp/xstate.hpp>`
- **THEN** it compiles and links with Boost.JSON's definitions as the only compiled part, since every header is self-contained

#### Scenario: The quick start program
- **WHEN** `example/xstate/qs_toggle.cpp` creates the toggle machine (`Inactive` and `Active`, the event `toggle` between them) and runs `initial_transition`, then `transition` twice
- **THEN** it prints `Value: "Inactive" (0 actions)`, `Value: "Active" (0 actions)` and `Value: "Inactive" (0 actions)`

### Requirement: Library: A port of XState 5.33.2, function by function
The library SHALL port XState 5.33.2, the npm package `xstate` (commit `fbee62e7c1586315ed478c2fedf530d7e0ff5a3e` of https://github.com/statelyai/xstate, MIT), pinned exactly by `test/oracle/package.json` and `package-lock.json`: the algorithm, not a semantics of its own. Transition selection, the exit and entry sets, history, done events, eventless transitions, raised and delayed events and output are translated function by function, and every ported function names, in its Doc Comment, the TypeScript function it ports, so that a difference from XState points at one function on each side. The machine's semantics, the order of its actions and microsteps and its errors follow that version; moving to another version of XState is a change of its own, with the oracle's new expected results as its evidence. Where xstate departs from XState it does so on purpose, for one of four reasons (XState's behaviour belongs to JavaScript, JSON cannot say what XState's config says, xactor runs actors differently, or xstate checks earlier than XState), and each departure is listed in the page's appendix "Where xstate differs from XState".

#### Scenario: The pin is exact
- **WHEN** `test/oracle/package.json` is read
- **THEN** it is private, of type `module`, named `webcpp-xstate-oracle`, and lists the one dependency `"xstate": "5.33.2"`

#### Scenario: A ported function names its original
- **WHEN** the Doc Comment of `matches_state` is read
- **THEN** it says that it ports XState's `matchesState`

### Requirement: Library: The public headers
The library SHALL ship 24 public headers. The machine core, which includes nothing of xactor, is `include/webcpp/xstate.hpp` and the 16 headers beside it in `include/webcpp/xstate/` other than `actors.hpp`: `actions.hpp`, `clock.hpp`, `config.hpp`, `definition.hpp`, `errors.hpp`, `event.hpp`, `functions.hpp`, `guards.hpp`, `implementations.hpp`, `machine.hpp`, `macrostep.hpp`, `microstep.hpp`, `node_set.hpp`, `snapshot.hpp`, `state_node.hpp` and `state_value.hpp`. The actor layer is `include/webcpp/xstate/actors.hpp` and the six headers of `include/webcpp/xstate/actors/`: `actor_system.hpp`, `fuel.hpp`, `host_logic.hpp`, `machine_logic.hpp`, `message.hpp` and `system_state.hpp`. `xstate.hpp` MUST include `config.hpp` first and then the 15 other headers of the core; `actors.hpp` MUST include `config.hpp`, `<webcpp/xstate.hpp>` and the six headers of the layer. `guards.hpp` and `microstep.hpp` declare only `webcpp::xstate::detail`, the transition-selection and entry-and-exit procedures the cursor runs; the other headers declare the names of the page's reference. Every header but `config.hpp` MUST include `<webcpp/xstate/config.hpp>` first.

#### Scenario: The core header leaves the actor layer out
- **WHEN** a program includes only `<webcpp/xstate.hpp>`
- **THEN** no header of `<webcpp/xactor/...>` and none of `<webcpp/xstate/actors/...>` is included, which `test/core_alone.cpp` checks by an `#error` for each include guard of xactor's headers and of the actor layer's

#### Scenario: The actor layer brings the core
- **WHEN** a program includes only `<webcpp/xstate/actors.hpp>`
- **THEN** every name of the machine core is declared too, since `actors.hpp` includes `<webcpp/xstate.hpp>`

### Requirement: Library: Two layers, one boundary
The library SHALL keep two layers apart. The machine core needs Boost's headers, Boost.JSON and webcpp's configuration alone, runs no actor (it knows XState's actors as data, the `implementations::actors` a machine names and the `children` a snapshot lists, and resolves an invoke or a spawnChild into an `xstate.spawnChild` action and a stopChild or the exit of an invoking state into an `xstate.stopChild`, for its caller to run), and is usable on its own. The actor layer executes what the core returns, as XState splits `resolve` from `execute`, and is the only part that knows xactor. `meta/include-boundaries.json` MUST declare one boundary: the headers `include/webcpp/xstate.hpp` and `include/webcpp/xstate/*.hpp`, but `include/webcpp/xstate/actors.hpp`, must not include `webcpp/xactor` or `webcpp/xstate/actors`, for the reason "the machine core runs without xactor and the actor layer"; the superproject's lint fails an include that crosses it, naming the file and the line.

#### Scenario: A core header includes xactor
- **WHEN** `include/webcpp/xstate/machine.hpp` includes `<webcpp/xactor/ids.hpp>`
- **THEN** the lint fails naming `machine.hpp` and the line, and `core_alone` fails to compile

#### Scenario: The boundary file
- **WHEN** `meta/include-boundaries.json` is read
- **THEN** it holds one boundary with `must-not-include` of `webcpp/xactor` and `webcpp/xstate/actors`

### Requirement: Library: Build target and dependencies
`build.jam` SHALL declare `project /webcpp/xstate` and one alias `xstate` whose usage requirements are `<include>include`, `<library>/webcpp/xactor//xactor`, `<library>/boost//headers`, `<library>/webcpp/config//config` and `<library>/webcpp//boost_json`. The library needs a C++20 compiler, Boost 1.92 or newer (its headers, Boost.JSON and Boost.System; Boost.Asio is brought by xactor outside WASI), webcpp's configuration and, for the actor layer, webcpp::xactor. A program that uses the machine core alone depends on xactor in name only: no header of it is included. The library needs no thread and no compiled library of its own.

#### Scenario: One alias brings everything
- **WHEN** a program names `/webcpp/xstate//xstate`
- **THEN** the include paths of xstate, xactor, Boost and webcpp's configuration, and Boost.JSON's definitions compiled once per variant, reach its compile and link lines

#### Scenario: The quick start builds with a compiler alone
- **WHEN** `qs_toggle.cpp` is copied as `toggle.cpp` and built with `clang++ -std=c++20 -I libs/xstate/include -I libs/config/include toggle.cpp tools/boost_json.cpp -o toggle`
- **THEN** it builds and runs, `tools/boost_json.cpp` being the one translation unit that includes `<boost/json/src.hpp>`

### Requirement: Library: Boost.JSON is the value type
Every value a machine holds (its config, its context, an event's payload, an action's `params`, a state value, an input and an output) SHALL be a `boost::json::value`, and results and error codes SHALL be Boost.System's. Boost.JSON's headers declare and `<boost/json/src.hpp>` defines, so exactly one translation unit of a program includes that header (webcpp's `tools/boost_json.cpp`, linked as `/webcpp//boost_json`). A JSON object keeps its members in the order they were written, which the library needs to read a config's `states`, `on` and `after` in the order XState reads them.

#### Scenario: The link misses Boost.JSON's definitions
- **WHEN** a program that creates a machine is linked without a translation unit that includes `<boost/json/src.hpp>`
- **THEN** the link fails on Boost.JSON's symbols, which `/webcpp/xstate//xstate` prevents by bringing `/webcpp//boost_json`

### Requirement: Library: Configuration for exceptions
`<webcpp/xstate/config.hpp>` SHALL be the first include of every public header; it includes `<webcpp/config.hpp>` first and defines `WEBCPP_XSTATE_NO_EXCEPTIONS` when webcpp has no exceptions (`WEBCPP_HAS_EXCEPTIONS` is not defined: the compiler has none, or the program defined `WEBCPP_NO_EXCEPTIONS` for every webcpp library) or when the program defines it for xstate alone, to turn xstate's exceptions off in a build that has them, as Boost.Asio's `BOOST_ASIO_NO_EXCEPTIONS` does. xstate raises no exception of its own, so the macro changes nothing in it; it exists so that a program can set it for every webcpp library alike, and an exception xstate raised would go through `boost::throw_exception`. For the MrDocs reference, which parses xstate as a build with exceptions does, the macro is documented by a definition undone at once.

#### Scenario: WEBCPP_NO_EXCEPTIONS defines the library's macro
- **WHEN** `test/config_test.cpp` is compiled with `WEBCPP_NO_EXCEPTIONS` and `WEBCPP_TEST_XSTATE_ALL_OFF` defined (the target `config_all_off`)
- **THEN** it compiles, since `WEBCPP_XSTATE_NO_EXCEPTIONS` is defined while `WEBCPP_HAS_EXCEPTIONS` is not

#### Scenario: A developer's definition is kept
- **WHEN** `config_test.cpp` is compiled with `WEBCPP_XSTATE_NO_EXCEPTIONS` and `WEBCPP_TEST_XSTATE_DEVELOPER_DEFINED` defined in a build that has exceptions (the target `config_developer_defined`)
- **THEN** it compiles, the macro still defined

#### Scenario: Defined exactly when webcpp has no exceptions
- **WHEN** `config_test.cpp` is compiled with no definition (the target `config`)
- **THEN** it compiles only if `WEBCPP_XSTATE_NO_EXCEPTIONS` is defined exactly when `WEBCPP_HAS_EXCEPTIONS` is not, and `<webcpp/config.hpp>` has been included by `config.hpp`

### Requirement: Library: No exception of its own, compiled without exceptions too
The library SHALL throw no exception and catch none: every operation that can fail returns a `result`, and a header raises only through `boost::throw_exception` (the lint refuses a bare `throw` in a public header, even in a template nothing instantiates). Every program, built for native, wasm32-wasip2 and wasm32-wasip3, is built with exceptions on, and the lint compiles every public header, and every test and example, which instantiate its templates, with `exception-handling=off`; the two Boost.Test suites, native only, are built once with exceptions, their framework included, and the lint compiles their own sources without them too. Without exceptions, Boost's own code still names `boost::throw_exception`, which the program defines (webcpp's `tools/throw_exception.cpp` reports what was thrown on the standard error and aborts, since in such a build a throw is a defect, never a condition); a program built with exceptions needs no such definition. The `value()` of a failed result reports through `boost::throw_exception` and `*` of one is undefined, so a caller tests `has_value()` first.

#### Scenario: A template nobody instantiates
- **WHEN** a public header holds a template that contains `throw`
- **THEN** the lint fails, since an exception goes through `boost::throw_exception`

#### Scenario: Examples without exceptions
- **WHEN** the lint compiles `example/xstate/qs_toggle.cpp` with `exception-handling=off` and `tools/throw_exception.cpp`
- **THEN** it compiles, since the program tests `has_value()` and never throws

### Requirement: Library: Determinism, no global state and no environment
The library SHALL hold no global or static mutable state, start no thread, and never read a clock, a file or a network: the same machine and the same events give the same run, and every machine of one process is independent of every other. Time is a number the host brings (`actor_system::clock_tick`, or a `simulated_clock` the caller moves), randomness does not exist in it, and the root's session ids count within one `actor_system`. The lint refuses any real clock, file, socket or process a header could name, which no test can show xstate never reaches.

#### Scenario: The same inputs give the same output on every target
- **WHEN** an example is run natively, on wasm32-wasip2 and on wasm32-wasip3
- **THEN** it prints on each the output committed beside it in its `.expected` file

### Requirement: Library: Targets
The library SHALL run natively (with any compiler that builds C++20: GCC 14 and 15, Clang 18 and 22, Apple Clang and MSVC 14.3 and 14.5 are the lanes of the continuous integration), on wasm32-wasip2 and on wasm32-wasip3 (the toolsets `clang-wasip2` and `clang-wasip3`, Clang of wasi-sdk, run by wasmtime), each with exceptions, as WebAssembly's exception handling. `test/Jamfile` and `example/Jamfile` MUST declare `webcpp.targets native wasip2 wasip3 ;`. The tests and the examples are built and run from the superproject's root with `b2 libs/xstate/test libs/xstate/example`, `b2 libs/xstate/test libs/xstate/example toolset=clang-wasip2 testing.launcher=wasmtime` and the same with `toolset=clang-wasip3`; b2 compares timestamps, so a result that counts is built from scratch with `b2 -a`. The two Boost.Test suites read their fixture files at run time, which a wasm program cannot without a directory its runtime opens, so they are built for the native target alone; every other test and every example runs on the three targets. On Linux, GCC and Clang build with libstdc++, so the lanes `clang-linux-18` and `clang-linux-22` run xstate with Clang on libstdc++. Emscripten is not a target of xstate's tests and examples.

#### Scenario: The same programs on WebAssembly
- **WHEN** `b2 libs/xstate/test libs/xstate/example toolset=clang-wasip2 testing.launcher=wasmtime` runs
- **THEN** every `webcpp.run` test, every `webcpp.compile` target and every example is built for wasm32-wasip2 and run by wasmtime, but `cases` and `actors_cases`

#### Scenario: Each lane records what b2 built
- **WHEN** a lane runs `b2 -a --dump-tests --out-xml=<lane>.xml toolset=<toolset> libs/xstate/test libs/xstate/example`
- **THEN** webcpp's `tools/report/report.py` merges the files of every lane into the test matrix at https://webcpporg.github.io/report/

### Requirement: Library: What is not ported
The library SHALL NOT port, in 0.1.0, these parts of XState 5.33.2, and the page's overview lists them: `enqueueActions`; callback actors (`fromCallback`) and observable actors (`fromObservable`, `fromEventObservable`); `fromTransition`; the persistence of actors (`getPersistedSnapshot` and an actor created from a persisted snapshot; a snapshot is a value its caller may store, and `resolve_state` brings it back); `toPromise` and `waitFor`; `syncSnapshot`, and with it an invoke's `onSnapshot`; `spawn` inside `assign`; `machine.provide()` (a machine takes its implementations once, from `create_machine`; the same config created again with other implementations is the machine `provide` would return); the SCXML converter, which the published package does not export; XState's `options.maxIterations` (the cursor's fuel replaces it); and `xstate/graph` (a test walks a machine with `get_next_transitions` and `get_next_snapshot`). A config that names an invoke's `onSnapshot` loads, as transitions on `xstate.snapshot.<id>`, but the actor layer sends no child's snapshot to its parent, so only an event its caller sends takes them.

#### Scenario: onSnapshot is read and never taken by a child
- **WHEN** a machine whose state `a` has `"invoke": {"src": "child", "onSnapshot": "ready"}`, with a sibling state `ready`, is created and run as an actor
- **THEN** it is created, and the child's snapshots send the parent no `xstate.snapshot.<id>` event, so it stays in `a` until its caller sends one

#### Scenario: Model-based testing is not a library feature
- **WHEN** the page's testing chapter shows paths through a machine
- **THEN** its program `testing_paths.cpp` walks breadth first with `get_next_transitions` and `get_next_snapshot` and prints `"idle": [xstate.init]`, `"running": [xstate.init, start]` and `"paused": [xstate.init, start, pause]`

### Requirement: Library: Metadata and licence
`meta/libraries.json` SHALL hold the key `xstate`, the name `xstate`, the authors `["R"]`, the maintainers `["R <github.overboard312@passmail.net>"]`, the description "A header-only port of XState's state machines and actors: machines read from XState's JSON config, stepped by XState's pure functions or run as actors on xactor.", the category `["State"]`, `cxxstd` `"20"` and `"port-of"` with the name `XState`, the language `TypeScript`, the version `5.33.2`, the url `https://github.com/statelyai/xstate` and the licence `MIT`. The code SHALL be under the Boost Software License 1.0 (`LICENSE_1_0.txt`) alone; `LICENSE-XSTATE.txt` is XState's MIT notice, unchanged, as XState 5.33.2 ships it, and the README names what derives from XState: the machine semantics, ported from XState's algorithms into `include/webcpp/xstate`; the test cases under `test/fixtures/`, transcribed from XState's tests; and the twins under `test/oracle/twins/` with the examples of `example/` that the page shares with XState's own documentation. That adds no licence to xstate's own code.

#### Scenario: The port is declared
- **WHEN** `meta/libraries.json` is read by the superproject's index
- **THEN** the index lists xstate with "XState 5.33.2" as what it ports, linked to its page

#### Scenario: The derived parts are named
- **WHEN** the README's section "What derives from XState" is read
- **THEN** it lists the three parts above and says that XState's licence adds none to xstate's own code

### Requirement: Library: Repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json`, `meta/include-boundaries.json`, `README.md`, `AGENTS.md` (only what is specific to xstate, the superproject's rules applying), `LICENSE_1_0.txt`, `LICENSE-XSTATE.txt`, `doc/`, `example/`, `test/`, `.gitignore` (`doc/html/`, `node_modules` and `.node-modules/`), `.gitattributes` (`* text=auto eol=lf`, `*.bat text eol=crlf`) and `.github/workflows/ci.yml`, which calls the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main` on push to `main`, pull requests and manual dispatch, with `contents: read` permission and a concurrency group that never cancels a run on `main`. The README MUST show the minimal example (`example/xstate/qs_toggle.cpp` and its output, each under an include marker that names its file), how to build and test inside the superproject, the oracle lane, what derives from XState, the documentation location and the licence. `AGENTS.md` MUST state the original and its version, the two layers and their boundary, that errors are values, that `config.hpp` comes first, how the oracle drives XState, the case suites, the examples and their twins, and the oracle lane.

#### Scenario: The CI is the superproject's
- **WHEN** `.github/workflows/ci.yml` is read
- **THEN** its one job `ci` uses `webcpporg/webcpp/.github/workflows/library.yml@main`, which builds the commit inside a checkout of webcpp on every lane xstate declares and on its oracle lane, with its page, the lint and the test matrix

### Requirement: Library: Licence notice and text rules
Every source file (`.hpp`, `.cpp`, `.py`, `.mjs`, `.jam`, `Jamfile`, `build.jam`, `.yml`, `.adoc`) SHALL open with the notice "Copyright (c) 2026 WebCpp.org" and the Boost Software License 1.0 lines, `//` for C++, JavaScript and AsciiDoc and `#` for the rest. Every file MUST be in English and without the em dash. The fixtures transcribed from XState's tests and the twins keep XState's copyright notice through `LICENSE-XSTATE.txt` and the README.

#### Scenario: A file without the notice fails the lint
- **WHEN** a new `.hpp` is added without the licence notice
- **THEN** the superproject's lint fails, naming the file

### Requirement: Errors: The error codes
The library SHALL report its errors as the values of `enum class errc : int`, which start at 1 so that an `error_code` of value 0, which a snapshot that has not failed holds, is no error, and which are fixed and never reused: `invalid_config` (1), `unknown_target` (2), `unknown_state` (3), `unknown_guard` (4), `unknown_delay` (5), `implementation_failed` (6), `invalid_event` (7), `actor_failed` (8), `unknown_actor` (9) and `system_id_taken` (10). `invalid_config` is a config that is not a machine, a member of the wrong type, or an implementation or a clock misused; `unknown_target` a target that names no state or a send that names no actor; `unknown_state` a state value, a state id or a path that names no state; `unknown_guard` a guard the config names without an implementation; `unknown_delay` an `after` delay without one; `implementation_failed` an implementation that failed where XState's would have thrown, also the error xstate reports for an `assign` on a context that is not an object and for a context function whose value is not one; `invalid_event` an event without a string `type` or a wildcard-typed one a macrostep takes from outside; `actor_failed` a child actor's error event that reached a machine and no transition took; `unknown_actor` an invoke or a spawnChild naming no actor of the implementations; `system_id_taken` an actor claiming a systemId another actor of the system holds. There is no `invalid_argument` in `errc`.

#### Scenario: Every code spells its own name
- **WHEN** `make_error_code` is called for each of the ten codes (the test `values`)
- **THEN** the `error_code` has the category named `webcpp.xstate` and its `message()` is `invalid_config`, `unknown_target`, `unknown_state`, `unknown_guard`, `unknown_delay`, `implementation_failed`, `invalid_event`, `actor_failed`, `unknown_actor` and `system_id_taken` respectively

#### Scenario: A code converts to an error_code and compares with one
- **WHEN** `created.error() == xstate::errc::unknown_target` is evaluated for the error of a machine whose `initial` names no state
- **THEN** it is `true`, since `boost::system::is_error_code_enum<webcpp::xstate::errc>` is `std::true_type`

### Requirement: Errors: The category
The library SHALL report its errors in one constant category, `webcpp::xstate::error_category()`, a `final` class derived from `boost::system::error_category`, constexpr-constructed with the fixed identity `0x7873746174650000` (the ASCII of "xstate" followed by two zero bytes), holding no mutable state. Its `name()` MUST be `"webcpp.xstate"`; its `message(int)` the spelling of the enumerator and `"unknown"` for a value `errc` does not define; and its `message(int, char*, std::size_t)` MUST copy that spelling into the buffer cut to fit with its terminating zero (a null buffer or a size of 0 copies nothing) and return the whole spelling. `make_error_code(errc)` makes the `error_code` of the category.

#### Scenario: An unknown value
- **WHEN** `xstate::error_category().message(99)` is called
- **THEN** it returns `"unknown"`

#### Scenario: A buffer too small
- **WHEN** `message(2, buffer, 5)` is called for `unknown_target`
- **THEN** the buffer holds `unkn` and a zero, and the call returns `"unknown_target"`

### Requirement: Errors: result and failure
The library SHALL define `template <class T> using result = boost::system::result<T, boost::system::error_code>` as what every function that can fail returns, `void` for a function that returns none, and `template <class T> result<T> failure(errc code) noexcept`, the way an implementation reports a failure: `return xstate::failure<bool>(xstate::errc::implementation_failed);`. An implementation MAY return any other `error_code`, which becomes the snapshot's `error` as it is, so a program tells its own failures apart. A caller reaches a value only through its `result`, and `*` of a failed result is undefined.

#### Scenario: An implementation fails with a code of its own
- **WHEN** the registered assigner `save_age` fails because the event carries no age, in `example/xstate/testing_errors.cpp`, and the test sends `save` without one
- **THEN** the snapshot's status is `error`, its value stays `"editing"` and its context `{"age":0}`, as `status: error`, `value: "editing"`, `context: {"age":0}` print

#### Scenario: failure builds the result a call would return
- **WHEN** `xstate::failure<xstate::machine>(xstate::errc::invalid_config)` is evaluated
- **THEN** it is a result without a value whose `error()` equals `xstate::errc::invalid_config`

### Requirement: Errors: The actor layer refuses with xactor's code
The actor layer SHALL refuse a call it cannot take with `webcpp::xactor::errc::invalid_argument`, of the category `webcpp.xactor`, and change nothing: an address that names no actor (`start`, `send`, `status_of`, `subscribe`, `on_emitted`), a `stop` of a child, an answer to a request no longer pending, and a call that changes the system made from inside a callback the host registered. The one refusal of the layer that is xstate's own is `create_actor` of a systemId another actor holds, `errc::system_id_taken`, of the category `webcpp.xstate`. The readers `snapshot_of`, `parent_of`, `id_of`, `get` and `child_of` refuse nothing: they answer `nullptr`, none or an empty view for what they do not find.

#### Scenario: An address that names no actor
- **WHEN** `system.start(xstate::actor_ref{.value = 7})` and `system.send(xstate::actor_ref{.value = 7}, event)` are called on an empty system, and `system.snapshot_of` is asked for the same address
- **THEN** the first two return failed results and `snapshot_of` returns `nullptr`

#### Scenario: A child is not stopped by the host
- **WHEN** the host calls `system.stop(kid)` for an invoked child `kid`
- **THEN** the result is an error whose message is `invalid_argument`, and `kid` is still `active`, still the child of its parent

### Requirement: Events: The event type
The library SHALL represent an event as `struct event { std::string type; boost::json::object payload; }`: XState writes an event flat (`{"type": "SUBMIT", "user": "ana"}`) and xstate keeps the type apart, the payload being every other member, which every implementation reads as `args.event.payload`. A caller builds one with designated initializers, `xstate::event{.type = "SUBMIT", .payload = {{"user", "ana"}}}`; a designated initializer that leaves members out compiles as user code with `-Wextra -Werror` (the test `user_aggregates`). xstate makes some events itself: `xstate.init`, which runs the initial macrostep; `xstate.done.state.<id>`, raised when the state `<id>` reaches a final state, with the final state's `output` when it has one; `xstate.after.<delay>.<id>`, the delayed event of a state's `after`; and, in the actor layer, `xstate.done.actor.<id>` and `xstate.error.actor.<id>`.

#### Scenario: An event built by designated initializers
- **WHEN** `xstate::event{.type = "go"}` is built
- **THEN** its `type` is `go` and its payload is empty (`user_aggregates.cpp` returns 0)

### Requirement: Events: Reading and writing XState's flat JSON form
`event_from_json(const boost::json::value&)` SHALL return a `result<event>` from XState's flat form: the member `type`, a string, becomes the type and every other member goes to the payload, in order; it fails with `errc::invalid_event` when the value is not an object, has no `type`, or its `type` is not a string. An event of the wildcard type is read like any other, since only a macrostep that takes one from outside refuses it. `to_json(const event&)` SHALL return the flat object, `type` first and then the payload's members in order, leaving out a payload member named `type`, since the event's type holds that key.

#### Scenario: Events read and refused
- **WHEN** `event_from_json` is given `{"type": "NEXT", "count": 2}`, `{"count": 2}`, `{"type": "*"}`, `{"type": 3}` and `"NEXT"`
- **THEN** it returns the events `{"type": "NEXT", "count": 2}` and `{"type": "*"}`, and `invalid_event` for the other three

#### Scenario: The JSON example
- **WHEN** `example/xstate/transitions_event_json.cpp` reads `{"type": "feedback.update", "rating": 5}`, `{"type": "*"}`, `{"rating": 5}`, `{"type": 5}` and `"feedback.update"`
- **THEN** it prints `feedback.update {"rating":5}`, `* {}` and `invalid_event` three times

### Requirement: Events: The init event and the wildcard descriptor
The library SHALL define `inline constexpr std::string_view init_event_type = "xstate.init"`, the type of the event every initial macrostep runs with, which carries the machine's input as its member `input` when the input is not null, and `inline constexpr std::string_view wildcard = "*"`, the descriptor that matches every event. The two libraries differ for two events a caller sends from outside: XState's macrostep takes no transition for `xstate.init`, only the eventless transitions it then finds enabled, and `xstate.stop`, the event that stops an XState actor, stops the machine; xstate's `begin` selects transitions for both as for any other event, so an `on` entry that names one, or a `*` transition, takes it, and a caller should send neither.

#### Scenario: The input travels in the init event
- **WHEN** an actor is created with the input `{"userId":"123","defaultRating":5}` and its machine's context function records the `xstate.init` event it is handed
- **THEN** the event is `{"type":"xstate.init","input":{"userId":"123","defaultRating":5}}`

### Requirement: Values: Splitting a state id into a path
`to_state_path(std::string_view id)` SHALL split a state id at each dot into the keys of its path, a backslash making the character after it part of the segment, and return at least one segment (an empty id is the path of one empty key).

#### Scenario: Paths
- **WHEN** `to_state_path` is given `a`, `a.b.c` and `a\.b.c`
- **THEN** it returns `{"a"}`, `{"a", "b", "c"}` and `{"a.b", "c"}`

### Requirement: Values: Building a state value from a path
`path_to_state_value(std::span<const std::string>)` SHALL return the state value a path describes: `["a", "b", "c"]` is `{"a": {"b": "c"}}`, `["a"]` is `"a"`, and an empty path is `{}`. `to_state_value(const boost::json::value&)` SHALL turn a dotted string into the object form (`"a.b.c"` is `{"a": {"b": "c"}}`) and return any other value as it is.

#### Scenario: A dotted string
- **WHEN** `to_state_value` is given `"a.b.c"`
- **THEN** it returns `{"a":{"b":"c"}}`

### Requirement: Values: Matching a state value
`matches_state(parent, child)` SHALL return whether the state value `child` is `parent` or a state within it, either value being a state value or a dotted string, with an explicit list of the pairs still to compare in place of recursion. `snapshot::matches` is this function with the snapshot's value as the child.

#### Scenario: The rows of XState's match test
- **WHEN** `matches_state` is evaluated on the 24 rows of the test `values`, taken one per `expect()` of XState's `match.test.ts`
- **THEN** it returns `true` for (`"a"`, `"a"`), (`"b"`, `"b.b1"`), (`"foo.bar"`, `"foo.bar.baz.quo"`), (`"b"`, `{"b": "b1", "c": "c1"}`), (`{"foo": "bar"}`, `{"foo": {"bar": {"baz": "quo"}}}`) and (`"a.b.c"`, `{"a": {"b": "c"}}`), and `false` for (`"a"`, `"b"`), (`"a.b.c"`, `"a.b"`), (`{"a": {"b": {"c": "d"}}}`, `{"a": "b"}`), (`"a"`, `"b.b1"`) and (`"foo.false.baz"`, `"foo.bar.baz.quo"`)

#### Scenario: Matching a snapshot
- **WHEN** `example/xstate/states_matches.cpp` matches the snapshot `{"form":"invalid"}` against `"question"`, `"form"`, `{"form":"invalid"}`, `{"form":"valid"}`, `"form.invalid"` and `{"track":"playing"}` inside `{"track":"playing","volume":"normal"}`
- **THEN** it prints `false`, `true`, `true`, `false`, `true` and `true`

### Requirement: Machine: create_machine
`create_machine(const boost::json::value& config, implementations registry)` SHALL return a `result<machine>`: the machine built from the JSON of XState's `createMachine` (or `setup({ ... }).createMachine`) config with the implementations its names stand for, or the error that refused the config, and no machine. A config is checked once, at creation, and refuses at once what XState refuses only later, when it first evaluates a guard, enters a state or runs a list of actions; a machine that exists has passed every check, and `create_machine_from_definition` calls `create_machine` too. Nodes are built in document order, a parent before its children, and transitions are formatted once every node exists, because a target may name any id. Two things are checked only when they are used, as in XState: the `#id` of an `xstate.stateIn` guard, which fails the macrostep or `can` with `unknown_state`, and the default of a history state that is the machine's root, which has no parent to resolve it against.

#### Scenario: The first machine
- **WHEN** `example/xstate/machines_create.cpp` creates Stately's feedback machine (the states `question` and `thanks`, the event `feedback.good`) with empty implementations and steps it
- **THEN** it prints `"question"` and then `"thanks"`

#### Scenario: A config that is not a machine
- **WHEN** `create_machine` is given `["not", "an", "object"]`
- **THEN** it returns `invalid_config` and no machine

### Requirement: Machine: The members of a config that are read
The library SHALL read these members of a config: on any state, `id`, `states`, `initial` (a child's key or `{"target": "<key>", "actions": [...]}` with an optional `meta` and `description`), `type` (`atomic`, `compound`, `parallel`, `final` or `history`; without it a state with children is compound, one whose `history` is truthy as JavaScript reads it is a history state, and any other is atomic), `history` (`"deep"` deep, any other truthy value shallow), `target` of a history state, `on`, `always`, `after`, `onDone`, `entry`, `exit`, `invoke`, `output` (kept only for a final state or the root), `tags` (a string or a list of strings), `meta` (any value, a null one being none) and `description` (a string); on the root, `context` and `version`. A transition is a target string, an object (`target`, `actions`, `guard`, `reenter`, `description`, `meta`) or a list of them. It reads no other member of a transition but `cond`, and leaves unread `types` and `schemas` (only TypeScript reads the first, XState keeps the second without using it), `options` (XState uses `maxIterations` to throw on a macrostep that never settles; here the bound is the fuel of the caller that drives the cursor) and `route`, so that the event `{"type": "xstate.route", "to": "#<id>"}` selects no transition.

#### Scenario: A history of false leaves the state atomic
- **WHEN** a state has `"history": false`, `null`, `0` or `""` and no `type`
- **THEN** its node is atomic, as in XState

#### Scenario: options are not read
- **WHEN** a config has `"options": {"maxIterations": 5}` on an `always` cycle
- **THEN** the cycle never settles, since only the caller's fuel bounds a macrostep

### Requirement: Machine: The enumerations and records of the tree
The library SHALL declare, in `state_node.hpp`, the plain data of a machine's tree, copied as values: `enum class node_type { atomic, compound, parallel, final, history }` (the config's `type`, or, without one, `compound` for a node with children, `history` for a node whose `history` member is truthy as JavaScript reads it, and `atomic` otherwise); `enum class history_kind { none, shallow, deep }` (`deep` for `"deep"`, `shallow` for any other truthy `history`, and `none` for a node that is a history node by its `type` alone; a history node of the kind `none` records as a shallow one does, as XState records every history but `deep` as shallow); `action_ref` (`kind` among `named`, `after_raise` and `after_cancel`, `type`, `params`, `config` and `delay`; the two `after` kinds are the delayed raise XState's `getDelayedTransitions` adds to a state's entry and the cancel it adds to its exit, the type of the first being the after event's `xstate.after.<delay>.<state id>`, also the raise's id); `guard_node` (`kind` among `named`, `negation`, `conjunction`, `disjunction` and `state_in`, `type`, `params`, `operands`, `state_value` and `config`); `transition_definition`; `invoke_definition`; and `state_node`.

#### Scenario: A history by type alone
- **WHEN** a state has `"type": "history"` and no `history` member
- **THEN** its node has `type` `history` and `history` `none`, and it records as a shallow history does

### Requirement: Machine: Node ids, types and document order
A machine SHALL be a tree of state nodes kept in one list in document order, the root first, a parent before its children, and the children in the order JavaScript enumerates the keys of `states`: a key that is an array index (`"0"`, or digits without a leading zero, below 2^32 - 1) first, by number, then the rest as written, so the children `b`, `10` and `2` are listed `2`, `10`, `b`. A node is named by its index, so the tree holds no pointer; `machine::size()` is the number of nodes, `node(index)` one of them and `root()` is `node(0)`. A state's id is the `id` its config gives or, without one, the machine's id and the state's path joined by dots, and an explicit id names its own node only (its children's ids are the machine's id and their path). The machine's id, `id()`, is the root's `id`, or `(machine)` when the config gives none. A `state_node` holds its `key`, `id`, `parent`, `path` of keys from the root, `type`, `history` kind, `states`, `entry`, `exit`, `initial`, `transitions`, `always`, `invoke`, `history_target`, `tags`, `meta`, `output`, `description`, `order` (XState's `order`: the number of distinct ids registered before it, which is the index unless two nodes share an id) and `config` without its `states`.

#### Scenario: Ids and types of a small tree
- **WHEN** `machine_test` creates the config with id `m`, a compound `a` (children `a1` and a final `a2`), a parallel `b` with the id `explicit` (children `r1` and `r2`) and a state `h` with `"history": "deep"`
- **THEN** the eight nodes have the ids and types `m` compound, `m.a` compound, `m.a.a1` atomic, `m.a.a2` final, `explicit` parallel, `m.b.r1` atomic, `m.b.r2` atomic and `m.h` history

#### Scenario: The player's nodes
- **WHEN** `example/xstate/machines_nodes.cpp` lists the nodes of the player machine
- **THEN** it prints `0 player player compound`, `1 stopped player.stopped atomic`, `2 playing active compound`, `3 normal player.playing.normal atomic`, `4 fast player.playing.fast atomic` and `5 done player.done final`

### Requirement: Machine: Finding a node
`machine::node_by_id(std::string_view)`, `child(std::size_t node, std::string_view key)` and `node_by_path(std::size_t from, std::string_view path)` SHALL each return a `result<std::size_t>`. `node_by_id` ports `getStateNodeById`: the leading `#` is optional, the first dot-separated segment is an id and each segment after it walks down by key, so a default id such as `(machine).a.a2` resolves through the root's id; a backslash escapes a dot in an id or a path. `child` returns the child of a node whose key is the given one, a dot being part of the key. `node_by_path` ports `getStateNodeByPath`: a path of keys relative to `from`, or the node an id names when the path starts with `#`. A name that finds no node, or an index that is not below `size()`, is `errc::unknown_state`, where XState throws.

#### Scenario: Lookups with escapes
- **WHEN** `machine_test` looks up `#foo\.bar`, `#foo.bar`, `#foo`, `#bar.qux.quux`, `#(machine)`, `#nothing` and `#foo.nothing` in the machine with the states `start`, `stateWithDot` (id `foo.bar`), `foo` (id `foo`, child `bar`) and `q` (id `bar`, children `baz` and `qux` with child `quux`)
- **THEN** the nodes are `foo.bar`, `(machine).foo.bar`, `foo`, `(machine).q.qux.quux`, `(machine)`, and `unknown_state` for the last two

#### Scenario: The active child of a node whose id is explicit
- **WHEN** `node_by_id("#active.fast")` is asked of the player machine whose state `playing` has the id `active`
- **THEN** it is the node `player.playing.fast`

### Requirement: Machine: A machine is immutable and cheap to copy
A `machine` SHALL be a handle to immutable data nothing changes once created, shared by every copy: copying one copies a pointer, not the tree, and a cursor, an actor and a `machine_actor` each keep a copy. `machine` has no default constructor, only `create_machine` makes one, and the transitions a microstep reports are pointers into the machine, valid while any copy of it lives. `config()` returns the config the machine was created from, `registry()` its implementations, `context()` the config's `context` (an empty object when the config gives none or null) and `guard(index)` the guard at an index of the machine's flat list of guards, a higher-order guard's operands being indexes into it. To run one config with other implementations a program creates a second machine from it with the first's `registry()` changed; xstate has no `provide`.

#### Scenario: A second machine with other implementations
- **WHEN** `example/xstate/machines_provide.cpp` creates `counter` with `increment` adding 1, copies its registry, replaces `increment` with an adder of 10 and creates `by_ten` from the same config
- **THEN** it prints `counter {"count":1}` and `byTen {"count":10}`, neither machine having changed the other

### Requirement: Machine: What creation refuses with invalid_config
`create_machine` SHALL return `errc::invalid_config` for: a config that is not an object, or a member of the wrong type (an `id`, a `tags` that is not a string or a list of strings, a `type` xstate does not know, an invoke's `id` or `systemId` that is not a string); a compound state without `initial`; an `on` key of `""` (an eventless transition is `always`); a transition that declares a truthy `cond` (`"never"`, `true`, `1`, `{}`, `[]`), renamed `guard` in XState v5 and refused by its development build, while a falsy one (`false`, `0`, `""`, `null`) is ignored; an `entry`, `exit`, `actions` (of a transition or of an object `initial`) or `tags` that is null, which XState's `toArray` reads as a list that holds null and throws on when it runs it; a history state's `target` that is neither a string nor a list of strings (`5`, `[5]`, `true`, null), the root's included; an `after` key that starts with a digit, `.`, `-` or `+` and is not a whole number of milliseconds (`"1.5"`, `"-5"`, `"5abc"`, `"1e3"`); a state whose default entry leads back to it (a history state whose default target is an ancestor whose initial states reach the history again); an invoke whose `src` is not a string or is missing; a higher-order guard without a `guards` list, an `xstate.not` with other than one operand, or an `xstate.stateIn` without `stateValue`; implementations of which one holds an empty `std::function`; and an entry of `inputs` or `outputs` that names what it may not.

#### Scenario: A table of refusals
- **WHEN** `machine_test` creates each of the rows `{"states": {"a": {}}}`, an `on` key of `""`, an `after` key `"1.5"`, `"-5"` or `"5abc"`, `{"type": "history", "target": 5}`, `{"entry": null}`, `{"tags": ["busy", 1]}`, `{"tags": 5}`, `{"invoke": {"src": {"type": "inline"}}}` and `{"invoke": [{"id": "kid"}]}`
- **THEN** each returns `invalid_config`

#### Scenario: A falsy cond is ignored
- **WHEN** a transition declares `"cond": false`, `0`, `""` or `null`, and another declares `"cond": "never"`
- **THEN** the first four configs create a machine, and the last returns `invalid_config`

#### Scenario: A history cycle
- **WHEN** a compound state `a` has the history states `h1` (target `h2`) and `h2` (target `h1`)
- **THEN** creation returns `invalid_config`, as does a history `h` whose target `#pp` is its own ancestor

### Requirement: Machine: What creation refuses with the other codes
`create_machine` SHALL return `errc::unknown_target` for a transition target, an `initial` or a history state's `target` (below the root) that names no state, and for an `onDone` of an invoke that names no state (`#nowhere`); `errc::unknown_guard` for a guard the config names (as a string or an object `type`) that `implementations::guards` does not hold; `errc::unknown_delay` for an `after` key that is not digits and names no entry of `implementations::delays` (where XState counts it as no delay); and `errc::unknown_actor` for an invoke's `src`, or the `src` of a `spawn_child_action` the config names as an action (in an `entry`, an `on`, an `always`, anywhere), that is no entry of `implementations::actors`. A registered `spawn_child_action` the config never names is not checked, as XState's `setup` accepts it. A named delay of an action's `delay` that the registry does not hold is not an error: it is no delay, as in XState.

#### Scenario: The refusals of the diff example
- **WHEN** `example/xstate/diff_refusals.cpp` creates a config whose `initial` names no child, one with a guard without an implementation, one with a delay without one, one whose default entry leads back, one with an exit that is null and one that invokes an unknown actor
- **THEN** it prints `refused, unknown_target`, `refused, unknown_guard`, `refused, unknown_delay`, `refused, invalid_config`, `refused, invalid_config` and `refused, unknown_actor`

#### Scenario: A spawnChild the config never names
- **WHEN** `implementations::actions` holds a `spawn_child_action` of `src` `absent` and the config is `{"id": "unused"}`
- **THEN** the machine is created; the configs `{"on": {"GO": {"actions": ["spawn"]}}}` and an `always` with `"actions": "spawn"` return `unknown_actor`

### Requirement: Machine: A history state at the root is accepted
`create_machine` SHALL accept a machine whose root is itself a history state, as XState does, whatever its `target` when it is absent, a string or a list of strings (an empty list names nothing, and the root enters nothing below it), and a microstep that needs the default of that history, the initial one or one that enters the root or reaches it through another history's default, MUST fail with `errc::unknown_target`, where XState's `resolveHistoryDefaultTransition` throws. A history state below the root, a child of the root included, resolves its default against its parent. Its target of any other type is refused at creation (`invalid_config`).

#### Scenario: The root is a history state
- **WHEN** a machine whose config is `{"type": "history"}` is created and its initial snapshot is computed
- **THEN** creation succeeds and the snapshot is in error with `unknown_target`

### Requirement: Machine: Computed inputs and outputs name what they compute
An entry of `implementations::inputs` SHALL name an invoke by its id and an entry of `implementations::outputs` a final state by its id, or the root by the root's id, and neither may also have a fixed one in the config; otherwise `create_machine` returns `invalid_config`. An input is computed when its state is entered, after the state's entry actions; a final state's output from the context and the event that entered it, and the machine's from the done event of the final state that completed it, as XState's `getMachineOutput` computes it. A value maker may return none, JavaScript's `undefined`: the member is left out, as XState leaves it out, so the log has no value, the child no input and the snapshot no output.

#### Scenario: Entries that name what they may and may not
- **WHEN** `machine_test` registers an input for the invoke `kid`, and outputs for the final state `m.end` and the root `m`
- **THEN** the machine is created; an input for `absent`, an input for an invoke whose config has `"input": 2`, an output for `absent`, for the non-final `m.a` and for a final state whose config has `"output": 2` each return `invalid_config`

#### Scenario: Outputs that are undefined, null or a value
- **WHEN** `example/xstate/io_undefined_output.cpp` finishes with `{"total":42}`, `{}` and `{"total":null}` as contexts
- **THEN** it prints `42`, `none` and `null` as the output

#### Scenario: The refusals of computed input and output
- **WHEN** `example/xstate/io_refused.cpp` registers an input for no invoke, an input the config fixes too, an output for a state not final, and an output the config fixes too
- **THEN** it prints `invalid_config` four times and then that outputs for a final state and the root are created

### Requirement: Machine: Invokes
A state's `invoke`, one object or a list, SHALL name a child actor by its string `src` with an optional `id` (by default `<index>.<state id>`, XState's `createInvokeId`), `input` and `systemId`, and an invoke is an `invoke_definition` of `id`, `src`, `input`, `system_id` and `config`. Its `onDone`, `onError` and `onSnapshot` are transitions on `xstate.done.actor.<id>`, `xstate.error.actor.<id>` and `xstate.snapshot.<id>`. Entering the state returns, after its entry actions and before its initial actions, one `xstate.spawnChild` per invoke, with the params `{id, src, input?, systemId?}`, and adds the child to the snapshot's `children`; exiting it returns, after its exit actions, one `xstate.stopChild` per invoke, with the params `{id}`, and removes the child, with no params when the child is no longer there. Both change the snapshot, and the machine core runs no child: its caller does.

#### Scenario: Invoking returns spawn and stop actions
- **WHEN** a machine enters a state with `"invoke": {"src": "worker", "id": "worker"}` and then leaves it
- **THEN** the first macrostep returns `xstate.spawnChild` with `{"id":"worker","src":"worker"}` and the snapshot's `children` lists `worker`, and the second returns `xstate.stopChild` with `{"id":"worker"}`

### Requirement: Machine: The order of a node's transitions
A node's transitions SHALL be kept in XState's `Map` order: the keys of `on` in the order JavaScript enumerates them, then `onDone` under `xstate.done.state.<id>`, then each invoke's `onDone`, `onError` and `onSnapshot`, then the delayed transitions of `after`; a descriptor given twice keeps its first place and its last transitions, except under `after`, whose transitions append. A transition's `source`, `target` (none for a targetless transition, which is not the same as an empty list), `actions`, `guard`, `reenter`, `event_type` (`""` for an eventless transition and the after event's type for a delayed one), `delay` and `config` are the members of a `transition_definition`. The order is the order a walk of a machine lists them in (`get_next_transitions`).

#### Scenario: The transitions a snapshot could take
- **WHEN** `example/xstate/pure_next_transitions.cpp` lists them for the form machine in `editing`
- **THEN** it prints `"SUBMIT" form.editing -> form.submitted`, `"TYPE" form.editing -> (targetless)`, `"" form.editing -> form.submitted` and `"RESET" form -> form.editing`

### Requirement: Machine: The initial context
The initial context SHALL be the config's `context`, or `{}` when it has none or it is null; any other value that is not an object (`false`, `0`, `""`, a number, a string, a list) is kept as written, and an `assign` on it fails with `errc::implementation_failed`, where XState starts the machine with `{}` for a falsy one and merges the update into any other with `Object.assign` (which spreads a string into numbered members and drops a number). When `implementations::context` is set, the initial context is what it returns for the input, in place of the config's; a failure it returns, or a value that is not an object (`implementation_failed`), fails the initial macrostep on its first `next()` with the snapshot before it.

#### Scenario: The contexts that are not objects
- **WHEN** `example/xstate/diff_object_context.cpp` creates machines whose contexts are `null`, `false`, `5` and `"ab"`, each taking an `ADD` that assigns `x`, and one whose context function returns `"ab"`
- **THEN** it prints `context null: {}, after ADD {"x":1}`, `context false: false, after ADD error, implementation_failed`, `context 5: 5, after ADD error, implementation_failed`, `context "ab": "ab", after ADD error, implementation_failed` and `a context function that returns "ab": error, implementation_failed`

### Requirement: Machine: The definition
`to_json(const machine&)` SHALL return the machine's definition, key for key as XState's `machine.toJSON()` writes it: for each node its `id`, `key`, `type`, `initial`, `states`, `history`, `on`, `transitions`, `entry`, `exit`, `order` (the root's written `-1`, as XState writes `order || -1`), `invoke` and `tags`, with `meta`, `output` and `description` and the config's `version` when there are any, a node's `history` written as XState writes it (`true` as `"shallow"`, a falsy or absent one as `false`, any other value as given), targets resolved to lists of `#id` and the actions as the config listed them. It leaves out what XState's definition leaves out (the context, the eventless transitions and a history state's default target), a state's `meta` that is null and a `description` that is not a string (which creation drops), and it adds the higher-order guards where a transition's config holds them, which XState cannot write. The definition is built deepest first and its members may come in another order than XState's, so two definitions compare as JSON values, not as text.

#### Scenario: The toggle's definition
- **WHEN** `example/xstate/machines_definition.cpp` prints the definition of the toggle machine by JSON pointers
- **THEN** it prints `/id "toggle"`, `/order -1`, `/initial/target ["#toggle.inactive"]`, `/states/inactive/type "atomic"`, `/states/inactive/order 1`, `/states/inactive/on/toggle/0/target ["#toggle.active"]` and `/states/inactive/on/toggle/0/source "#toggle.inactive"`

### Requirement: Machine: Loading a definition back
`create_machine_from_definition(definition, implementations, context)` SHALL load a definition that `to_json` or XState wrote and return a `result<machine>`: a definition holds names, not code, and no context, so both are given again. It refuses with `invalid_config` a value that is not an object, and a state whose `states`, `on`, `invoke`, `entry` or `exit` is not of the JSON type a definition gives it (an object of state objects, an object of lists of transition objects, a list of invoke objects, a list of actions) or, in a state that has `states`, an `initial` whose first target is not a string; from any other object it rebuilds the config the definition describes and creates the machine with `create_machine`, which checks it as it checks any config. A compound state's `initial` is rebuilt from its first target, `#` and the id of a child, with the `actions`, `meta` and `description` of the written initial transition; a target that names none of the children leaves the state without `initial`, which is `invalid_config`. A node's `key`, `order` and `transitions` in the definition, and the `version` of a state below the root, are not read; the root's `version` becomes the machine's. The loader drops every entry or exit action of the type `xstate.raise` or `xstate.cancel`, since the raise and the cancel an `after` adds are written so. Given the original's context, a machine loaded from the definition of one that has no `always`, no history default target, no higher-order guard, no `meta` that is null and no custom action named `xstate.raise` or `xstate.cancel` takes the original's steps and writes the same definition.

#### Scenario: A toggle stored as text and loaded back
- **WHEN** `example/xstate/machines_reload.cpp` writes the toggle's definition, stores it as text, loads it back with its context `{"presses":0}` and steps it
- **THEN** it prints `"active" {"presses":0}` and then `true` for the equality of the two definitions

#### Scenario: The version survives
- **WHEN** `machine_test` creates a machine with `"version": "1.2"`, writes its definition, loads it back and writes the definition again
- **THEN** the two definitions are equal

#### Scenario: A definition that is not one never aborts
- **WHEN** `create_machine_from_definition` is given `["not", "a", "definition"]`, `{"id": "m", "entry": "notify", "states": {}}`, `{"id": "m", "exit": 5, "states": {}}` or `{"id": "m", "initial": {"target": [1]}, "states": {"a": {"id": "m.a"}}}`
- **THEN** each returns `invalid_config`

#### Scenario: A guard survives a definition XState did not write
- **WHEN** a transition guarded by `{"type": "xstate.not", "guards": ["isEmpty"]}` is written by `to_json` and loaded back
- **THEN** the loaded machine writes the same guard, which `diff_guards.cpp` prints as `{"type":"xstate.not","guards":["isEmpty"]}`

### Requirement: Implementations: The registry
`struct implementations` SHALL hold, by name, the code a machine runs that its JSON config cannot hold, as XState's `setup({ actions, actors, guards, delays })` registers it, in seven members: `context` (an optional `context_maker`, one function: the initial context from the input, in place of the config's `context`), `actions` (`std::map<std::string, action_implementation, std::less<>>`: the built-in action a name stands for), `actors` (`std::map<std::string, actor_implementation, std::less<>>`: what an invoke's or a spawnChild's `src` runs), `guards` (`predicate`, by guard name), `delays` (`delay_maker`, by delay name), `inputs` (`value_maker`, by invoke id: an invoke's input, computed when the state is entered) and `outputs` (`value_maker`, by final state id or by the root's id: an output). An empty `implementations`, `{}`, makes every action of the config a custom action and holds no guard, delay or actor. It is a value: a module that adds its own entries to another's copies it and inserts them. Creation checks the names the config uses (a guard, an `after` delay and an actor it does not find refuse the machine); an action name it does not hold is not an error but a custom action.

#### Scenario: A setup with two assigns
- **WHEN** `example/xstate/machines_setup.cpp` registers the `assign_action`s `increment` and `decrement` and sends `inc`, `inc`, `dec`
- **THEN** it prints `inc {"count":1}`, `inc {"count":2}` and `dec {"count":1}`

#### Scenario: A delay that is a function
- **WHEN** `example/xstate/machines_names.cpp` registers the guard `canAccess` and the delay `shortDelay` (250 ms) and leaves the action `trackAccess` unregistered
- **THEN** entering `checking` returns `xstate.raise {"event":{"type":"xstate.after.shortDelay.access.checking"},"id":"xstate.after.shortDelay.access.checking","delay":250}`, and leaving it returns `xstate.cancel` and the custom action `trackAccess`

### Requirement: Implementations: The function types
The library SHALL define seven function types, each returning a `result`, so that an implementation fails by returning an error and never by throwing: `assigner` (`std::function<result<boost::json::object>(const action_args&)>`, the partial update an assign merges), `event_maker` (`result<event>`, the event a raise, sendTo, sendParent or emit sends), `value_maker` (`result<std::optional<boost::json::value>>`, a value or none for JavaScript's `undefined`: it computes a log's value, a child's input, an invoke's input and an output), `predicate` (`result<bool>`), `context_maker` (`result<boost::json::value>(const boost::json::value& input)`), `delay_maker` (`result<std::uint64_t>(const action_args&)`) and `id_maker` (`result<std::string>`, a spawned child's id). A failure fails the macrostep that called it, which settles with the snapshot it began from, its status `error` and the failure as its `error`. An empty `std::function` held by the implementations refuses the machine at creation, since calling it would abort in a build without exceptions.

#### Scenario: A guard that cannot decide
- **WHEN** `example/xstate/transitions_guards.cpp` sends `submit` to a form with no `feedback` member, to a guard that reads it
- **THEN** it prints `no feedback -> "form" error implementation_failed`

#### Scenario: An optional function left out
- **WHEN** a `log_action{.value = std::nullopt, .label = "here"}` is registered
- **THEN** `create_machine` accepts it, and the log's value is the context and the event

### Requirement: Implementations: What an implementation is handed
Every implementation but `implementations::context` SHALL be handed an `action_args` built for the call, whose references live during the call: `context` (the context as it is when the implementation runs, after the assigns that ran before it in the microstep), `event` (the microstep's event; for the machine's output under the root's id, the done event of the final state that completed the machine) and `params` (the action's or the guard's `params` as the config wrote them, null when the config gave none, and for an input or an output). An implementation receives no actor and no system, where XState's does: an effect that needs them is a custom action, which the host performs, or an `emit_action`, whose `event_maker` reads the context and the event.

#### Scenario: Params travel with the name
- **WHEN** `example/xstate/actions_params.cpp` runs an action configured `{"type": "track", "params": {"response": "good"}}` and a second configured by the name `showConfetti`
- **THEN** it prints `track {"response":"good"}` and `showConfetti`

### Requirement: Implementations: delay_ref
`delay_ref` SHALL be `std::variant<std::uint64_t, std::string>`: the delay an action names is a number of milliseconds or the name of a delay of the registry, which computes it when the action resolves. A name the registry does not hold is no delay, as in XState: the raise is queued at once and the send is returned without a `delay`. An `after` key names a delay the same way, except that creation refuses one naming no registered delay (`unknown_delay`). Time is in whole milliseconds, a delay is a `std::uint64_t`, and a delay key is a number only when it is digits alone.

#### Scenario: A raise delayed by a name that is not registered
- **WHEN** a `raise_action` has `.delay = delay_ref{"unregistered"}`
- **THEN** the raise is queued at once and returned as `xstate.raise` without a `delay`

### Requirement: Implementations: The built-in actions
`action_implementation` SHALL be `std::variant<assign_action, raise_action, log_action, send_parent_action, cancel_action, spawn_child_action, stop_child_action, send_to_action, forward_to_action, emit_action>`, the built-in action a name in `implementations::actions` stands for. `assign_action{assignment}`; `raise_action{event, id, delay}`; `log_action{value, label}` (without a value maker the logged value is `{context, event}`); `send_parent_action{event, id, delay}`; `cancel_action{id}`; `spawn_child_action{src, id (a fixed string or an id_maker), system_id, input}` (an id left out is the empty string); `stop_child_action{id}`; `send_to_action{target, event, id, delay}`; `forward_to_action{target, id, delay}` (sends the microstep's event); `emit_action{event}`. They are aggregates, built with designated initializers, whose omitted optional members are empty.

#### Scenario: Designated initializers that leave members out
- **WHEN** `test/user_aggregates.cpp` builds `raise_action{.id = std::nullopt}`, `log_action{.label = std::nullopt}`, `spawn_child_action{.src = "child"}`, `send_to_action{.target = "#_parent"}` and `forward_to_action{.target = "#_parent"}` with `-Wextra -Werror`
- **THEN** it compiles and returns 0

### Requirement: Implementations: Actors a machine can name
`actor_implementation` SHALL be `std::variant<machine_actor, host_actor>`. A `machine_actor` is a machine run as a child actor with that machine's own implementations, built from a `machine` (`machine_actor{machine}`) and holding it through a `std::shared_ptr<const machine>` named `logic`, because a machine holds its implementations, which may hold this. A `host_actor` is an empty struct: it stands for XState's `fromPromise`, its work being done by the host, which answers it later through `actor_system::resolve` and `reject`. The machine core only names them; the actor layer runs them.

#### Scenario: A machine actor holds the machine it runs
- **WHEN** `machine_test` creates the machine `{"id": "child"}` and builds `machine_actor{*child}` from it
- **THEN** `actor.logic->id()` is `child`

### Requirement: Guards: Named guards and higher-order guards in JSON
A transition's `guard` SHALL be a name (a string) or an object `{"type": <name>, "params": ...}` naming an entry of `implementations::guards`, or one of four higher-order guards that XState writes as functions and xstate writes as JSON objects: `{"type": "xstate.not", "guards": [g]}` (exactly one operand), `{"type": "xstate.and", "guards": [...]}`, `{"type": "xstate.or", "guards": [...]}` and `{"type": "xstate.stateIn", "stateValue": v}`. `and` stops at its first false operand and `or` at its first true one, as `every` and `some` do, so an operand after that one never runs. `stateIn` holds when the snapshot matches its `stateValue` or, for a string `#id`, when the node with that id is active. The guards of a machine are kept in one flat list (`machine::guard(index)`) and a higher-order guard names its operands by index, so a guard nested as deep as its JSON is held and evaluated without recursion. `to_json` writes the higher-order guards, where XState's definition writes nothing.

#### Scenario: Higher-order guards over two regions
- **WHEN** `example/xstate/transitions_higher_order.cpp` sends `save` as an owner and a guest, `connect`, and `save` as an admin, to a document and network machine
- **THEN** it prints `save owner -> {"document":"dirty","network":"offline"}`, `connect -> {"document":"dirty","network":"online"}`, `save guest -> {"document":"dirty","network":"online"}` and `save admin -> {"document":"saved","network":"online"}`

#### Scenario: A malformed higher-order guard
- **WHEN** a config has an `xstate.not` guard with two operands, an `xstate.and` without `guards` or an `xstate.stateIn` without `stateValue`
- **THEN** creation returns `invalid_config`

### Requirement: Guards: Evaluation and failures
A guard SHALL be evaluated against the context, the event and the snapshot; a guard name the registry does not hold is refused when the machine is created (`unknown_guard`), a `stateIn` whose `#id` names no node fails the macrostep with `unknown_state`, and an error a predicate returns fails the macrostep, as it fails a call to `can`. Guards are evaluated in the order of the node's transitions and only until one is taken. A transition without a guard is always enabled.

#### Scenario: A stateIn guard with an id that names nothing
- **WHEN** a transition is guarded by `{"type": "xstate.stateIn", "stateValue": "#absent"}` and an event reaches it
- **THEN** the machine was created (`#id` is checked when used), and the macrostep settles in error with `unknown_state`

### Requirement: Actions: The action returned to the caller
The library SHALL return every action a macrostep resolves, except an `assign`, as `struct action { std::string type; boost::json::value params; bool builtin = false; std::optional<std::size_t> bound_at; bool forwarded = false; }`. A custom action (`builtin` false) has the `type` the config named and the `params` it gave, null when it gave none. A built-in one was resolved by xstate and its `params` holds what it resolved: `xstate.raise` `{event, id?, delay?}`, `xstate.cancel` `{sendId}`, `xstate.log` `{value?, label?}`, `xstate.sendTo` `{targetId, event, id?, delay?}`, `xstate.emit` `{event}`, `xstate.spawnChild` `{id, src, input?, systemId?}` and `xstate.stopChild` `{id}` (null when the child was not there). `builtin` tells the two kinds apart, not the type: a config may name an action `xstate.sendTo` and give it no implementation, and it is returned as a custom action all the same. `forwarded` is set for a forwardTo's sendTo. Actions are returned in the order XState returns them.

#### Scenario: A custom action comes back as data
- **WHEN** `example/xstate/concepts_actions.cpp` sends events to a dog machine whose actions `stretch`, `yawn` and `bark` (with `{"times": 2}`) have no implementation
- **THEN** it prints `value: "awake"`, `the caller executes stretch`, `the caller executes yawn` and `the caller executes bark {"times":2}`

#### Scenario: A test replaces a mock
- **WHEN** `example/xstate/testing_logic.cpp` collects the custom action `notify` with `{"message": "Active!"}` from a toggle that becomes active twice
- **THEN** it prints `value: "active"` and `notified: ["Active!","Active!"]`

### Requirement: Actions: The order of entry, exit and transition actions
A microstep SHALL run the exit actions of the exited states (descendants before their ancestors), then the actions of the transition, then the entry actions of the entered states (ancestors before their descendants), and return them in that order; a state entered by default also runs its initial transition's actions after its entry actions. Entering a state with invokes returns one `xstate.spawnChild` per invoke after the entry actions and before the initial actions; exiting returns one `xstate.stopChild` per invoke after the exit actions.

#### Scenario: The order of a transition into a final root state
- **WHEN** `example/xstate/actions_order.cpp` starts a machine with the entry and exit actions `rootEntry`/`rootExit`, `enterQuestion`/`exitQuestion`, `enterAsking`/`exitAsking`, `showConfetti`/`exitThanks` and sends `feedback.good` with the action `track`
- **THEN** it prints `initial: rootEntry enterQuestion enterAsking` and `feedback.good: exitAsking exitQuestion track showConfetti exitThanks rootExit`

### Requirement: Actions: assign
`assign_action` SHALL merge the partial update its assigner returns into the context as `Object.assign` does: each member replaces the context's member of that name and the others stay. An assign changes the context and is never returned. The microstep fails with `errc::implementation_failed` when the context is not an object, and with the assigner's own error when it fails.

#### Scenario: assign merges
- **WHEN** `example/xstate/context_assign.cpp` assigns `{"feedback": "Some feedback"}` over a context holding a rating of 5
- **THEN** it prints `{"feedback":"Some feedback","rating":5}`, then `{"feedback":"Some other feedback","rating":5}` and `0 actions`

### Requirement: Actions: raise, delayed raise and cancel
A `raise_action` without a numeric delay SHALL put the event in the macrostep's internal queue, where a later microstep of the same macrostep runs it, and be returned as `xstate.raise`; one with a delay SHALL be returned only, for the caller's clock. The params hold `event` (flat), `id` when it has one and `delay` in milliseconds when delayed. A `cancel_action{id}` is returned as `xstate.cancel` with `sendId`. A state with `after` raises `xstate.after.<delay>.<id>`, with that string as the event's type and the raise's id and the delay as its delay, on entry, and cancels it on exit, as XState does; the keys of `after` are read in the order JavaScript enumerates them, so an `after` that writes `"1000"` before `"500"` returns the raise of 500 first.

#### Scenario: A raise queued and a raise delayed
- **WHEN** `example/xstate/actions_raise.cpp` enters `loading`, whose entry raises `started` with `{"attempt": 1}`
- **THEN** it prints `"loading" xstate.raise {"event":{"type":"started","attempt":1}}` and then `"running"`

#### Scenario: A delayed transition returns its raise and its cancel
- **WHEN** `example/xstate/transitions_after.cpp` starts the game with `waitingForButtonPush` after 5000 ms and sends `PUSH_BUTTON`
- **THEN** the initial macrostep returns `xstate.raise {"event":{"type":"xstate.after.5000.game.waitingForButtonPush"},"id":"xstate.after.5000.game.waitingForButtonPush","delay":5000}` and `PUSH_BUTTON` returns `xstate.cancel {"sendId":"xstate.after.5000.game.waitingForButtonPush"}` and `logSuccess`

#### Scenario: Delayed raise and cancel by id
- **WHEN** `example/actors/actions_delayed.cpp` runs a `remind` action that raises `nudge` after 1000 ms under the id `nudge`, and a `dismiss` that cancels it
- **THEN** it prints `remind: xstate.raise {"event":{"type":"nudge"},"id":"nudge","delay":1000}` and `dismiss: xstate.cancel {"sendId":"nudge"}`

### Requirement: Actions: log
A `log_action` SHALL be returned as `xstate.log` with the params `value`, what the value maker returned, and `label` when it has one; without a value maker the value is `{context, event}`, and a value maker that returns none leaves `value` out.

#### Scenario: Logs with a value, a label and no maker
- **WHEN** `example/actors/actions_log.cpp` runs three log actions on `increment`
- **THEN** the host hears `xstate.log {"value":"incremented"}`, `xstate.log {"value":1,"label":"count"}` and `xstate.log {"value":{"context":{"count":1},"event":{"type":"increment"}}}`

### Requirement: Actions: sendTo, sendParent and forwardTo
`send_to_action` SHALL be returned as `xstate.sendTo` with `{targetId, event, id?, delay?}`; its target is a string as XState reads one: a child's id or `#_<id>`, `#_parent`, `#_internal` (the actor itself) or `#system:<systemId>` (the actor registered under it). `send_parent_action` is a sendTo to `#_parent`, resolved even for a machine without a parent, where XState's pure functions throw (the machine core returns it as XState returns it for an actor that has a parent; the actor layer fails a root that sends to its parent with `unknown_target`). `forward_to_action` sends the microstep's event, with `action::forwarded` set, and fails with `unknown_target` for an empty target. The microstep fails with `errc::unknown_target` when the target names a child the snapshot does not hold, unless an entry or initial action names, by its bare id, an invoke of the state being entered; `#_parent`, `#_internal` and `#system:` targets are always accepted by the core, which delivers nothing and leaves that to its caller or the actor layer.

#### Scenario: sendParent in the pure functions
- **WHEN** `example/xstate/diff_send_parent.cpp` sends `FINISH` to a machine whose transition has a `send_parent_action` of `DONE`
- **THEN** it prints `xstate.sendTo {"targetId":"#_parent","event":{"type":"DONE"}}`

#### Scenario: A system id is returned for its caller to resolve
- **WHEN** a `send_to_action` with the target `#system:notifier` and the event `NOTE` runs on `GO`
- **THEN** the actions are one `xstate.sendTo` with `{"targetId": "#system:notifier", "event": {"type": "NOTE"}}` and the snapshot stays `active`

### Requirement: Actions: spawnChild and stopChild
`spawn_child_action` SHALL resolve by adding the child `(id, src)` to the snapshot's `children`, a child already there under that id taking the new `src`, and returning `xstate.spawnChild`; its id is fixed or computed when the action resolves, its input computed after, and a failure of either fails the macrostep with no child. `stop_child_action` SHALL remove the child from the snapshot's `children` and return `xstate.stopChild` with `{id}`, or null params when the snapshot held no such child; both change the snapshot (even a stop of a child that is not there), so an eventless microstep whose only effect is one selects the eventless transitions again, and one that keeps spawning or stopping never settles. The `children` list is `(id, src)` in the order JavaScript enumerates XState's children object: ids that are array indices first, by number, then the others in spawning order. XState keys a child spawned without an id `"undefined"` and a stopChild then leaves it there; here an id left out is `""`, and a stop of `""` removes it.

#### Scenario: A spawn with an input that fails
- **WHEN** a `spawn_child_action` whose input maker returns `implementation_failed` runs on `GO`
- **THEN** the snapshot settles in error with `implementation_failed` and `children` is empty

#### Scenario: Two children without an id
- **WHEN** `example/actors/diff_spawn_id.cpp` spawns two children without an id, each with a system id
- **THEN** it prints `[""]`, `first: active, keyed: false` and `second: active, keyed: true`

### Requirement: Actions: emit
`emit_action` SHALL be returned as `xstate.emit` with `{event}`; the actor layer hands it to the inspector and to the listeners of `actor_system::on_emitted`, and the core delivers it to no one.

#### Scenario: Emitted events heard by listeners
- **WHEN** `example/actors/actions_emit.cpp` runs a machine that emits `someStaticEvent` with `{"data": 42}` and a dynamic one
- **THEN** it prints `action: track`, `static: {"type":"someStaticEvent","data":42}`, `any: someStaticEvent` and `any: someDynamicEvent`

### Requirement: Actions: A sendTo bound to an invoke of the state being entered
A `send_to_action` whose target is the bare id of an invoke of the state being entered SHALL carry `action::bound_at`: how many of its microstep's actions come before the point where XState binds it to the child that id names, the end of that state's entry actions, spawns and initial actions (XState's `retryResolveSendTo`); no other action has one. When no child has that id at that point, the actor layer fails the actor with `unknown_target` where the send is delivered, where XState relays the event to `undefined` and throws a `TypeError`.

#### Scenario: Bound after the state's actions
- **WHEN** `cursor_test` enters `b`, whose entry is `ping_n` and `hello` and whose invoke has the id `n`, and whose initial child's entry is `later`
- **THEN** four actions are returned and the first, `xstate.sendTo`, has `bound_at` 3, while none of the others has one

### Requirement: Actions: Values that may be undefined
A value an implementation computes (a log's value, an invoke's or a spawn's input, a final state's or the machine's output) SHALL be allowed to be none, JavaScript's `undefined`, and the member is then left out, as XState leaves it out: JSON has `null` only, and the event `xstate.init` carries `input` only when the input is not null, so a null input and no input are the same.

#### Scenario: undefined is not null
- **WHEN** a final state's output maker returns none for one context and `null` for another
- **THEN** the first snapshot has no `output` and the second has `output` null

### Requirement: Snapshot: Its members
A snapshot SHALL be a plain value to keep, copy or store (`struct snapshot`), with these members: `value` (XState's state value, computed from `nodes`), `context`, `status` (`active`, `done` or `error`), `output` (an `std::optional<boost::json::value>`, absent when the machine has none, JavaScript's `undefined`), `error` (a `boost::system::error_code`, empty unless the status is `error`), `error_value` (with the error `actor_failed`, the failed actor's error when its error event carried one), `nodes` (the active nodes by index, in XState's order), `history` (what each history node remembers, by its id, in recording order), `tags` (the tags of the active nodes, each once, in the order of `nodes`) and `children` (the `(id, src)` of the child actors the microsteps spawned and have not stopped since). A default-constructed snapshot is no machine's: a program starts from `get_initial_snapshot` or a cursor. XState's fourth status, `stopped`, has no counterpart here: XState gives it to a snapshot that receives `xstate.stop`, while xstate runs that as any other event, and `actor_system::status_of` tells it for an actor, as a `webcpp::xactor::status`.

#### Scenario: A snapshot read
- **WHEN** `example/xstate/concepts_snapshot.cpp` prints the snapshot of the walk machine after `leave home`
- **THEN** it prints `value: {"onAWalk":"walking"}`, `context: {"dog":"Rex"}`, `status: active`, `tags: ["outside"]`, `matches onAWalk: true`, `has tag outside: true`, `can arrive home: true` and `can leave home: false`

#### Scenario: A snapshot of a machine that is done
- **WHEN** `example/xstate/states_snapshot.cpp` sends `submit` to the feedback form until it is done
- **THEN** it prints `value: "thanks"`, `status: done`, `nodes: ["feedback.thanks","feedback"]`, `tags: []` and `output: {"sent":true}`

### Requirement: Snapshot: matches, has_tag and remembered
`snapshot::matches(state_value)` SHALL be `matches_state` with the snapshot's value as the child; `has_tag(std::string_view)` whether `tags` holds the tag (XState's `hasTag`); and `remembered(std::string_view history_id)` a pointer into `history`, or `nullptr` when the node has not recorded.

#### Scenario: Tags follow the active states
- **WHEN** `example/xstate/states_tags.cpp` runs a machine through `prompt`, `{"thanks":"celebrating"}` and `closed`
- **THEN** it prints `"prompt" ["visible"] visible: true`, `{"thanks":"celebrating"} ["visible","confetti"] visible: true` and `"closed" ["hidden"] visible: false`

### Requirement: Snapshot: The order of the active nodes and the keys of a value
`snapshot::nodes` SHALL be an insertion-ordered set: kept nodes keep their place, exited nodes leave it and entered nodes are appended in document order; a microstep that ends with the same active nodes and history keeps the order it had, as XState returns its snapshot unchanged, and any other microstep that leaves the machine done lists them by descending `order`, each after its descendants, as XState sorts its array of active nodes in place when the machine is done. That order decides which eventless transition a parallel region takes first. A state value's keys are in the order JavaScript enumerates them, as XState's value is an object: array indices first, by number, then the rest in the order of the nodes. `node_set` SHALL be the ordered set the library computes with: `insert` (a node already in is not added again), `erase` (a node not in is no error), `contains`, `size`, `empty`, `at`, `begin`/`end` and `ordered()`, a `const std::vector<std::size_t>&` valid until the set changes.

#### Scenario: Array-index regions come first
- **WHEN** `machine_test` computes the initial snapshot of a parallel root with regions `z` (children `z1`, `z2`) and `3` (children `c`, `d`)
- **THEN** its value serializes as `{"3":"c","z":"z1"}`, and `resolve_state` of `{"z": "z2", "3": "d"}` serializes as `{"3":"d","z":"z2"}`

#### Scenario: The set keeps the order of insertion
- **WHEN** nodes 4, 2, 4 and 7 are inserted into a `node_set` and 2 is erased
- **THEN** `ordered()` is `{4, 7}` and `contains(2)` is false

### Requirement: Snapshot: get_meta
`get_meta(const machine&, const snapshot&)` SHALL return the `meta` of every active node that has one, as an object keyed by node id, its members in the order of the snapshot's `nodes` (XState's `snapshot.getMeta()`); a null `meta` is none and is not listed.

#### Scenario: The meta of the active states
- **WHEN** `example/xstate/states_meta.cpp` prints `get_meta` for the feedback machine in `prompt` and in `thanks`, and the description of a state
- **THEN** it prints `{"feedback":{"title":"Feedback"},"feedback.prompt":{"content":"How was your experience?"}}`, `{"feedback":{"title":"Feedback"},"feedback.thanks":{"content":"Thank you for your feedback!"}}` and `Waits for the user to rate the experience.`

### Requirement: Snapshot: resolve_state
`resolve_state(const machine&, const boost::json::value& state_value, boost::json::value context)` SHALL return a `result<snapshot>` for a state value (XState's `machine.resolveState({ value, context })`), completed with a compound state's initial child, a parallel state's regions and every ancestor. The snapshot's status is `done` when the value is in a final state at the top level and `active` otherwise, and it has no output, no history and no children. Its `nodes` are in the order XState's `resolveState` gives them, which for a parallel state differs from a microstep's: each level's named children before their descendants, so the done value `{"a": "a2", "b": "b2"}` of a parallel root `p` gives `p`, `p.a`, `p.b`, `p.a.a2`, `p.b.b2`. The keys of an object value are read in the order JavaScript enumerates them, a string value names a child of the root by its key (`"a.b"` is the key `a.b`, not a path) and a nested value is an object. A value that names a state the machine does not have, or is neither a string nor an object, is `errc::unknown_state`.

#### Scenario: A persisted snapshot comes back
- **WHEN** `example/xstate/pure_persisted.cpp` restores `"yellow"` with the context `{"cycles":0}` and sends the timer event
- **THEN** it prints `restored: "yellow"`, `context: {"cycles":0}` and `next: "red"`

#### Scenario: The nodes a value implies
- **WHEN** `example/xstate/pure_state_nodes.cpp` resolves `{"on":{"track":"paused","volume":"normal"}}` and `{"on":{"track":"playing","volume":"normal"}}`
- **THEN** it prints ` player player.on player.on.track player.on.track.paused player.on.volume player.on.volume.normal` and ` player player.on player.on.track player.on.volume player.on.volume.normal player.on.track.playing`

#### Scenario: A value naming no state
- **WHEN** `resolve_state` is given `{"nothing": "x"}`
- **THEN** it returns `unknown_state`

### Requirement: Functions: initial_transition and its relatives
The library SHALL port XState's pure functions as the macrostep cursor run to its end, without a fuel bound: `get_initial_microsteps(machine, input = null)` (`getInitialMicrosteps`) returns every microstep of the initial macrostep; `get_initial_snapshot(machine, input = null)` (`getInitialSnapshot`) the settled snapshot, a machine whose initial macrostep fails coming back in a snapshot whose status is `error`; and `initial_transition(machine, input = null)` (`initialTransition`) the settled snapshot and every action of the macrostep, as a `std::pair<snapshot, std::vector<action>>`. A caller checks the snapshot's status before it executes the actions: of an initial macrostep that failed, XState's actor has executed none. They return no error: a machine that fails comes back in a snapshot whose status is `error`.

#### Scenario: The initial transition
- **WHEN** `example/xstate/pure_transition.cpp` calls `initial_transition` and `transition` on a machine whose `started` event runs `doSomething`
- **THEN** it prints `initial value: "pending"`, `initial actions: []`, `next value: "started"` and `actions: [doSomething]`

#### Scenario: The microsteps of the initial state
- **WHEN** `example/xstate/pure_initial_microsteps.cpp` runs `get_initial_microsteps` on a machine whose `a` leads to `b` by an eventless transition with the actions `enterA`, `aToB` and `enterB`
- **THEN** it prints `"a" [enterA]` and `"b" [aToB, enterB]`

### Requirement: Functions: transition and its relatives
`transition(machine, from, event)` (`transition`) SHALL return the snapshot a macrostep leads to and its actions as a pair, `get_next_snapshot` (`getNextSnapshot`) the settled snapshot alone, and `get_microsteps` (`getMicrosteps`) every microstep, each running `begin` to its end without a bound. `from` must be a snapshot of the machine. A machine that fails comes back in a snapshot with the status `error`, where XState's `transition` throws; the pair's actions are those of the microsteps that completed before the failure, and a caller checks the status before it executes them (of a macrostep that failed, XState's actor has executed only the custom actions and logs). A machine whose eventless transitions never settle never returns from them.

#### Scenario: The next snapshot
- **WHEN** `example/xstate/pure_next_snapshot.cpp` sends the timer event four times to the traffic light
- **THEN** it prints `"green"`, `"yellow"`, `"red"` and `"red"`

#### Scenario: A failure inside the pure functions
- **WHEN** `example/xstate/pure_failure.cpp` sends an event whose second action fails
- **THEN** it prints `ran notify`, `ran progress`, `status: error (implementation_failed)`, `value: "idle"` and `context: {"attempts":0}`

#### Scenario: The microsteps of an event
- **WHEN** `example/xstate/pure_microsteps.cpp` sends an event to a machine that raises and takes eventless transitions
- **THEN** it prints `"second" [xstate.raise]`, `"third" []` and `"fourth" []`

### Requirement: Functions: can
`can(machine, from, event)` (`snapshot.can(event)`) SHALL return a `result<bool>`: whether the event would take, from the snapshot, a transition that does something, that is one with a target or actions, evaluating the guards as XState does and running no action. A failure comes back as the error: a guard's own, or `errc::unknown_state` when the value of `from`, or a `stateIn` guard's `#id`, names no state of the machine.

#### Scenario: What a form can do
- **WHEN** `example/xstate/pure_can.cpp` asks `can` for `SUBMIT`, `TYPE`, `IGNORE` and `UNKNOWN`, then for `SUBMIT` after `TYPE`
- **THEN** it prints `SUBMIT: false`, `TYPE: true`, `IGNORE: false`, `UNKNOWN: false` and `after TYPE, SUBMIT: true`

### Requirement: Functions: get_next_transitions
`get_next_transitions(machine, from)` (`getNextTransitions`) SHALL list, as pointers into the machine, every transition a snapshot could take, guards not evaluated: for each active atomic (or final) node its own transitions and its ancestors', each node once, a node's event transitions first, in the order of `state_node::transitions`, and its eventless ones after. A type may come more than once, from several guarded transitions of a state or from a state and its ancestor, where XState's event descriptors are a set; an active atomic state's transitions come before its ancestors'.

#### Scenario: Walking a machine
- **WHEN** `example/xstate/testing_paths.cpp` walks the machine breadth first, sending one event of each type offered
- **THEN** it prints `"idle": [xstate.init]`, `"running": [xstate.init, start]` and `"paused": [xstate.init, start, pause]`

### Requirement: Cursor: begin_initial and begin
`begin_initial(const machine&, const boost::json::value& input = nullptr)` and `begin(const machine&, snapshot from, event)` SHALL make the cursor `macrostep` of the initial macrostep and of the macrostep an event runs from a snapshot, running no microstep yet; a `macrostep` has no public constructor. The initial macrostep runs the initial microstep, which enters the initial states from a snapshot whose `nodes` hold the root alone (`began_from()`), then XState's loop for the event `xstate.init`, which carries `input` as its member `input` when it is not null; its initial context is `machine.context()` or what `implementations::context` returns for the input. The first `next()` of `begin` runs the external event's microstep; an event that no transition takes leaves the snapshot unchanged and reports an empty list of transitions, and the macrostep settles there unless the event enables an eventless transition, which a guard that reads the event can do.

#### Scenario: The initial macrostep with an input
- **WHEN** `example/xstate/pure_begin_initial.cpp` begins a machine with the input `{"user":"ana"}` and calls `next` until it settles
- **THEN** it prints `{"type":"xstate.init","input":{"user":"ana"}}, none, [], "checking"` and `{"type":"xstate.init","input":{"user":"ana"}}, checking -> signedIn, [welcome], "signedIn", settled`, then `context: {"user":"ana"}`

### Requirement: Cursor: One microstep per call, and settled on one only
Each call of `macrostep::next()` SHALL run exactly one microstep and return a `progress{step, settled, resolved_before_failure}`: the first call runs the external event's microstep or, for the initial macrostep, the initial one, and each later call runs one iteration of XState's macrostep loop (an enabled eventless transition first, a raised event otherwise). `progress::settled` MUST be true on the microstep after which XState's macrostep loop would stop, and on no other: when no eventless transition is enabled and no raised event is queued, or the status is not `active`. The cursor selects the eventless transitions XState's next iteration would take right after each microstep, so `settled` is known on the microstep that closes the macrostep, and the next call does not select them again. Once the macrostep has settled, `next()` runs nothing and returns a settled `progress` with the current snapshot, no list of transitions and no actions. `done()`, `current()` (the snapshot as the last microstep left it), `began_from()` and `result()` (a `macrostep_result{snapshot, actions, microsteps}`, meaningful once `done()`) read the cursor.

#### Scenario: Eight microsteps and one mark
- **WHEN** `cursor_test` runs the event `GO` (idle to `a`, `a` to `b` whose entry raises `PING`, `PING` to `c`, then `c` and `loop` twice while `count` reaches 2, then `c` to `c2`) with `next()` until `done()`
- **THEN** `settled` is false on microsteps 1 to 7 and true on the 8th, `result().microsteps` is 8 and the snapshot's value is `c2`

#### Scenario: An event no transition takes
- **WHEN** `cursor_test` begins the event `NOTHING`
- **THEN** one microstep runs and is marked `settled`, the snapshot equals the initial one and `result().actions` is empty

#### Scenario: The cursor of a request
- **WHEN** `example/xstate/pure_cursor.cpp` runs `CHECKOUT`, `CHARGED` through a cursor, printing each `next()`
- **THEN** it prints `1: CHECKOUT, idle -> validating, [track], "validating"`, `2: CHECKOUT, validating -> charging, [xstate.raise], "charging"`, `3: CHARGED, charging -> confirmed, [sendReceipt], "confirmed", settled` and `result: "confirmed", [track, xstate.raise, sendReceipt], 3 microsteps`

### Requirement: Cursor: A copy resumes where the original stopped
A `macrostep` SHALL be a value: it holds a copy of its machine (a handle), of its snapshots and of its queue, so a copy resumes exactly where the original stopped, apart from it. The core never stops a macrostep on its own: an `always` cycle never settles, and the caller's fuel is the only bound; a caller that runs out of fuel keeps the cursor and resumes it later, perhaps in a later request.

#### Scenario: Stopping anywhere
- **WHEN** `cursor_test` stops a cursor of `GO` after 1, 2, 3, 5 and 7 microsteps, copies it and finishes both
- **THEN** the cursor and its copy settle in the result a straight run gives

#### Scenario: Fuel and a copy
- **WHEN** `example/xstate/pure_fuel.cpp` runs a cursor for two microsteps, copies it, runs the original for two more and the copy for two
- **THEN** it prints `first request: 2 microsteps, done false, at "charging"`, `second request: 1 microstep, done true, at "confirmed"` and `the copy: done true, at "confirmed", 3 microsteps`

#### Scenario: A cycle is bounded by the caller
- **WHEN** `example/xstate/pure_cycle.cpp` gives `SPIN` a fuel of 1000 microsteps
- **THEN** it prints `out of fuel after 1000 microsteps, at "right"` and `the stable state is still "idle"`

### Requirement: Cursor: Eventless transitions are selected again only after a change
Eventless transitions SHALL be selected after the first microstep of a macrostep, the initial one or the external event's, and after every microstep of a raised event, whatever it changed. After a microstep of eventless transitions they are selected again only when it changed the snapshot: the active nodes, the history, the context (an `assign` ran), the children (a spawn or a stop resolved, even a stop of a child that is not there) or the status. This is XState's comparison of snapshots by identity, stated as a rule: its loop selects them again when the microstep returned another snapshot than the one it began from. An eventless microstep whose only effect is a custom action changes nothing, so the macrostep settles there.

#### Scenario: A custom action does not repeat
- **WHEN** `cursor_test` takes `GO` to a state whose eventless transition has no target and the action `tick`
- **THEN** the macrostep takes 2 microsteps, the last marked settled, and returns the action `tick` once

#### Scenario: A spawn or a stop repeats
- **WHEN** the same eventless transition runs a `spawn_child_action` or a `stop_child_action`
- **THEN** the cursor does not settle in 100 microsteps

### Requirement: Cursor: A wildcard event from outside is refused
A macrostep that takes an event of the type `*` from outside SHALL fail with `errc::invalid_event`, as XState's development build refuses it ("An event cannot have the wildcard type ('*')"), while an event of that type that the machine raises itself takes the wildcard transitions. The failed macrostep settles with the snapshot it began from, its status `error`.

#### Scenario: The wildcard example
- **WHEN** `example/xstate/transitions_descriptors.cpp` sends the type `*` to the feedback machine
- **THEN** it prints `* -> invalid_event`, where the twin on XState prints `* -> An event cannot have the wildcard type ('*')`

### Requirement: Semantics: Event descriptors
A transition's event descriptor SHALL match an event type when it is equal to it, is the wildcard `*` (which matches every type, the machine's own included), or is a partial descriptor, a prefix and then `.*`, whose dot-separated tokens equal the event type's leading tokens: `feedback.*` matches `feedback`, `feedback.good` and `feedback.bad.rude`, and not `feedbacks`. A `*` is a wildcard only as the whole descriptor, or as the last token after a dot when no earlier token is `*` itself; a `*` inside a longer token is a plain character (`mou*se.*` is a partial descriptor and matches `mou*se.x`), and any other descriptor that holds a `*` (`mouse*`, `*.click`, `mouse.*.click`, `mouse.*.*`) matches only an event of exactly that type. xstate writes no warning about any of them, where XState's development build writes a console warning about a descriptor that ends in `.*` and holds another `*`. One event may match several descriptors of one state; their transitions are tried in this order: those of the descriptor equal to the event's type, then those of each matching wildcard descriptor, the longest first (`feedback.bad.*` before `feedback.*` before `*`); the first enabled transition is taken, and only when none is does the search go up to the parent, the deepest active state being tried first, so a `*` transition of an active child takes the `xstate.after...` event of its parent's delayed transition before the parent sees it. A raised event of the type `*` is the descriptor `*` exactly, so `*` transitions take it and no partial descriptor matches it.

#### Scenario: Descriptors
- **WHEN** `transitions_descriptors.cpp` sends `feedback.good`, `feedback.bad`, `feedback.bad.rude`, `feedback.meh`, `feedback`, `feedbacks` and `mouse.click` to a state with the transitions `feedback.good` (to `thanks`), `feedback.*` (to `form`), `feedback.bad.*` (to `apology`) and `*` (to `other`)
- **THEN** it prints `feedback.good -> "thanks"`, `feedback.bad -> "apology"`, `feedback.bad.rude -> "apology"`, `feedback.meh -> "form"`, `feedback -> "form"`, `feedbacks -> "other"` and `mouse.click -> "other"`

### Requirement: Cursor: Failures settle in error with the snapshot the macrostep began from
An implementation that fails, or a refusal the cursor makes, SHALL set the snapshot's `status` to `error` with the error code and settle the macrostep at once with the snapshot it began from, on the failing call: the failing microstep reports no actions, as XState's pure functions throw, and the macrostep's result holds none of them. `progress::resolved_before_failure` holds the actions the failing microstep resolved before the failure, which a running actor of XState has run in part. The failure is an implementation's own code or one of: `unknown_target` (a sendTo names no actor, or a microstep needs the default of a history state that is the machine's root), `implementation_failed` (an `assign` runs on a context that is not an object, or the context function's value is not one), `invalid_event` (an external event of the type `*`), `unknown_state` (the snapshot's value or a `stateIn` guard's `#id` names no state). When the failure comes from a guard of an eventless transition, which the cursor evaluates after a microstep to know whether it settled, the call reports the microstep it ran, with its transitions and actions, and the failed snapshot.

#### Scenario: A failure in the middle of an event
- **WHEN** `cursor_test` sends `BREAK` (`go` then `fail`) and `BREAK_GUARD` (a failing guard)
- **THEN** each first `next()` is `settled`, `done()` is true, the status is `error` with the message `implementation_failed`, and the value, context and nodes are the initial snapshot's

#### Scenario: What the failing microstep resolved
- **WHEN** `BREAK` runs
- **THEN** `made.step.actions` is empty, `resolved_before_failure` holds one action of the type `go` and `result().actions` is empty

#### Scenario: A failing guard of an eventless transition
- **WHEN** `example/xstate/pure_guard_failure.cpp` takes an event whose actions are `note`, an emit and a log, and whose eventless guard fails
- **THEN** it prints `step: [note, xstate.emit, xstate.log], settled`, `ran note`, `logged: say`, `status: error (implementation_failed)` and `value: "idle"`

### Requirement: Cursor: An error event no transition handles
An external event whose type starts with `xstate.error.actor` (a prefix, as XState's `isErrorActorEvent` reads it) and that selects no transition SHALL settle in one microstep, with an empty list of transitions and no action: the snapshot the macrostep began from, its status `error`, its error `actor_failed` and its `error_value` the event's member `error`, when it has one. Its children stay, and no `xstate.stopChild` is returned.

#### Scenario: A child's error reaches a machine with no onError
- **WHEN** `xstate.error.actor.checkout` with `{"error": {"code": "declined"}}` reaches a machine with no transition for it
- **THEN** the snapshot is in error with `actor_failed` and its `error_value` is `{"code":"declined"}`

### Requirement: Cursor: A macrostep that does not end active stops its children
A macrostep whose snapshot is not active when it settles, done or still in the error it began from, SHALL return, after every microstep's actions, one `xstate.stopChild` per child left, in `macrostep_result::actions` and in no microstep; the snapshot keeps its children (XState's `stopChildren`, whose snapshot XState drops). A macrostep that the error-event rule settles, or one that fails, returns none.

#### Scenario: A done machine stops its children
- **WHEN** a machine with a running child reaches its top-level final state
- **THEN** `result().actions` ends with `xstate.stopChild` for the child, and the final snapshot still lists it in `children`

### Requirement: Cursor: Starting with an input
A machine SHALL start with an input, null when none. The event `xstate.init` carries a non-null input as its member `input`. When `implementations::context` is set, the initial context is what it returns for the input; a failure, or a value that is not an object, fails the initial macrostep on its first `next()`, with the snapshot before it (XState's `_getPreInitialState`).

#### Scenario: A machine whose context needs its input
- **WHEN** `example/xstate/pure_input.cpp` starts a machine with an input and then without one
- **THEN** it prints `value: "pending"`, `context: {"count":0}` and `status: active`, then `without input: error, context {}`

### Requirement: Cursor: microstep and macrostep_result
A `microstep` SHALL hold the `event` it ran for (the macrostep's, a raised one, or the last one again for an eventless transition), its `transitions` (an optional list of pointers into the machine: none for the initial microstep and for one that failed, an empty list for an event no transition takes), its `actions` and the `snapshot` it left. A `macrostep_result` holds the stable `snapshot`, every `action` the microsteps returned in order, followed by the `xstate.stopChild` of each child left when it settles not active, and how many `microsteps` ran, a failing one excluded. When the macrostep fails, the snapshot is the one it began from with the status `error`, the actions are those of the microsteps that completed before the failure and `microsteps` counts them.

#### Scenario: The transitions of the initial microstep
- **WHEN** `example/xstate/pure_begin_initial.cpp` prints the first microstep's transitions
- **THEN** it prints `none`

### Requirement: Clock: simulated_clock
`simulated_clock` SHALL be a value the caller owns, holding the delayed events a machine returned until the time the caller sets reaches them, in whole milliseconds from 0: `now()`; `schedule(event, delay, id)` (a later schedule under the same id takes the id over: the earlier event still comes due, but a cancel of the id no longer finds it, as in XState's scheduler); `apply(action)`; `cancel(id)`; `set(time)` (a `result<void>`: `errc::invalid_config` when `time` is earlier than `now()`, which changes nothing, where XState's `SimulatedClock.set` throws); `increment(milliseconds)`; and `pop_due()`, which takes out the earliest event whose time has come, by deadline and then by the order the events were scheduled in (a rule where XState leaves equal deadlines to its sort), without a cancel finding it afterwards. The machine never reads a clock: a caller schedules what a step returned, moves the time, and delivers each due event as a macrostep of its own, applying what that returns before it takes the next, so a delivered event can cancel a later one.

#### Scenario: The light on the simulated clock
- **WHEN** `example/xstate/pure_clock.cpp` applies the actions of the light's steps, moves the clock to 500, 1000, 1200 and 2000, sends `STOP` at 1200 and delivers each due event
- **THEN** it prints `at 500:`, `at 1000:` followed by `  xstate.after.1000.light.green -> "yellow"`, `at 1200:` followed by `  STOP -> "red"`, and `at 2000:`

#### Scenario: Time never goes back
- **WHEN** `set(5)` is called on a clock at 10
- **THEN** it returns `invalid_config` and `now()` stays 10

### Requirement: Clock: What apply takes from an action
`simulated_clock::apply` SHALL schedule a built-in `xstate.raise` with a numeric `delay` (under its `id` when it has one), apply a built-in `xstate.cancel` by its `sendId`, and hold the id of a delayed built-in `xstate.sendTo` with an id (a sendParent's included) until it comes due without an event to deliver, as XState's scheduler keys every delayed event by its id; a raise without a delay, which its macrostep already ran, and every other action are ignored. A custom action (`builtin` false) is ignored whatever its type, a config being free to name one `xstate.raise`.

#### Scenario: Only a built-in raise is scheduled
- **WHEN** `cursor_test` applies an `xstate.raise` action with `builtin` false and `{"event": {"type": "nudge"}, "delay": 10}`, increments 10, then applies the same with `builtin` true and increments 10
- **THEN** the first `pop_due()` is empty and the second returns the event `nudge`

#### Scenario: Time as an example
- **WHEN** `example/xstate/testing_time.cpp` runs the traffic light on a `simulated_clock` to 500 ms and then 1010 ms
- **THEN** it prints `after 500 ms: "green"` and `after 1010 ms: "yellow"`

### Requirement: Semantics: Transition targets and the transition domain
A transition SHALL resolve its target as XState does: an id (`#id`) names any state; a name names a sibling of the transition's source, or a state below the sibling by a dotted path; a path starting with a dot names a state below the source (`".child"`); an empty target names the source's parent. A transition with several targets (a list, as in a parallel state) enters each. A transition between siblings exits the source and enters the target, and a targetless transition runs its actions only.

#### Scenario: Targets of every shape
- **WHEN** `example/xstate/transitions_targets.cpp` sends `next` three times, `reset` and `deep`
- **THEN** it prints `start "a"`, `next -> {"b":"one"}`, `next -> {"c":"child"}`, `next -> {"c":"other"}`, `reset -> "a"` and `deep -> {"b":"two"}`

#### Scenario: Several targets in parallel regions
- **WHEN** `example/xstate/transitions_parallel_targets.cpp` sends `set.dark.custom`, `set.light`, `set.dark.custom` and `mode.light`
- **THEN** it prints `start {"mode":"light","theme":"default"}`, `set.dark.custom -> {"mode":"dark","theme":"custom"}`, `set.light -> {"mode":"light","theme":"default"}`, `set.dark.custom -> {"mode":"dark","theme":"custom"}` and `mode.light -> {"mode":"light","theme":"custom"}`

### Requirement: Semantics: reenter and self-transitions
The library SHALL follow XState's rules for transitions whose target is the source state itself or lies within it: a transition to a child (`.child`) exits and enters the child alone and leaves the source, unless `reenter` is true, which exits the source too and enters it again; a transition whose target is its own source exits the active descendants and enters the initial ones without leaving the source, unless `reenter` is true; a targetless transition runs only its actions.

#### Scenario: The reenter table
- **WHEN** `example/xstate/transitions_reenter.cpp` sends `event.thatReenters`, `event.targetless`, `event.self`, `event.normal` and `event.selfReenters`
- **THEN** it prints `start {"parentState":"someChildState"} [enterParent, enterSome]`, `event.thatReenters -> {"parentState":"otherChildState"} [exitSome, exitParent, enterParent, enterOther]`, `event.targetless -> {"parentState":"otherChildState"} [notify]`, `event.self -> {"parentState":"someChildState"} [exitOther, enterSome]`, `event.normal -> {"parentState":"someChildState"} [exitSome, enterSome]` and `event.selfReenters -> {"parentState":"someChildState"} [exitSome, exitParent, enterParent, enterSome]`

### Requirement: Semantics: Selecting a transition
A state SHALL pick the first transition whose guard holds among those that match the event, tried from the innermost active state outward, and a transition that matches stops the search even when it has no target: an enabled targetless transition of a child hides its ancestors' transitions for the same event.

#### Scenario: A child shadows its parent
- **WHEN** `example/xstate/transitions_selection.cpp` sends `escape` in `editing.menu`, in `editing.text`, and in `editing.saving` whose own `escape` is an empty transition
- **THEN** it prints `escape -> {"editing":"text"}` from the menu, `escape -> "closed"` from the text, and `escape -> {"editing":"saving"}` from the saving state

### Requirement: Semantics: Eventless transitions
An `always` transition SHALL be taken as soon as its guard holds, in the same macrostep: after each microstep the machine selects the eventless transitions again until none is enabled, and a cycle of them never settles.

#### Scenario: A kettle
- **WHEN** `example/xstate/transitions_eventless.cpp` sends `temp.update` 105, `boil` and `temp.update` 90
- **THEN** it prints `temp.update: "lukewarm" []`, `boil: "heating" [] "boiling" [turnOffLight]` and `temp.update: "boiling" [] "heating" []`

#### Scenario: A cycle
- **WHEN** `example/xstate/transitions_eventless_cycle.cpp` pays for five microsteps of a `ping` and `pong` cycle
- **THEN** it prints `"ping"`, `"pong"`, `"ping"`, `"pong"`, `"ping"` and `not settled after 5 microsteps`

### Requirement: Semantics: Final states, done events and the machine's output
A final state SHALL make its parent done: entering one raises `xstate.done.state.<parent id>` with the final state's `output` when it has one, which the parent's `onDone` takes; a parallel state is done when all its regions are in a final state; a machine whose top level reaches a final state has the status `done` and an output: the root's `output` (or `implementations::outputs` under the root's id) computed from the done event of the final state that completed it. A final state's output is computed from the context and the event that entered it.

#### Scenario: A parallel state is done
- **WHEN** `example/xstate/states_final_parallel.cpp` sends `BEANS_GROUND` and `WATER_BOILED` to the coffee machine
- **THEN** it prints `{"preparing":{"grindBeans":"grindingBeans","boilWater":"boilingWater"}}`, `{"preparing":{"grindBeans":"beansGround","boilWater":"boilingWater"}}` and `"makingCoffee"`

#### Scenario: A done state's microsteps
- **WHEN** `example/xstate/io_done_state_microsteps.cpp` sends `ground` to the coffee machine whose `preparation` has a final `ready` with the output `{"grams": 18}`
- **THEN** it prints `{"type":"ground","grams":18} -> {"preparation":"ready"}` and `{"type":"xstate.done.state.coffee.preparation","output":{"grams":18}} -> "brewing"`

#### Scenario: The machine is done with its output
- **WHEN** `example/xstate/concepts_final.cpp` sends `arrive home`
- **THEN** it prints `arrive home -> "walkComplete", done` and `output: {"walked":true}`

### Requirement: Semantics: History states
A history state SHALL be a pseudo-state that enters the children its parent was last in: shallow history remembers the parent's active children, deep history every active descendant, and a history node of the kind `none` (a `type` of `history` with a falsy or absent `history`) records as a shallow one does, as XState records every history but `deep` as shallow. A history that has recorded nothing enters its `target` when it has one and the parent's initial state otherwise.

#### Scenario: Shallow and deep
- **WHEN** `example/xstate/states_history_deep.cpp` powers the player off from `radio.playing` and resumes it through `powered.shallow` and `powered.deep`
- **THEN** it prints `shallow remembers ["player.powered.radio"]`, `deep remembers ["player.powered.radio.playing"]`, `resume: {"powered":{"radio":"stopped"}}` and `resumeDeep: {"powered":{"radio":"playing"}}`

#### Scenario: Back to the payment
- **WHEN** `example/xstate/states_history.cpp` goes through `checkout`, `switch`, `next` and `back`
- **THEN** it prints `"cart"`, `checkout: {"payment":"card"}`, `switch: {"payment":"paypal"}`, `next: "address"`, `remembered: ["checkout.payment.paypal"]` and `back: {"payment":"paypal"}`

### Requirement: Semantics: Initial states, parent states and parallel states
A compound state SHALL start in the child its `initial` names, running the actions of an object `initial`; a parent state's children are entered from the outermost inward; a parallel state enters all its regions, in document order; and its transitions on the parent apply whichever child is active.

#### Scenario: An initial with an action
- **WHEN** `example/xstate/states_initial.cpp` runs the light through `timer`, `timer` and `emergency`
- **THEN** it prints `"green"`, `"yellow"`, `{"red":"walk"} ["beep"]` and `{"red":"stop"} []`

#### Scenario: A parallel player
- **WHEN** `example/xstate/states_parallel.cpp` starts the player, sends `PLAY` and `MUTE`, and then `RESET`, which both regions take
- **THEN** it prints `{"track":"paused","volume":"normal"}`, `{"track":"playing","volume":"muted"}` and `1 microstep: {"track":"paused","volume":"normal"}`

### Requirement: Actors: actor_system
`class actor_system` SHALL be a system of actors on one `webcpp::xactor::scheduler`, run by its host: it ports XState's `createActor` and its actor system in one. Its scheduler holds one first-in first-out mailbox per actor and one first-in first-out queue of the actors that have work, and each turn hands one message to its actor, so a message an actor sends is queued, never handled inside the sender's turn. The host creates actors from machines (which runs nothing), then starts them, sends them events, brings the time and answers the host actors; each of these calls and `resume` runs the system until no message is left, as XState's `send()` processes at once, and returns a `result<run_outcome>`. `explicit actor_system(budgets limits)` makes a system with no actor whose every execution starts with `limits.fuel`; it MUST be neither copyable nor movable, since every actor holds a reference to its shared state, and one thread runs it. `webcpp::xactor::budgets` is brought into the namespace as `budgets`; its one member `fuel` is what each execution starts with (a million by default in xactor), and nothing refills it.

#### Scenario: Create, start, send
- **WHEN** `actors_lifecycle` creates an actor from a toggle machine (`off` and `on`), starts it and sends `TOGGLE`
- **THEN** `snapshot_of` is `nullptr` before the start; `start` returns `run_outcome::settled`; the value is `"off"` and then `"on"`; and `status_of` is `xactor::status::active`

#### Scenario: Not copyable, not movable
- **WHEN** a program copies or moves an `actor_system`
- **THEN** it does not compile

### Requirement: Actors: create_actor and actor_options
`create_actor(const machine&, const actor_options& = {})` SHALL create an actor that runs a machine, not yet started, and return a `result<actor_ref>`; `actor_options` is `{input (a boost::json::value, null for none), id (optional string), system_id (optional string)}`. The machine reads the input in `implementations::context`, and the event `xstate.init` of its initial macrostep carries it when it is not null. When no id is given, the actor's id (which `id_of` returns) is its session id `x:<n>`, where `n` counts the actors the system created or began to create before it, children and host actors included, a spawn whose claim of a systemId was refused included, as XState's counter numbers every actor (XState counts across every system of the process and xstate within one `actor_system`); an actor given an id still takes a number. A non-empty `system_id` registers the actor from its creation, before it starts, until it is done, fails or is stopped; none or an empty one registers nothing. `create_actor` refuses a systemId another actor of the system holds with `errc::system_id_taken`, which creates nothing. Unlike XState's, the actor's construction, which runs its initial macrostep, happens on `start`, so until then `snapshot_of` returns `nullptr` and `get` finds none of its children's systemIds.

#### Scenario: Session ids
- **WHEN** `actors_lifecycle` creates a root (which invokes a machine child `a` and a host child `b`), then a second root, a third with the id `mine`, and a fourth
- **THEN** their ids are `x:0`, `x:3`, `mine` and `x:5`

#### Scenario: A refused claim takes a number
- **WHEN** a root's `GO` spawns a child with a systemId that another child holds, which fails the root, and a new root is created
- **THEN** the new root's id is `x:3`

#### Scenario: The options example
- **WHEN** `example/actors/actors_options.cpp` creates one actor with the id `feedback` and the systemId `root-id` and one without options, starts the first and asks `get("root-id")`
- **THEN** it prints `feedback`, `x:1` and `true`

### Requirement: Actors: start, send and run to completion
`start(actor)` SHALL start an actor, constructing it first (XState's `actor.start()`), and `send(actor, event)` deliver an event from the host (`actor.send(event)`); each returns the outcome of the run. An actor runs one macrostep at a time: an event that arrives while a macrostep has not settled, or before the actor has started, waits, and the waiting events run afterwards in the order they arrived, each its own macrostep (XState's run to completion). An actor that has ended drops an event sent to it, as XState drops an event sent to a stopped actor. A second `start` does nothing.

#### Scenario: Events sent before the start
- **WHEN** a counter whose `ADD` increments `count` is sent `ADD` before it is started
- **THEN** `snapshot_of` is `nullptr`, and after `start` the context is `{"count": 1}`

#### Scenario: A stopped actor receives nothing more
- **WHEN** a started toggle actor is stopped and sent `TOGGLE`
- **THEN** `status_of` is `stopped`, the send returns a value and the snapshot's value stays `"off"`

### Requirement: Actors: run_outcome, and what each host call does
`enum class run_outcome { settled, out_of_fuel }` SHALL be what a host call that delivers returns: `settled` when every actor has done what it was given and no message is left, and `out_of_fuel` when an actor parked because its execution could not pay for its next piece of work. `start`, `send`, `clock_tick`, `resolve`, `reject` and `resume` each open executions of their own, each with the budget's fuel, and run the system until no message is left. `run_until_idle()` delivers nothing, opens no execution and gives no fuel: it runs what is queued and returns the same outcome, so after one of the calls above it only reports whether an actor is still parked.

#### Scenario: A call that settles
- **WHEN** a toggle actor is started and sent an event with plenty of fuel
- **THEN** each call returns `run_outcome::settled`

### Requirement: Actors: Fuel, parking and resume
Every microstep an actor runs, every message it sends (an event, a child's construction and its reply, a child's start, a stop) and every timer it arms SHALL cost one unit of the execution's fuel, paid before it runs. When the execution cannot pay, the actor parks with its cursor, the actions it has not resolved and the effects it has not run, and the call returns `out_of_fuel`; a parked actor runs nothing it has not paid for, and nothing twice, does nothing until its resume, and stays parked only while something is left to do. An event that reaches it waits behind what it has left. `resume()` gives each parked actor a new execution (with the budget's fuel) and continues it where it stopped. Each call that delivers opens executions of its own, one for each input it applies and one for each parked actor `resume()` continues; the timers a `clock_tick` releases share its one execution. The start of a machine that invokes one machine child costs five units: the root's initial microstep, the child's construction, its initial microstep, its answer and its start. A run resumed until it settles does what a run with plenty of fuel does, whatever the budget: the same macrosteps, in the same order for each actor, to the same end (not the same order across two actors).

#### Scenario: A chain on a fuel of 3
- **WHEN** `example/actors/actors_fuel.cpp` starts a machine whose `GO` takes five microsteps (`idle` to `a`, then four eventless ones, `e` taking `PING`) with `{.fuel = 3}`, sends `GO` and `PING`, and calls `resume()`
- **THEN** it prints `start: settled "idle"`, `GO: out_of_fuel "idle"`, `PING: out_of_fuel "idle"` and `resume: settled "f"`

#### Scenario: A cycle that never settles
- **WHEN** the same program starts a machine whose initial macrostep is an eventless cycle on a fuel of 100, resumes it and stops it
- **THEN** it prints `start: out_of_fuel none`, `resume: out_of_fuel none` and `stop: stopped`

#### Scenario: Every budget does what plenty does
- **WHEN** `example/actors/testing_fuel.cpp` runs a family (an app that invokes a worker, counts pongs and finishes after 1000 ms) with a fuel of 1, resuming 14 times, and then with every budget from 1 to 40
- **THEN** each actor's own sequence of macrosteps is the one a run with plenty of fuel gives, and the program prints `fuel 1: the same, after 14 resumes` and `every budget from 1 to 40: the same`

### Requirement: Actors: Held inputs while an actor is parked
While an actor is parked, the host's `start`, `send`, `clock_tick`, `resolve` and `reject` SHALL be checked (an address that names no actor is refused as ever) and then held in the order made, and return `out_of_fuel`; `resume()` applies them in that order once the parked actors have settled, each run to its end before the next, as XState runs each of its host's calls to its end. An answer whose request is no longer pending when it is applied is dropped, as XState ignores a promise that settles after its actor stopped. `stop` is never held: it is how the host ends an actor that would never settle.

#### Scenario: An event behind a parked macrostep
- **WHEN** an actor is parked in the middle of a macrostep and the host sends it another event
- **THEN** the send returns `out_of_fuel`, the event waits, and after `resume()` the first macrostep settles before the second runs

### Requirement: Actors: Construction and start
Construction and start SHALL follow XState's `createActor` and `start()`: an actor's construction runs its initial macrostep, resolving its actions; its start then hands over the custom actions and logs that macrostep returned and runs what it deferred, the starts of its active children first, as XState's `start()` starts the snapshot's active children and then runs the actor's initial actions. A child born done or failed starts where its spawn put it. A child is constructed when its parent resolves its spawnChild, and started when the parent's macrostep has settled; the root is constructed and started by the host's `start`. A construction that fails drops what it would have handed over and deferred, and the start then fails the actor. When the initial macrostep fails, the start hands over none of the custom actions and logs.

#### Scenario: Not constructed until started
- **WHEN** `example/actors/diff_construction.cpp` creates a root that invokes a worker registered under the systemId `worker`
- **THEN** it prints `before start: no snapshot, worker unknown` and `after start: "running", worker registered`

#### Scenario: A failing initial macrostep hands over nothing
- **WHEN** `actors_lifecycle` starts an actor whose initial macrostep fails after a custom action
- **THEN** the action listener hears no custom action and the actor is in error

### Requirement: Actors: How the actor resolves actions
As each microstep is computed, an actor SHALL resolve its actions in order, as XState's `resolveActionsAndContext` does, then the stopChild actions the macrostep returns after its microsteps: a custom action reaches `on_action`, and `xstate.log` `on_log`; `xstate.spawnChild` claims its systemId, if any, and creates the child the actor implementations name for its `src`, a machine actor or a host actor, with its input (a machine child is constructed at once and the parent waits until it is, so the children of its initial macrostep exist and their systemIds are registered when the next action resolves; its start is deferred); `xstate.stopChild` releases the systemIds the child and its descendants hold and the actor forgets the child (a child this macrostep created, which has not started, is stopped at once and never starts; the stop of a started child is deferred; in the initial macrostep all of it is deferred); `xstate.sendTo` resolves its target now (a child by id or `#_<id>`, the parent, the actor itself, or the actor registered under a systemId, the actor itself when none is, but for a forwardTo, which fails the actor with `unknown_target` there), one that carries `bound_at` being bound to the child its id names once that many of the microstep's actions are resolved, and its delivery or timer is deferred; a delayed raise, `xstate.cancel` and `xstate.emit` are deferred.

#### Scenario: A parent sends to its child and the child answers
- **WHEN** `actors_sending` invokes an `echo` child (answering `PING` with a sendParent of `PONG`) in a state that goes to `b` on `PONG`, and the parent's `GO` sends `PING` to `kid`
- **THEN** the root's value is `"b"`

#### Scenario: A systemId that names nobody
- **WHEN** a root sends to `#system:nobody` an event `TO_SELF` that sets `self` in the context
- **THEN** the event reaches the root itself, and its context is `{"self": true}`

### Requirement: Actors: Deferred effects
When its macrostep has settled and every action is resolved, an actor SHALL publish its snapshot (`snapshot_of`) and run what the resolution deferred, in order: deliveries and timers, cancels, emits, the stops of started children, the starts of created ones. A started child asked to stop receives the request behind everything already sent to it, so it finishes the macrostep it runs and the events that arrived before, ignores what arrives after, asks each of its own children to stop the same way, and stops, running no exit action; a child that has already ended ignores it, and its children keep running, as in XState. The actor then notifies its subscribers when its snapshot is active or done (an empty listener is none), releases its systemId if it is done or failed, even when the report waits for fuel, and reports to its parent.

#### Scenario: An exit send reaches the child before the stop
- **WHEN** a state with an invoked `kid` has an exit action that sends `kid` an event and the state is left
- **THEN** the child handles the event before it is stopped

#### Scenario: A stopped child's own children
- **WHEN** a parent leaves the state that invoked `middle`, which invokes `grand`
- **THEN** `status_of` is `stopped` for `middle` and `grand`, and `child_of(root, "kid")` is none

### Requirement: Actors: Done and error events to the parent
An actor that ends SHALL report to its parent: `xstate.done.actor.<id>` with `{output, actorId}` when it is done, and `xstate.error.actor.<id>` with `{error, actorId}` when it failed, the error being the actor's `error_value` when an unhandled error event failed it (so a grandchild's error climbs unchanged) or the name of its error code, `message()`, such as `"implementation_failed"`. `ending_event(kind, id, member, value)` makes the event (`"done"` and `"output"`, or `"error"` and `"error"`; XState's `createDoneActorEvent` and `createErrorActorEvent`), leaving `member` out when the value is none. A parent's invoke `onDone` and `onError` take them; an error event no transition takes fails the parent with `actor_failed` and its `error_value`. The failed child's snapshot keeps its code in `error` and the reported error in `error_value`, where XState's keeps the value alone.

#### Scenario: onDone takes the output
- **WHEN** `actors_children` finishes a `worker` (output `{"answer": 42}`) invoked as `kid` with an `onDone`
- **THEN** `kid` is `done`, the parent is in `loaded` and its context is `{"result": {"answer": 42}}`

#### Scenario: An error handled and one unhandled
- **WHEN** `actors_children` breaks a child under a parent with an `onError` and a child under one without
- **THEN** the first parent is in `failed` with the context `{"reason": "implementation_failed"}`; the second is in status `error`, with the error `actor_failed` and the `error_value` `"implementation_failed"`

#### Scenario: The error events of the example
- **WHEN** `example/actors/actors_errors.cpp` rejects the payment that a `checkout` (which has no `onError`) invokes with the error `{"code":"declined"}`, under a `shop` whose invoke of `checkout` has an `onError`
- **THEN** it prints `shop {"type":"xstate.error.actor.checkout","error":{"code":"declined"},"actorId":"checkout"}`, `checkout: error {"code":"declined"}` and `error code: actor_failed`

#### Scenario: A child's done event
- **WHEN** `example/actors/actors_invoke.cpp` finishes a `worker` with the output `{"result":42}`
- **THEN** it prints `"waiting" ["worker"]`, `done {"type":"xstate.done.actor.worker","output":{"result":42},"actorId":"worker"}`, `"finished" []` and `worker: done`

### Requirement: Actors: Timers and clock_tick
A delayed raise or sendTo SHALL arm a timer owned by its sender, under its id when it has one; a timer armed under an id is the one a cancel of that id finds until another is armed under it or any timer of that id fires (an earlier one under the same id still fires, as XState's scheduler keeps its timeout and its `timerMap` names the latest). `clock_tick(std::uint64_t now)` SHALL move the system's time to `now` and release the timers due by then one at a time, in (deadline, arming) order, and run after each, all in one execution, so a timer's handling can cancel a later one of the same tick, or take its id over, as XState's `SimulatedClock` fires them; a timer armed meanwhile and due by `now` is released in the same call. When an actor parks, the rest of the tick is held first in line, so no timer fires for a parked actor. A delayed sendTo is forwarded by its sender when the timer fires, in the order the timers fire. A stopped, done or failed actor's timers never fire. A time earlier than the last tick's is not refused: the system's time moves back to it and a timer armed afterwards falls due that much earlier (xactor's rule), where `simulated_clock::set` and XState's refuse it.

#### Scenario: after fires on the tick that reaches it
- **WHEN** `actors_time` ticks to 99 and then 100 an actor in `a` with `"after": {"100": "b"}`
- **THEN** the value is `"a"` and then `"b"`

#### Scenario: Leaving a state cancels its after
- **WHEN** an actor in `a` (`after` 100 to `b`, `LEAVE` to `c`) is sent `LEAVE` and ticked to 500
- **THEN** its value is `"c"` and the entry action of `b` never ran

#### Scenario: A delayed send reaches the child at its deadline
- **WHEN** at time 10 the root's `GO` sends `kid` a `PING` after 50 ms under the id `later`, and the clock ticks to 59 and then 60
- **THEN** the kid is in `idle` after 59 and in `pinged` after 60; a `FORGET` that cancels `later` first keeps the kid in `idle`

#### Scenario: The clock moved back
- **WHEN** `example/actors/diff_earlier_tick.cpp` ticks to 1000 and then to 10, creates and starts a light with `after` 100, and ticks to 1050 and 1100
- **THEN** it prints `clock_tick(10): settled`, `at 1050: "yellow"` and `at 1100: "yellow"`

#### Scenario: The delayed examples
- **WHEN** `example/actors/transitions_after_actor.cpp` ticks an actor in `attempting` (a named delay `timeout`, a guarded retry that re-enters and counts attempts, and a transition to `failed`) to 0, 1000, 2999, 3000 and 6000
- **THEN** it prints `at 0: "attempting" {"attempts":1}`, `at 1000: "attempting" {"attempts":2}`, `at 2999: "attempting" {"attempts":2}`, `at 3000: "attempting" {"attempts":3}` and `at 6000: "failed" {"attempts":3}`

### Requirement: Actors: systemIds
A `systemId` SHALL name one actor from its creation until the actor is done, failed or stopped, or until a stopChild releases it or one of its ancestors: `get(system_id)` finds it, and a claim of it fails the claiming actor. The roots of one `actor_system` share one system, where XState gives each root a system of its own, so a systemId names one actor across all of them. A done or failed actor releases it at once, even while its report waits for fuel, and a child whose construction leaves it done or failed releases its own then. When its parent starts it, the start fails the parent with `errc::system_id_taken` if a child the parent's macrostep created, or one descended from it, holds the name then; it is a deferred effect that fails. XState runs each start and each send of the macrostep's deferred effects nested, before the next, so a sibling that ends during its own start has released the name by then; here those run later, in turns of their own, so the check sees the names as the macrostep's own children hold them, and the outcome can differ.

#### Scenario: A systemId names its actor while it runs
- **WHEN** a root created with the systemId `app` invokes `kid` with the systemId `helper`, and then leaves that state
- **THEN** `get("app")` is the root, `get("helper")` equals `child_of(root, "kid")`, and after the state is left `get("helper")` is none

#### Scenario: A systemId claimed twice
- **WHEN** a root invokes `one` and `two` with the same systemId `dup`
- **THEN** the root's status is `error`, its snapshot's error is `system_id_taken`, neither child starts and its snapshot's `children` is empty

#### Scenario: create_actor refuses a held name
- **WHEN** `create_actor` is called with a systemId a running actor holds
- **THEN** it returns an error equal to `errc::system_id_taken` of the category `webcpp.xstate`, and creates nothing

### Requirement: Actors: What fails an actor
What XState throws for while resolving, a claim of a systemId that is held and a sendTo to the parent of a root, SHALL fail the actor there: no later action of the macrostep is handed over or resolved, nothing it deferred runs, the children it created are stopped, and the snapshot is the one the macrostep began from, its status `error` and its error `system_id_taken` or `unknown_target`; what was handed over before stays handed over, and the started children it asked to stop are the actor's again, as the snapshot it falls back to lists them. A microstep that fails, by an implementation or a history at the root, first has the actions it resolved before the failure resolved as any (custom actions and logs are handed over, and a spawn's claim of a held systemId fails first), then ends the same way; an initial macrostep hands over none of them. A deferred effect that fails fails the actor with the snapshot its macrostep reached, which an inspector hears as XState's update inspects it, and the children the macrostep created that have not started are stopped; a start's active children have. The failed actor's xactor status is `error`; its children keep running, as in XState.

#### Scenario: A send to the parent of a root
- **WHEN** a root's action is a `send_parent_action`
- **THEN** the root fails with `unknown_target`

#### Scenario: A send to a missing child
- **WHEN** a parent's `GO` sends an event to a child id its snapshot does not hold
- **THEN** the parent fails with `unknown_target`

#### Scenario: A failed parent leaves its host child pending
- **WHEN** a root with a host child `fetch` fails on `BREAK`
- **THEN** its status is `error` and `host_requests()` still holds the one request

### Requirement: Actors: Host actors
A host actor SHALL be created when its spawnChild resolves and, once started, is a pending `host_request{actor, parent, id, src, input}` listed by `host_requests()` in the order the actors started, until `resolve(request, output)` or `reject(request, error)` answers it (the value is an `std::optional<boost::json::value>`, none for JavaScript's `undefined`) or its actor is stopped. `resolve` finishes it done, its parent receiving `xstate.done.actor.<id>` with `output` and `actorId`; `reject` fails it, its parent receiving `xstate.error.actor.<id>` with `error` and `actorId`, its turn returning `errc::actor_failed` so that its xactor status is `error`. A host actor releases its systemId before it reports and ignores every other event. A request whose actor has ended is no longer pending, and `resolve` or `reject` of it returns xactor's `invalid_argument` and changes nothing. An answer held while an actor is parked waits for the parent and is dropped if it stopped the child. `host_logic` (the logic of a host actor) and `host_answer{resolved, value}` are the classes behind it; a host does not construct them.

#### Scenario: A fetch answered
- **WHEN** `actors_host` invokes a host actor `fetchUser` (id `fetch`, input `{"userId": 42}`) in `loading`, and the host resolves the request with `{"name": "ana"}`
- **THEN** the request lists actor, parent `root`, id `fetch`, src `fetchUser` and the input; after `resolve` the parent is in `loaded` with the context `{"user": {"name": "ana"}}`, `host_requests()` is empty and the actor is `done`

#### Scenario: A rejection
- **WHEN** the host rejects the request with `{"message": "boom"}`
- **THEN** the parent is in `failed` with the context `{"reason": {"message": "boom"}}` and the actor's status is `error`; an unhandled rejection of `"offline"` fails the parent, whose `error_value` is `"offline"`

#### Scenario: A late answer
- **WHEN** a request's invoking state is left and the host then resolves it
- **THEN** `host_requests()` was empty, the call returns an error equal to `xactor::make_error_code(xactor::errc::invalid_argument)`, and the actor's status is `stopped`

#### Scenario: The host example
- **WHEN** `example/actors/actors_host.cpp` rejects `fetchUser` with `"Network error"` and then, on a retry, resolves it with `{"name":"David","location":"Florida"}`
- **THEN** it prints `request fetchUser getUser {"userId":"42"}` and `"failure" {"userId":"42","user":null,"error":"Network error"}`, then the request again and `"success" {"userId":"42","user":{"name":"David","location":"Florida"},"error":"Network error"}`

### Requirement: Actors: stop
`stop(actor)` SHALL stop a root actor and its whole family at once (XState's `actor.stop()`), each member after its descendants, the children of a failed member included, where XState's leaves them running; no exit action runs, and a stopped actor's timers never fire. It returns a `result<std::vector<actor_ref>>` of the actors it stopped. For a child, or an address that names no actor, or from a callback, it returns `invalid_argument` and changes nothing, as XState's `actor.stop()` refuses a child; a parent's `stopChild` of a failed child keeps that child's children running, as XState does. A done actor's children were stopped by the stopChild its last macrostep returned.

#### Scenario: The family stops
- **WHEN** the host stops a root with an invoked child `kid`
- **THEN** `status_of(kid)` is `stopped`

#### Scenario: A failed child's children
- **WHEN** a child `kid` with its own child `gk` fails on `BREAK` and its parent's `onError` takes the parent to `b`
- **THEN** `kid` is in status `error` and `gk` is still `active`, and `gk` still takes a `PING` and goes to `pinged`

### Requirement: Actors: Readers
`snapshot_of(actor)` SHALL return a `const snapshot*` (valid while the system lives, what it points to changing when the actor settles again; `nullptr` before the actor's construction, for a host actor and for an address that names no actor); `status_of(actor)` a `result<webcpp::xactor::status>` (`active`, `done`, `error` or `stopped`; `invalid_argument` for an address that names no actor); `parent_of(actor)` an `std::optional<actor_ref>` (none for an actor the host created and for an address that names no actor); `id_of(actor)` an `std::string_view` (empty for an address that names no actor); `get(system_id)` the actor registered under a systemId while it holds it; and `child_of(parent, id)` the child an actor knows by an id while it has it. `actor_ref` is `webcpp::xactor::actor_ref` brought into the namespace: one member, `value`, a number that keeps nothing alive; addresses start at 1 and 0 names no actor. All of them are `const` and free to call from a callback.

#### Scenario: Readers of a missing actor
- **WHEN** the readers are given `actor_ref{.value = 7}` on an empty system
- **THEN** `snapshot_of` is `nullptr`, `parent_of` is none, `id_of` is empty, `get("x")` is none and `status_of` is an error

### Requirement: Actors: Callbacks run inside a turn
The host's callbacks (a `subscribe` or `on_emitted` listener, the `on_action` and `on_log` callbacks, and the inspector) SHALL run inside an actor's turn, while the system runs, and a call from one of them to a member that changes the system, a registration included, MUST return xactor's `errc::invalid_argument` and change nothing: `create_actor`, `start`, `send`, `clock_tick`, `resolve`, `reject`, `resume`, `run_until_idle`, `stop`, `subscribe`, `on_emitted`, `on_action`, `on_log` and `inspect`. The `const` members, which read the system (`snapshot_of`, `status_of`, `get` and the others), are free. XState accepts a send from a subscriber and handles it after the update in progress; a host that must act on what it heard records it in the callback and acts once the call that ran the system has returned.

#### Scenario: Twelve calls refused in two callbacks
- **WHEN** `actors_host` subscribes a listener that calls `start`, `send`, `clock_tick`, `resume`, `run_until_idle`, `stop`, `create_actor`, `subscribe`, `on_emitted`, `on_action`, `on_log` and `inspect`, and then `snapshot_of` and `status_of`, and the actor is started and sent `TOGGLE`
- **THEN** the listener hears `"off"` and `"on"`, the twelve changing calls are refused both times, the two readers succeed, and the actor is `active` with the value `"on"`

#### Scenario: The callbacks example
- **WHEN** `example/actors/diff_callbacks.cpp` has a listener send `NEXT` when it hears `"b"`
- **THEN** it prints `heard "a"`, `heard "b"`, `  the listener's send: invalid_argument, webcpp.xactor`, `after the host's send: "b"`, `heard "c"` and `after the host's second send: "c"`

### Requirement: Actors: subscribe, on_emitted, on_action, on_log
`subscribe(actor, listener)` SHALL add a listener of the snapshots an actor settles in (XState's `actor.subscribe`): it hears each snapshot the actor settles in while it is active, and the one it is done in, in order, for every processed event whether a transition was taken or not, and none for an error. `on_emitted(actor, listener)` adds a listener that hears every event the actor emits (XState's `actor.on('*')`), after the inspector, an event of the type `*` reaching it once, where XState's listener hears it twice; a listener that stands for one type tests the type itself, and adding it first keeps XState's order. Each call adds a listener, an actor's listeners hear in the order they were added, an empty listener is none, and an address that names no actor is `invalid_argument`. `on_action(listener)` hands every custom action of every actor to a listener with the actor that returned it (`std::function<void(actor_ref, const action&)>`), and `on_log(listener)` every `xstate.log` with its value (`nullptr` for `undefined`, valid during the call) and its label (empty when none); both hold one callback for the whole system, which a later call replaces, and an empty one removes it. A macrostep's custom actions come before its emits.

#### Scenario: A subscriber sees each snapshot once
- **WHEN** `actors_host` subscribes to a toggle with a final `end` and sends `NOTHING`, `TOGGLE` and `END` after the start
- **THEN** the listener hears `"off"`, `"off"`, `"on"` and `"end"`; and on another actor a `TOGGLE` and a failing `BREAK` notify twice

#### Scenario: An emitted wildcard event once
- **WHEN** a machine's `GO` emits an event of the type `*` to an `on_emitted` listener
- **THEN** the listener hears `*` once and the actor stays `active`

#### Scenario: Watching an actor
- **WHEN** `example/actors/actors_watching.cpp` runs an actor with a custom action `greet`, a log and an emit
- **THEN** it prints `action greet {"name":"Ada"}`, `log received {"type":"ping"}` and `emitted {"type":"pong"}`

#### Scenario: Listeners of every type
- **WHEN** `example/actors/diff_listeners.cpp` adds a listener of every type first and a listener of `PING` second, and `GO` emits `PING`
- **THEN** it prints `the listener of every type heard PING` and `the listener of PING heard PING`

### Requirement: Actors: inspect and the inspector
`inspect(inspector)` SHALL hand the system's inspector what every actor does, replacing the previous one (XState's `inspect` option); `struct inspector` holds three optional callbacks, `started(actor_ref)` (an actor's start), `settled(actor_ref, const machine&, const event& cause, const snapshot& settled)` (each macrostep a machine actor settles, with the event that began it) and `emitted(actor_ref, const event&)` (an event an actor emits, before the actor's own listeners); an empty member is never called. A macrostep that settles with `actor_failed`, which an unhandled `xstate.error.actor.<id>` event gives, is heard with that snapshot; a macrostep that fails where XState throws is not heard (an implementation's own failure, `implementation_failed` from an assign on a context that is not an object, `unknown_state`, `invalid_event`, `unknown_target` from a sendTo naming no actor or a root history's default, and one that fails while its actions resolve), and one a deferred effect fails is heard with the snapshot it reached, as XState's update inspects it.

#### Scenario: Inspecting a toggle
- **WHEN** `example/actors/actors_inspect.cpp` inspects a toggle that emits `activated` and sends it `toggle`
- **THEN** it prints `started`, `settled xstate.init "inactive"`, `emitted activated` and `settled toggle "active"`

### Requirement: Actors: Invoking and spawning children
A machine child SHALL be created from an `implementations::actors` entry that is a `machine_actor`, with the invoke's or the spawnChild's input, and run by its own `machine_logic`; its parent, `id_of` and `child_of` name it by its id, and the state that invoked it, when left, stops it with its own children (`stopChild`), while a spawned child lives until it is stopped or its parent is done. A child spawned and stopped in one macrostep never starts. A started child asked to stop finishes what it was sent before, and a failed child's own children keep running.

#### Scenario: Spawn and stop
- **WHEN** `example/actors/actors_spawn.cpp` spawns three children and stops the second
- **THEN** it prints `["child-1","child-2","child-3"]`, `["child-1","child-3"]` and `child-2: stopped`

#### Scenario: A child spawned and stopped in one macrostep
- **WHEN** a macrostep spawns `kid` and then stops it
- **THEN** the child never starts

### Requirement: Actors: The messages
`actor_message` SHALL be `std::variant<start_actor, event, resume, relay, host_answer, stop_actor, construct_actor, constructed, delayed>`, every message an actor of the layer receives, in that order, since xactor names an alternative in its log by its index and so the order is part of a recorded run. `start_actor` starts the actor, constructing it first if no parent did; `event` runs as a macrostep; `resume` continues a parked actor; `relay{to, event, send_id}` is a delayed sendTo whose timer fired, its sender forwarding the event to the actor the target named when it was resolved; `host_answer{resolved, value}` is the host's answer to a host actor; `stop_actor` asks an actor to stop with its family once it has handled what its parent sent it before; `construct_actor` constructs a machine child, which answers `constructed` once its own children are constructed too; `delayed{send_id, event}` is a delayed event to the actor itself armed under an id, whose firing unregisters the id before the actor takes the event. A host does not build them: the calls of `actor_system` deliver them, and the logic classes send them to each other.

#### Scenario: The alternatives' order
- **WHEN** `actor_message{construct_actor{}}.index()` is evaluated
- **THEN** it is 6, and `delayed` is 8

### Requirement: Actors: machine_logic, host_logic and system_state
The layer SHALL implement an actor as a class derived from `webcpp::xactor::actor_logic<actor_message>`: `machine_logic(system_state&, machine, boost::json::value input)`, the logic of a machine actor, keeps its snapshot and runs one macrostep at a time with the cursor, paying one unit a microstep, and ends done when its machine is done (after its parent has received its done event) or fails (after its parent has received its error event, its turn then returning the failure so that its xactor status is `error`); `host_logic(system_state&, optional<json::value> input)` is the logic of a host actor. A host constructs neither: `create_actor` and an invoke or spawnChild make them. `system_state` holds what the layer knows beside xactor's statuses, shared by every actor of the system and touched only during an actor's turn, so the system's single thread is the only one that ever touches it: each actor's `actor_record{id, src, system_id, current, children, parent}`, the registry of systemIds, the pending host requests, the parked actors and the host's callbacks, with the listener types `snapshot_listener`, `event_listener`, `action_listener` and `log_listener`. `fuel.hpp` holds the helpers that tell a refusal for fuel (xactor's `errc::resource_limit`, which parks the actor and changes nothing) from any other refusal.

#### Scenario: A refusal for fuel parks, any other fails
- **WHEN** a send inside a turn is refused by xactor with `resource_limit`
- **THEN** the actor parks to do it again on resume, whereas a refusal with another code fails the call

### Requirement: Actors: Interleaving across actors
The layer SHALL keep XState's order within one actor and queue every delivery between actors: xactor puts the message in its actor's mailbox and the actor in the queue of actors with work, and a turn hands one message to its actor, so a delivery never runs inside its sender's call, where XState delivers an event to an idle actor at once, nested in its sender's call stack. Each actor receives what one sender sends it in the order sent, and handles it in XState's order; but when an actor hears from two senders and one send is caused by the other's, the order between the two may differ, and so may the order of callbacks across actors. So an actor's start hands over its own initial custom actions before its initial children handle theirs, and a parent handles the last sends of a child that is done once the child has released its systemId. The tests compare each actor's own sequence, never the order across actors.

#### Scenario: The start of an app and its kid
- **WHEN** `example/actors/diff_interleaving.cpp` starts an app whose entry is `appStarted` and which invokes a kid whose entry is `kidStarted`
- **THEN** it prints `appStarted` and `kidStarted`, where the twin on XState prints `kidStarted` first

### Requirement: Differences: Where xstate departs from XState, and why
The library SHALL depart from XState on purpose and nowhere else, for four reasons, and the page's appendix "Where xstate differs from XState" (Appendix B, anchor `differences`) MUST list every departure, each with what XState does, what xstate does and why, and, where a program can show it, the C++ program, its twin and both outputs. The groups are: what JSON imposes (a machine is JSON, which holds no function, no `undefined` and no reference to an actor); what the actor model on xactor imposes (deliveries queued instead of nested in the sender's call, actors paying for their work with fuel, time the host brings, the actors of one `actor_system` living in one system, and a host's callback running inside an actor's turn); where XState leaks JavaScript or has a defect (an accident of the language or of an implementation, which xstate does not reproduce); and where imitating XState would make the API worse (failures as values, a config checked when it is loaded, a clock that hands its caller the due events, and one kind of listener of emitted events); with the checks of XState's development build, which xstate always makes. What xstate does not have yet is not a divergence and is listed in the overview instead.

#### Scenario: A divergent twin prints XState's own output
- **WHEN** the twin of `example/xstate/pure_guard_failure.cpp` (and each of the 21 other divergent twins) runs on XState 5.33.2's development build
- **THEN** it prints what `test/oracle/twins/xstate/pure_guard_failure.expected` holds, which differs from the program's `.expected`; an output equal to the program's would be a fault, since that twin agrees

### Requirement: Differences: Functions become names
Every place where XState's config holds a function SHALL take another form in xstate: a name in `implementations` (as XState's `setup()` already registers them), with JSON `params`. Three cannot keep what they do in XState: an inline action or a function `setup()` registers as an action becomes a custom action (a name `actions` does not hold, which the core returns and the actor layer hands to `on_action`, where XState runs the function); `params: ({ event }) => ...` becomes fixed `params` (the functions a built-in action or a guard is registered with read the event and the context from their `action_args`; a custom action carries its `params` alone, so what it needs from the context or the event is in them or reaches the host through an `emit_action`); and `src: someMachine` or `fromPromise(...)` becomes the name of an entry of `actors` (any other `src` refuses the machine with `invalid_config`, a name with no entry with `unknown_actor`). `context: ({ input, spawn, self }) => ...` becomes `implementations::context`, which receives the input only, a context being JSON and holding no actor.

#### Scenario: A custom action carries its params alone
- **WHEN** `example/actors/actions_custom.cpp` runs `exitAction`, `track` (with `"good"`) and `showConfetti` as custom actions
- **THEN** the host is handed them in order, and prints `returned: exitAction track showConfetti`, `tracking "good"` and `confetti!`, then the same two lines again from the actor

### Requirement: Differences: Targets that name actors are strings
A `sendTo` target SHALL be a string: a child's id or `#_<id>`, `#_parent`, `#_internal`, or `#system:<systemId>` (the actor registered under that systemId, which the actor layer resolves when the action resolves); when no actor holds the systemId a `sendTo` goes to the actor itself, as XState's does when the function `({ system }) => system.get('notifier')` returns `undefined`, and a `forwardTo` fails. Events and inputs carry no actor reference: a child answers its parent through `#_parent`, and any other actor is named by its id or its systemId.

#### Scenario: Sending between a parent and a counter by id
- **WHEN** `example/actors/actions_send_to.cpp` clicks twice and sets 10 on an app that sends to its child counter
- **THEN** it prints `click: app {"last":1}, counter {"count":1}`, `click: app {"last":2}, counter {"count":2}` and `set: app {"last":10}, counter {"count":10}`

### Requirement: Differences: The checks of XState's development build
The library SHALL make, in every build, the checks of XState's development build that a JSON machine can meet, and write no warning: an event of the type `*` that a macrostep takes from outside fails the macrostep with `invalid_event` (an event of that type the machine raises itself takes the wildcard transitions); a transition that declares a truthy `cond` refuses the machine with `invalid_config`; a `forwardTo` whose target resolves to no actor fails with `unknown_target` (an empty target, and, in an actor, `#system:<systemId>` when no actor holds the systemId). XState's production build ignores the `cond`, runs an event of the type `*`, and forwards an event whose target is `undefined` to the sender itself, which forwards it again without end; xstate behaves as the development build does, and the oracle runs that build. XState's development build also writes a console warning about an event descriptor whose `*` is not its last token; xstate writes none, and matches such a descriptor as the exact string.

#### Scenario: The three checks and the descriptors
- **WHEN** `example/actors/diff_development.cpp` makes the three checks and then sends `mouse.x` twice, `mou*se.x` and `mouse.*.*` to a state whose two descriptors are `mouse.*.*` and `mou*se.*`
- **THEN** it prints `a transition that declares cond: invalid_config`, `an event of the type * from outside: error, invalid_event`, `a forwardTo to no actor: error, unknown_target`, `the event mouse.x: "idle"` twice, `the event mou*se.x: "starred"` and `the event mouse.*.*: "deep"`

### Requirement: Differences: Failures are values
An implementation that fails SHALL return a `result` and never throw: the failure settles the macrostep with the snapshot it began from, its `status` `error` and its `error` that code, from the pure functions as from an actor, where XState's `transition()` and `getNextSnapshot()` let the error out and its `getInitialSnapshot()` and an actor catch it into the snapshot; the failing microstep returns no action. A machine child that fails sends its parent an error event whose `error` is the child's `error_value` when an error event no transition handled failed it, and otherwise the name of its error code. The failed child's snapshot keeps the code in `error` and the error it reported in `error_value`, where XState's keeps the value alone. An `error_code` carries no value of its own, where XState's implementation may throw any value, so the code's name is the JSON that says which failure it was.

#### Scenario: transition() and an actor agree on the snapshot
- **WHEN** `example/actors/diff_failures.cpp` runs a failing assign under `transition` and in an actor
- **THEN** it prints `transition() returns: error, "idle", implementation_failed, 0 actions` and `an actor's snapshot: error, "idle", implementation_failed`

### Requirement: Differences: Time in whole milliseconds
Time SHALL be counted in whole milliseconds, a delay being a `std::uint64_t`, where XState's `SimulatedClock` and delays take fractions: an `after` key is a number of milliseconds only when it is digits alone, and any other key that starts with a digit, a sign or a dot (`"1.5"`, `"-1"`, `"1e3"`, `"2fast"`) refuses the machine with `invalid_config`, even when a delay of that name is registered, the remaining keys naming delays. XState reads `"1.5"`, `"-1"` and `"1e3"` as numbers and `"2fast"` as a delay's name. xactor's timers count whole milliseconds, which keeps every deadline an exact integer.

#### Scenario: A fractional key
- **WHEN** a config has `"after": {"1.5": "b"}` and the delays registry holds `"1.5"`
- **THEN** creation returns `invalid_config`

### Requirement: Differences: Cleanup that XState leaves undone
When a macrostep fails after it spawned children, the actor SHALL fail and stop the children the macrostep created and did not start, which releases their systemIds, where XState leaves them constructed and never started with their systemIds registered; and a stopped, done or failed actor SHALL drop every timer it armed, where XState's `cancelAll` leaves the latest timer of each id firing (so of two timers armed under one id, the earlier or the later still fires and its event is delivered if its target runs). An actor that has ended sends nothing. A timer armed during a tick for a deadline the tick has already reached fires in the same call, where XState fires it only while its flush has timers left to visit.

#### Scenario: A deferred failure stops the children the macrostep created
- **WHEN** `actors_children` runs a macrostep that spawns children and then fails with a deferred effect
- **THEN** the children that have not started are stopped and their systemIds are free

#### Scenario: A stopped actor's earlier timer under an id
- **WHEN** an actor arms a timer under an id, is stopped, and the clock ticks past it
- **THEN** the timer never fires

### Requirement: Differences: A state's null meta and a description that is not a string
A state's `meta` that is null SHALL be no meta: `get_meta` does not list it and `to_json` leaves it out, where XState keeps it, lists it and writes it. A state's `description` that is not a string SHALL be left out of the machine and so of its definition, where XState keeps it whatever its type. A transition's `meta` and `description` are kept as XState keeps them (the JSON the config gives). A `null` is how JSON writes no value, so it stands for no meta, as it stands for no context.

#### Scenario: A null meta
- **WHEN** a state has `"meta": null` and `"description": 5`
- **THEN** `state_node::meta` and `state_node::description` are empty and `to_json` writes neither

### Requirement: Docs: The documentation page
The library SHALL have one documentation page, `doc/xstate.adoc`, titled "xstate: XState's state machines and actors for C++", built by `webcpp.doc xstate : xstate.adoc ;` and `webcpp.reference xstate ;` in `doc/Jamfile` (`b2 libs/xstate/doc`, into `doc/html/index.html`, published at https://webcpporg.github.io/libs/xstate/), teaching xstate as Stately's documentation teaches XState: each concept, why it exists, its smallest example, its variations and its rules. It MUST include, in this order: the introduction (`introduction/overview.adoc` "Overview", `quick-start.adoc` "Quick start" and `reading.adoc` "How to read this documentation"); the chapter State machines (`state-machines/concepts.adoc`, `machines.adoc`, `states.adoc`, `context.adoc`, `transitions.adoc`, `actions.adoc`, `input-output.adoc` and `pure.adoc`); the chapter Actors (`actors/concepts.adoc` "The actor model" and `actors.adoc`); Testing; Guarantees; the Reference; the appendices "From XState to C++" (A), "Where xstate differs from XState" (B), "Rationale" (C) and "Glossary" (D); and the "Changelog". Every program it shows was compiled and run, natively and for wasm32-wasip2 and wasm32-wasip3, and printed the output shown under it; every program is also proven against XState (its twin prints the same output, or the output of a difference xstate makes on purpose) unless it shows something XState does not have. A message of Asciidoctor's fails the build.

#### Scenario: The page is built and checked
- **WHEN** `b2 libs/xstate/doc` runs
- **THEN** it builds the page with Asciidoctor.js on Node (no Ruby and no Java), the reference with MrDocs, and fails on a message of Asciidoctor's, on a reference to no section, and on a link into xactor's page that lands on no anchor

#### Scenario: The index lists the library
- **WHEN** the superproject's `b2 doc` runs with `libs/xstate` present
- **THEN** the index lists xstate from its `meta/libraries.json` with "XState 5.33.2" as what it ports

### Requirement: Docs: The reference and the Doc Comments
Every public symbol of `webcpp::xstate` (each class, function, type, enumerator, data member and macro) SHALL have a Doc Comment, and MrDocs MUST build the reference strict, so that a missing Doc Comment fails the documentation build: a first sentence, then details, the part of XState 5.33.2 it ports ("It ports XState's `getNextSnapshot`"), `@param`, `@tparam`, `@return`, `@pre` and `@note` where they apply, and `@see` with the title of a section of the page; every `@see` title is a section title of the page. A comment that names xactor's API names it with its full name (`webcpp::xactor::scheduler`), since MrDocs cannot link into another library's reference, and the page links xactor's page where it leans on it (`link:{webcpp-libs}/xactor/{webcpp-page}#...`). `doc/mrdocs.yml` holds presentation keys only: `show-enum-constants: true` (each enumerator has a section of its own, with its value and its whole Doc Comment, since `errc`'s details say when each error is reported) and `inherit-base-members: reference` (the two actor logics derive from xactor's `actor_logic`, which is outside this reference's corpus: a class lists what it inherits as a reference to the base's member, never as a copy of it). The Doc Comments cite a guarantee by its anchor, `(doc: #xstate-invariant-13)`.

#### Scenario: A symbol without a Doc Comment
- **WHEN** a new public function has no Doc Comment
- **THEN** the strict MrDocs reference fails and so does `b2 libs/xstate/doc`

### Requirement: Docs: The guarantees
The page's section "Guarantees" (anchor `guarantees`) SHALL list what xstate promises whatever its caller does, in two groups, each guarantee under a number that keeps its anchor so that a comment of the code or of a test cites it: the machine core's 24 guarantees, anchors `xstate-invariant-1` to `xstate-invariant-24`, and the actor layer's 12, numbered A1 to A12 with the anchors `xstate-invariant-a1` to `xstate-invariant-a12`. The build MUST fail on a citation (`(doc: #xstate-invariant-13)`) that names no anchor of the page. The core's are: (1) a machine is created once from its config and never changes, and creation refuses the listed configs; (2) an action the implementations do not hold is a custom action; (3) `assign` merges and is never returned; (4) a raise without a delay goes to the internal queue; (5) `after` raises and cancels; (6) one `next()` runs one microstep; (7) `settled` is true on the closing microstep only; (8) the core never stops a macrostep and a cursor is a value; (9) eventless transitions are selected after the first microstep and after a changing one; (10) the active nodes are an insertion-ordered set; (11) an implementation's failure sets the status to `error`; (12) every result equals XState's; (13) `simulated_clock` releases by (deadline, order); (14) `to_json` writes XState's definition; (15) no exception, no global state; (16) input and `xstate.init`; (17) invoke; (18) spawn and stop actions; (19) a macrostep not active stops its children; (20) an unhandled error event settles in error; (21) the actions that name other actors; (22) every named actor is registered; (23) a history at the root; (24) values that may be undefined. The actor layer's are A1 to A12 as the requirements "Actors: ..." state them.

#### Scenario: A citation of a guarantee
- **WHEN** a test comment says `(doc: #xstate-invariant-a9)`
- **THEN** the page has the anchor, A9 being the ninth guarantee of its section "The actor layer", and the build passes

#### Scenario: A citation of nothing
- **WHEN** a comment cites `(doc: #xstate-invariant-99)`
- **THEN** `b2 libs/xstate/doc` fails

### Requirement: Docs: Appendices A, C and D
Appendix A, "From XState to C++" (anchor `from-xstate-to-cpp`), SHALL show how each of XState's JavaScript spellings is written in C++ (a JavaScript object becomes a JSON config; functions become implementations registered by name; a flat event object becomes `event{type, payload}`; actor references become `actor_ref`; promises become host actors; exceptions become `result`) and end with the table "XState's names" (anchor `reference-xstate-names`) from each function and type of XState that xstate ports to its counterpart (`createMachine`, `setup` to `create_machine` and `implementations`; `getNextSnapshot` to `get_next_snapshot`; `machine.toJSON()` to `to_json`; `SimulatedClock` to `simulated_clock`; `createActor` to `actor_system::create_actor`; and the others), marking those internal to XState. Appendix C, "Rationale" (anchor `rationale`), SHALL give each decision with its reason, the alternative weighed and its cost: a JSON config, implementations registered by name, header-only, errors returned and not thrown, no exception of its own, the macrostep cursor and fuel, determinism, the actor layer on xactor, static actors left to compile-time machines, Boost's way of building and testing, and the oracle and the twins as evidence. Appendix D, "Glossary" (anchor `glossary`), SHALL define the page's terms, the names that start with `xstate.`, the terms in a machine and in an actor system, and fuel.

#### Scenario: The names table
- **WHEN** Appendix A's table is read for `createDoneActorEvent`
- **THEN** it maps it to `ending_event`, marked internal to XState

### Requirement: Docs: Counts on the page come from the data
`doc/counts.py` SHALL print `name=<number>` lines that the page uses as attributes, never typed, beside the counts of every library: the examples of each group (`n-examples-xstate`, `n-examples-actors`), the node:test twins of the testing chapter (`n-twins-node-test`: each `testing_<name>.mjs` under `test/oracle/twins/` that imports from `node:test`), and for each of the machine and actor fixtures `n-<prefix>-cases`, `n-<prefix>-steps`, `n-<prefix>-cases-ported` (those of a file whose `source` names one of XState's test files, such as `after.test.ts`), `n-<prefix>-tests-excluded`, and the cases of xstate's own files (`review` for both, `resolution` and `vocabulary` for actors), with `n-machine-tests-to-actor-cases` and `n-machine-cases-through-pure`. A count read from a text, a file's `source` or `reason`, or a twin's imports, MUST fail when it finds nothing rather than let a rewording put a zero on the page, as does a missing own file; a fault is written on the standard error and the standard output holds only counts. It exits 0 printing the counts, 1 naming a fault, 2 on a usage error.

#### Scenario: The counts of the library at this commit
- **WHEN** `doc/counts.py` runs on the library's directory
- **THEN** it prints `n-examples-xstate=89`, `n-examples-actors=57`, `n-twins-node-test=9`, `n-machine-cases=504`, `n-machine-steps=1111`, `n-machine-cases-ported=413`, `n-machine-tests-excluded=159`, `n-machine-cases-review=43`, `n-machine-tests-to-actor-cases=36`, `n-machine-cases-through-pure=47`, `n-actor-cases=228`, `n-actor-steps=501`, `n-actor-cases-ported=163`, `n-actor-tests-excluded=153`, `n-actor-cases-review=30`, `n-actor-cases-resolution=12` and `n-actor-cases-vocabulary=16`

#### Scenario: A usage error
- **WHEN** `doc/counts.py` runs without its argument
- **THEN** it prints `usage: counts.py <library-directory>` on the standard error and exits 2

### Requirement: Docs: Changelog
`doc/changelog.adoc` SHALL be the page's last appendix "Changelog" (anchor `xstate-changelog`): what changed in each version of webcpp, newest first, one line per change, under "Unreleased" (anchor `xstate-changelog-unreleased`, lines between `tag::unreleased[]` and `end::unreleased[]`) that a user sees. It lists: that the guarantee without exceptions is now compiling only, which the lint checks; the `WEBCPP_XSTATE_NO_EXCEPTIONS` configuration macro, defined automatically or by the developer; the changelog itself; that `config.hpp` includes `<webcpp/config.hpp>`; and `WEBCPP_NO_EXCEPTIONS`, which disables xstate's exceptions with every webcpp library's.

#### Scenario: The unreleased notes
- **WHEN** `doc/changelog.adoc` is read
- **THEN** its "Unreleased" section holds five bullets, the first "Changed the guarantee without exceptions to compiling only, which the lint checks."

### Requirement: Examples: The programs of the page
The library SHALL ship 146 example programs in two groups, each a whole `main` that prints what it checks and returns 1 when a check fails: 89 in `example/xstate/` (machines run through the pure functions by the core alone) and 57 in `example/actors/` (machines run as actors on xactor), named `<prefix>_<topic>.cpp`, the prefix naming the section of the page that shows it: `qs_` (quick start), `ov_` (overview), `gl_` (glossary), `concepts_`, `machines_`, `states_`, `context_`, `transitions_`, `actions_`, `io_` (input and output), `pure_`, `actors_`, `testing_` and `diff_` (a difference from XState). `example/Jamfile` MUST name every program with `webcpp.example` from an explicit list (no glob, so a removed program is a visible change) under `webcpp.targets native wasip2 wasip3 ;`, and each is built and run on every target and its output compared with the `<name>.expected` file beside it. A program's source is included into the page by tag (`tag=main`, `tag=example`). `example/.clang-tidy` lets an example's `main` end on an exception of `boost::json::parse`, as Boost's own examples do.

#### Scenario: Every example passes natively
- **WHEN** each of the 146 programs is compiled with `clang++ -std=c++20` against Boost 1.92 and run
- **THEN** its output equals its `.expected` file byte for byte (146 of 146, checked on 2026-10-13)

#### Scenario: A removed program is visible
- **WHEN** a program is deleted from `example/xstate/` without a change to `example/Jamfile`
- **THEN** the build fails, since the Jamfile names it

### Requirement: Examples: The overview and the quick start
The quick start SHALL follow Stately's quick start step by step, with one step of its own (the pure functions), each block of code coming from a whole program of the repository and the output under it being what that program printed: create a machine (`qs_toggle`), step it with the pure functions, create an actor and send events (`qs_actor`), use delayed transitions (`qs_delayed`), handle context data (`qs_context`) and add input (`qs_input`).

#### Scenario: The quick-start actor
- **WHEN** `example/actors/qs_actor.cpp` creates an actor from the toggle, starts it and sends `toggle` twice
- **THEN** it prints `Value: "Inactive"`, `Value: "Active"`, `Value: "Inactive"` and `Now: "Inactive"`

#### Scenario: Context and input
- **WHEN** `example/actors/qs_context.cpp` sends four toggles, and `qs_input.cpp` starts the actor with a maximum count of 1
- **THEN** the first prints `State: "Inactive" {"count":0}`, `State: "Active" {"count":1}`, `State: "Inactive" {"count":1}` and `State: "Active" {"count":2}`, and the second `State: "Inactive" {"count":0,"maxCount":1}`, `State: "Active" {"count":1,"maxCount":1}`, `State: "Inactive" {"count":1,"maxCount":1}` and `State: "Inactive" {"count":1,"maxCount":1}`

#### Scenario: The delayed quick start
- **WHEN** `example/actors/qs_delayed.cpp` runs the toggle with a delayed transition
- **THEN** it prints `Value: "Inactive"`, `Value: "Active"` and `Value: "Inactive"`

### Requirement: Tests: The lightweight_test programs
The tests SHALL be written with Boost.Core's lightweight_test (`BOOST_TEST`, `BOOST_TEST_EQ`, `BOOST_TEST_ALL_EQ`, `boost::report_errors()`), one `webcpp.run` per file in `test/Jamfile`, on native, wasip2 and wasip3: `values` (error codes, `matches_state` and `to_state_path` rows, events read from JSON), `machine` (nodes, lookups, refusals, definitions, implementations), `cursor` (the macrostep cursor and the clock), and the actor layer split by responsibility into five programs, each a case in exactly one: `actors_lifecycle` (12 cases: created, started, sent events, stopped, done or failed; what a failing macrostep hands over; the ids the system names its actors with), `actors_sending` (16: the events actors send each other and the systemIds that name them), `actors_children` (10: the children an actor invokes and spawns), `actors_time` (15: timers, delayed events and cancels; fuel, a parked macrostep and what waits for it) and `actors_host` (20: host actors and their requests, and the callbacks that cannot call the system back). Each program's `main` calls its cases in order and returns `boost::report_errors()`. `test/require.hpp` holds `webcpp::test::require(bool)` (ends the program with `std::exit(boost::report_errors())` when a check a helper cannot go on without did not hold) and `webcpp::test::noted(bool, notes...)` (prints what a check was made for on the line after its failure and returns what `BOOST_TEST` returned); `test/actor_helpers.hpp` holds the helpers the five actor programs share, in `webcpp::test::xstate_actors`.

#### Scenario: Every program passes
- **WHEN** `values`, `machine`, `cursor`, `actors_lifecycle`, `actors_sending`, `actors_children`, `actors_time` and `actors_host` are compiled against Boost 1.92 and run natively
- **THEN** each exits 0 (8 of 8, checked on 2026-10-13)

#### Scenario: A failed check names its row
- **WHEN** a row of `a_state_value_matches_as_matches_state_says` fails
- **THEN** the line after the failure names it: `for matchesState(<parent>, <child>)`

### Requirement: Tests: Compile-only tests and every header alone
`test/Jamfile` SHALL declare `webcpp.headers-alone xstate : ../include ;`, which compiles each public header alone in its own translation unit on every target, so that none depends on an include made before it, and these `webcpp.compile` targets: `config`, `config_developer_defined` and `config_all_off` (the one file `config_test.cpp` compiled three ways), `core_alone` (the machine core includes none of xactor's and the actor layer's headers: an `#error` for each include guard of the 11 xactor headers and the seven actor-layer headers it must not bring) and `user_aggregates` (the aggregates of the actor layer built with designated initializers that leave members out, as user code with `-Wextra -Werror`; it also holds `webcpp::xactor::budgets`, an `envelope` and a `logged_envelope`). The checks of `core_alone` are written, one per header, by the command in the file's own comment, which must be run again when a header is added to xactor or to the actor layer (the lint's include boundary catches its include meanwhile).

#### Scenario: A new header
- **WHEN** a header is added to `include/webcpp/xstate/` that includes xactor
- **THEN** `core_alone` fails to compile and the lint fails naming the line

### Requirement: Tests: The case suites
`cases` and `actors_cases` SHALL be the two Boost.Test suites (`webcpp.boost-test`, native only, header-only Boost.Test): each reads the fixture files (the directory reaches it as the define `WEBCPP_TEST_XSTATE_FIXTURES`, with forward slashes and the whole define quoted so that a checkout whose path holds a space builds) and registers one test case per case, running each case's machine one microstep at a time through the cursor, the way a caller paying fuel runs it, and comparing every step with what XState produced: each microstep's event, transitions, actions and snapshot (value, context, status, output, tags, active nodes in order), the machine's definition and, where the definition holds the whole machine, the machine loaded back from it; the actor cases are compared actor by actor (the events each actor processed and the snapshot each left, the events it emitted, its status and the host's pending requests), never the order across actors. A number compares by value, since JavaScript holds every number as a double, and an object's keys in any order. A case file without XState's output, an output file that is no output file and a malformed case or output fail their cases instead of vanishing. The target depends on the fixtures' files, so a change to one rebuilds it.

#### Scenario: One test case per case, and the suite's own
- **WHEN** `cases` and `actors_cases` are built with Boost.Test's header-only framework and run natively
- **THEN** `cases` reports `Running 508 test cases...` (the 504 machine cases and four tests of its own) and `actors_cases` `Running 231 test cases...` (the 228 actor cases and three of its own), both `No errors detected` (checked on 2026-10-13)

#### Scenario: A case file with no output
- **WHEN** a case file has no counterpart under `fixtures/expected/`
- **THEN** the test `a_case_file_without_xstates_output_fails_instead_of_vanishing` shows that its cases fail

### Requirement: Tests: The ported cases and their files
The fixtures SHALL be XState's tests ported as data: `test/fixtures/cases/` (38 files) for the machine core and `test/fixtures/actors/cases/` (31 files) for the actor layer, one JSON file per test file of XState's (or per range of lines of a long one, `invoke_1.json` to `invoke_5.json`), which names its `source` and holds its `cases` (each with a `name`, the `test` it ports, the machine's config as JSON, and its `steps`) and, under `excluded`, every test of that file it leaves out with its `reason`; the results XState produced are beside them under `fixtures/expected/` and `fixtures/actors/expected/` (38 and 31 files). A machine case step is `start` (optional `input`), `send`, `advance`, `resolve` (a state value) or `query` (`matches`, `has_tag`, `can`, `meta`, `next_transitions`); a case may expect `expect_create_error` (XState refuses it too) or `xstate_refuses` with the error code (xstate refuses at creation what XState throws on only once it runs). An actor case lists the actors its machine may run (machines, each with implementations of its own, and host actors) and steps `create`, `start` (optional `input` and `system_id`), `send` (with `to`), `advance`, `resolve`, `reject`, `get` and `stop`. The implementations a case names come from a vocabulary written twice, entry for entry, in `test/oracle/vocabulary.mjs` for XState and `test/vocabulary.hpp` for xstate (fixed assigns and guards such as `increment`, `push` and `context_at_least`, the built-in actions a case declares with their options under `actions`, and any other name, which is a custom action). A test that cannot be ported is never dropped in silence: it is listed under `excluded` with its reason (a function JSON cannot hold, TypeScript's type checker, `enqueueActions`, callbacks or observables, persistence, `machine.provide`, `maxIterations`; spies, warnings or XState's internal objects; or a test skipped in XState's own suite). XState's SCXML suite is left out whole.

#### Scenario: The first case of after.json
- **WHEN** `test/fixtures/cases/after.json` is read
- **THEN** its first case ports `after.test.ts > delayed transitions > should transition after delay`: a light with `after` of 1000 in each state and the steps `start`, a `matches green` query, `advance` 500, `matches green`, `advance` 510 and `matches yellow`

#### Scenario: Cases of xstate's own
- **WHEN** the files `review.json`, `resolution.json` and `vocabulary.json` are read
- **THEN** they pin what XState does where its tests are silent (a root history node, an event of the type `*`, a transition that declares `cond`, a value computed as undefined, a timer armed twice under one id, a `history` that is neither `shallow` nor `deep`, the order in which JavaScript enumerates the keys of delays, regions, events and state values, the systemId of a child born done), how XState resolves `spawnChild`, `stopChild` and `sendTo` in an actor, and the vocabulary itself

### Requirement: Tests: XState as the oracle
`test/oracle/` SHALL pin the npm package `xstate` at 5.33.2 exactly, with its lockfile (installed by `npm ci` by the first target that needs it, `node_modules` being ignored by git), and hold the scripts that run xstate's test cases on it and write what XState produced: `oracle.mjs` runs the machine cases, each macrostep after the initial one through `machine.transition` with an actor scope of the script's own that records every action and every `@xstate.microstep` inspection event and whose actor has a parent (XState's pure functions cannot serve: their actor has none and throws on `sendParent`), the initial macrostep through `machine.getInitialSnapshot` with the same scope (its microsteps taken from `getInitialMicrosteps`, since XState's inspection does not report the first), and delayed events through XState's own `SimulatedClock`; `actors.mjs` runs the actor cases on XState's actors, a host actor being a promise the script settles when a step says so, writing for every actor (keyed by its path of ids from the root, and by its rank among those with the same path, `root/n#2`) the events it processed with the snapshot each left, the events it emitted, its status and the host's pending requests, never the order between two actors; `serialize.mjs` and `vocabulary.mjs` are shared; `development.mjs` refuses any build of XState but the development one. Every command runs `node --conditions=development`, XState's development build, which makes the checks XState's own tests rely on. `test/oracle/Jamfile` declares the lane with `webcpp.original node --conditions=development`, `webcpp.twins ../../example : twins : .mjs : --test-reporter=spec --test-reporter-destination=stderr`, `webcpp.cases machines : oracle.mjs : ../fixtures/cases : ../fixtures/expected`, `webcpp.cases actors : actors.mjs : ../fixtures/actors/cases : ../fixtures/actors/expected` and `webcpp.lane oracle : twins cases-machines cases-actors`. No library, suite or example uses the package, and its output is committed, so the suites run without Node.

#### Scenario: The oracle lane reproduces the committed results
- **WHEN** `node --conditions=development oracle.mjs` runs the machine cases and `actors.mjs` the actor cases on XState 5.33.2 (installed with `npm ci` from the lockfile), and each output tree is compared byte for byte with `test/fixtures/expected` and `test/fixtures/actors/expected`
- **THEN** both trees are identical (checked on 2026-10-13 with Node 26.7.0)

#### Scenario: No other build of XState
- **WHEN** a script runs without `--conditions=development`
- **THEN** `development.mjs` refuses to go on

### Requirement: Tests: The twins
Every program of `example/xstate/` and `example/actors/` SHALL have a twin in `test/oracle/twins/` under the same group and name, `<name>.mjs`, a Node program that does on XState 5.33.2 what `<name>.cpp` does on xstate and prints it the same way, unless a line `<group>/<name>: <reason>` in `test/oracle/twins/without-twin.txt` says why XState has no counterpart. Of the 146 programs, 114 twins agree (they print the program's `.expected`), 22 diverge on purpose (the twin's own output is committed beside it as `<name>.expected` and MUST differ from the program's) and 10 have no twin: `actors/actors_fuel`, `actors/gl_fuel` and `actors/testing_fuel` (XState has no fuel and no resume), `xstate/io_refused` (no registry of inputs and outputs by invoke id or state id), `xstate/machines_reload` (XState cannot create a machine from the definition `toJSON()` writes), `xstate/pure_begin_initial`, `xstate/pure_cursor`, `xstate/pure_fuel` and `xstate/testing_resume` (no macrostep cursor), and `xstate/transitions_event_json` (XState takes an event as an object and reads none from JSON). Nine twins of the testing chapter are tests of Node's own runner (`node:test`, `node:assert`), where Stately's page uses Vitest, so that each runs on Node alone. The runner (`tools/oracle/twins.py`, declared by `webcpp.twins`) runs the twins in parallel, each in a process of its own, on XState's development build, with the test runner's report sent to the standard error, colour off (`NO_COLOR` set, `FORCE_COLOR` removed) and at most 60 seconds, after which the twin and every process it started are killed; what the twin prints, every carriage return removed, MUST equal the expected output. A difference fails the target and the runner prints the diff, as does a program with neither a twin nor a line in `without-twin.txt` or with both, a twin or a line that names no program, an output of no twin, a line that is not `<group>/<name>: <reason>`, a `node` not on the `PATH`, a missing directory or a directory of examples with no program.

#### Scenario: The twins and the examples agree
- **WHEN** the twins runner lists the tree with `--list`
- **THEN** its last line is `total 146 114 22 10`

#### Scenario: Every twin passes on XState
- **WHEN** the twins run on XState 5.33.2's development build under Node 26.7.0
- **THEN** the runner exits 0, every agreeing twin printing its program's `.expected` and every divergent one its own (checked on 2026-10-13)

### Requirement: Tests: update-expected
`b2 libs/xstate/test/oracle//update-expected` SHALL be the only way the committed expected results are written, and is not a check: it writes again every expected directory from the original, `test/fixtures/expected/` and `test/fixtures/actors/expected/`, and the twins' own outputs (`twins.py --update`, which refuses an output that agrees with the program's); it writes nothing else. The oracle lane (`b2 -a libs/xstate/test/oracle//oracle`) fails when what XState produces is no longer what the suites and the examples hold. The continuous integration runs the lane from scratch beside the other lanes, in its job "Own lane (xstate, oracle)".

#### Scenario: A hand edit of an expected result
- **WHEN** a byte of `test/fixtures/expected/after.json` is changed by hand
- **THEN** the oracle lane fails, naming the file, and `update-expected` restores it from XState

### Requirement: Tests: The conversion record
`test/CONVERSION.md` SHALL record what each old test checked and where the new one checks it, so that the conversion from Boost.Test to lightweight_test can be reviewed assertion by assertion: the programs (old file, old target, new file, new target), the actor layer's 73 cases split by responsibility into five programs with their helpers, the forms converted (`BOOST_AUTO_TEST_CASE` to a function called from `main` in declaration order; `BOOST_DATA_TEST_CASE` to a function of one row called for each row; `BOOST_TEST_REQUIRE` to an if-return or `require`; `BOOST_TEST_CONTEXT` to `noted`), the assertions counted twice, statically and by grep, case by case (345 in the old `actors_test.cpp`, against 59, 65, 46, 66 and 103 in the five programs and 6 in the helpers), what is not carried and what replaces it (the 23 hand-written alone-header files by `webcpp.headers-alone`, the toolchain test by the checks of the superproject's build tests, the failing suite by a case of the build's tests), and the sections each actor object holds, counted with `-ffunction-sections -fdata-sections` against MSVC's COMDAT limit (the five actor programs and `actors_cases` take `/bigobj`; `cases` does not). The variant of each suite without exceptions and without RTTI no longer exists (owner decision, 2026-10-08): "without exceptions" is checked by the lint, which compiles every public header, and every test and example, with `exception-handling=off`.

#### Scenario: MSVC object size
- **WHEN** `test/Jamfile` declares the five actor programs and `actors_cases`
- **THEN** each carries `<toolset>msvc:<cxxflags>/bigobj`, and `cases` and the core's programs do not

### Requirement: Tests: The lint rules
The superproject's lint, which runs clang-format and clang-tidy from wasi-sdk with rules of webcpp's own, SHALL check xstate for: the licence notice every source file opens with, the layout of JSON literals, the machine core's boundary (naming the file and the line of any include of xactor or of the actor layer from the core), the patterns webcpp bans (any real clock, file, socket or process a header could name, a bare `throw`), every public header and every test and example compiled with `exception-handling=off`, and the `(doc: #...)` citations. `test/.clang-tidy` and `example/.clang-tidy` hold the notes the tests and examples need.

#### Scenario: A real clock in a header
- **WHEN** a header of xstate names `std::chrono::system_clock`
- **THEN** the lint fails, since no test can show that xstate never reads a clock
