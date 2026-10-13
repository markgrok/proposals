## Context

webcpp::xstate is a port of XState 5.33.2, Stately's TypeScript library of state machines and
actors. The code was written in an earlier private project, beside the actor system it runs on,
and moved into webcpp on 2026-10-07, after webcpp::xactor: one commit (`631e737`) that renamed it `webcpp::xstate` and gave it webcpp's
build, then 26 commits that converted its oracle, its tests, its Doc Comments, its page and its
CI to webcpp's conventions (`../tasks.md`). Its last commit, `5299db2`, is dated 2026-10-12, and
the library is in the superproject's release 0.1.0.

This document records, retroactively, the decisions 0.1.0 is built on. There is no separate
specification or plan in `reference/`: the library's documentation (`doc/*.adoc`, its guarantees
and its appendices "Where xstate differs from XState" and "Rationale"), `README.md`, `AGENTS.md`
and `test/CONVERSION.md` are its specification, and `specs/xstate/spec.md` states them as
requirements true of `reference/source/`.

## Goals / Non-Goals

**Goals:**

- Run the JSON config XState's `createMachine` reads, with the C++ implementations of its
  actions, guards, delays and actors registered by name, and produce what XState produces for it:
  the same snapshots, the same actions in the same order, the same microsteps, the same errors,
  proven by XState itself on every case ported from its tests and every example.
- Give a host control that XState does not: of the work an event may cost (the macrostep cursor
  and fuel), of time (the host brings it), of what a failure becomes (a value) and of every
  effect on the world (custom actions, logs, emitted events and host actors cross as data); the
  same inputs give the same run.
- Be a Boost-style library: header-only C++20, portable to every compiler webcpp tests, native
  and WebAssembly (wasip2, wasip3), usable in a build without exceptions, built with b2, tested
  with lightweight_test, documented in one page with an MrDocs reference.
- Layer cleanly: a program that only steps machines depends on Boost and Boost.JSON alone, and
  one that runs actors adds xactor.

**Non-Goals:**

- Everything XState 5.33.2 has that a JSON config and a single-threaded, queued actor system
  cannot or did not yet carry: `enqueueActions`, callback and observable actors, `fromTransition`,
  persistence of actors, `toPromise` and `waitFor`, `syncSnapshot`, `spawn` inside `assign`,
  `machine.provide()`, `options.maxIterations`, `xstate/graph` and the SCXML converter (the
  overview lists them).
- A compile-time state machine: a program that knows its machine when it is written writes it
  with a library such as Boost.MSM and runs it as an xactor actor; xstate interprets data.
- Threads, a real clock, files or a network: the host brings time and does the work a promise
  would do.
- A semantics of its own: where xstate departs from XState it does so on purpose, and the
  appendix lists each departure.

## Decisions

### D1. A port of XState 5.33.2, judged by XState

xstate ports the algorithm function by function, and every ported function names the TypeScript
function it ports, so a difference points at one function on each side. The evidence is XState
itself: `test/oracle/` pins the npm package `xstate` at 5.33.2 with its lockfile, runs it in its
development build, and writes what it produces for every ported case (504 machine cases and 228
actor cases) and for the twin of every example (136 of 146). The expected results are committed,
so the suites run without Node; the oracle lane runs XState again and fails when what it produces
is no longer what the suites hold. Alternative rejected: expectations written by hand, which
state what their writer believed XState does and would hold xstate to a translation error.
Moving to another XState version is a change of its own, with the oracle's new results as its
evidence.

### D2. A machine is JSON data, its code is registered by name

A machine is a `boost::json::value` in XState's config format, read when the program runs, so it
can come from a file, a request or Stately Studio's export, and XState can run the same config.
What JSON cannot hold (a function, `undefined`, a reference to an actor) is registered by name
in `implementations`, as XState's `setup()` already does, or takes another form
(higher-order guards are JSON objects `xstate.not`, `xstate.and`, `xstate.or` and
`xstate.stateIn`; an `undefined` is an empty `std::optional`; an actor is named by a string).
`create_machine` checks all it can at that moment and refuses a config XState refuses only later,
because the moment a machine is loaded is the moment its author can be told what is wrong with
it. Alternative rejected: a machine described in C++ (a builder or a table of types), which cannot
come from a file or from Stately Studio and which XState cannot run.

### D3. Two layers, one mechanical boundary

The machine core (`<webcpp/xstate.hpp>`) knows XState's actors as data and runs none; it resolves
an invoke or a spawnChild into an action for its caller to run, as XState splits `resolve` from
`execute`. The actor layer (`<webcpp/xstate/actors.hpp>`) executes those actions on xactor. The
boundary is declared in `meta/include-boundaries.json`, which the lint enforces by file and line,
and `test/core_alone.cpp` checks by the include guards, so the core stays usable by a program
that does not want a scheduler.

### D4. Errors are values, and nothing throws

Every operation that can fail returns `result<T>` (Boost.System's `result` with an `error_code`)
of the category `webcpp.xstate`, whose ten `errc` values are fixed and never reused; where XState
throws, xstate returns an error, at creation for a config and, while a machine runs, in the
snapshot's `status` and `error`, the snapshot XState's actor keeps when an implementation throws.
The headers compile without exceptions, which the lint checks for every public header and for
every test and example; programs are built with exceptions on every target, and whether a program
uses them is its own choice. `WEBCPP_XSTATE_NO_EXCEPTIONS` is defined with webcpp's configuration
and by the developer, and changes nothing in the library, which raises only through
`boost::throw_exception`. Alternatives rejected: `std::expected` (C++23, and xstate is C++20), and
exceptions (they do not reach a caller in the builds xstate is made for).

### D5. The macrostep cursor is the one primitive, fuel is the host's bound

XState's pure functions are the cursor run to its end. `macrostep::next()` runs exactly one
microstep and says whether it settled the macrostep, so a caller decides how much work an event
may cost and resumes where it stopped; the cursor is a value and a copy resumes exactly. The core
never stops a macrostep on its own (an `always` cycle never settles), and XState's
`options.maxIterations`, which the config's author sets and which throws, is not read: the bound
belongs to the program that runs the machine. In the actor layer every microstep, message and
timer costs one unit from the execution's one reservoir (xactor's fuel), an actor that cannot pay
parks and `resume()` continues it, so a machine loaded at run time cannot hang the host. One
reservoir for each execution, rather than a count for each message, bounds the total work
whatever its shape.

### D6. The actor layer runs on xactor and accepts what that costs

Each machine actor is an xactor actor whose logic drives the cursor under its turn's fuel; the
layer adds what xactor leaves out: a child's done and error events reaching its parent, invoke,
the systemId registry, emitted events and inspection. It reuses xactor's scheduler, timers, fuel
and guarantees instead of building a second actor system. The cost is documented, not hidden:
deliveries are queued instead of nested in the sender's call, so the order between two actors can
differ from XState's (never within one actor, which the tests compare), a root is constructed on
`start`, the roots of an `actor_system` share one system, time is whole milliseconds the host
brings, and a host's callback runs inside an actor's turn and may not call the system back.

### D7. Host actors replace promises; effects cross as data

A `host_actor` stands for `fromPromise`: once started it is a pending `host_request`, and the host
answers it with `resolve` or `reject` when it chooses, so a test is the host and has nothing to
wait for. A custom action (a name the implementations do not hold) is returned as
`{type, params}` and handed to `on_action`, a log to `on_log`, an emitted event to the listeners:
the machine says what is to be done, and the program decides how. An implementation receives no
actor and no system, where XState's does.

### D8. Fidelity down to JavaScript's accidents that XState's behaviour depends on

Where XState's observable behaviour is a property of JavaScript that a port must reproduce to
match the oracle, xstate states it as a rule: object keys are enumerated array indices first, by
number, then in insertion order (for `states`, `on`, `after`, state values, `children`); the
active nodes are an insertion-ordered set, and a machine that is done lists them by descending
`order`; snapshots are compared by identity, stated as "changed" for the eventless loop; the
latest timer armed under an id is the one a cancel finds; equal deadlines fire in arming order.
Where XState's behaviour is an accident that does harm (a timer firing for a stopped actor, a
listener hearing an event twice, children of a failed macrostep never started and never
stopped), xstate does what the
accident gets in the way of and says so in the appendix.

### D9. Every departure is deliberate, recorded and tested

The appendix "Where xstate differs from XState" gives each departure with what XState does, what
xstate does and why, grouped by reason (what JSON imposes, what the actor model on xactor
imposes, where XState leaks JavaScript or has a defect, where imitating XState would make the API
worse) and, where a program can show it, the C++ program, its twin and both outputs: 22 divergent
twins print XState's different output, committed beside them, and the runner fails when a twin's
output equals the program's. XState's development build is the judge for the three checks a JSON
machine can meet (an event of the type `*` from outside, a truthy `cond`, a `forwardTo` to no
actor), which xstate makes in every build.

### D10. Determinism, no state, no environment

No global or static mutable state, no thread, no clock, file or network in a header (the lint
refuses what could name one); the same machine and events give the same run on every target.
Time is a number the host brings (`clock_tick`) or a `simulated_clock` its caller moves. An
`actor_system` is neither copyable nor movable and is driven by one thread. The core's recursions
are explicit lists and stacks, so a machine as deep as its JSON never exhausts the stack.

### D11. Header-only, with Boost.JSON as the value type

Nothing of xstate is compiled; a program supplies Boost.JSON's definitions once. Every value is a
`boost::json::value`, whose objects keep member order, which the key-order rules need. Alternative
considered later (not 0.1.0): a document type of webcpp's own, see Open Questions.

### D12. Tested the way webcpp tests: three targets, three kinds of test

lightweight_test programs for what the oracle cannot see (values, the machine's creation and
refusals, the cursor, and the actor layer in five programs split by responsibility), two
Boost.Test suites for the ported cases (native only, since they read fixture files), compile-only
targets (the three configurations, the core without xactor, user-code aggregates) and every
header alone; every example on native, wasip2 and wasip3 with its output compared; the oracle lane
beside them. Each lane of the continuous integration builds from scratch and records what b2
built, merged into the published test matrix. MSVC compiles the actor programs and `actors_cases`
with `/bigobj`, a figure counted with another toolchain's sections and not measured on MSVC at
the time.

### D13. One page in Stately's shape, with numbered guarantees

The documentation teaches xstate as Stately's documentation teaches XState (each concept, why, its
smallest example, variations, rules), every program compiled and run on every target and every
output committed; a section of guarantees whose numbers (1 to 24, and A1 to A12 for the actor
layer) are anchors that code and tests cite, the build failing on a citation naming no anchor;
every count on the page computed (`doc/counts.py`), never typed; a reference written by MrDocs
from Doc Comments that name the XState function each symbol ports.

### D14. Licence

The library's code is under the Boost Software License. What derives from XState (the machine
semantics ported from its algorithms, the test cases transcribed from its tests, the twins and the
shared examples) keeps XState's MIT notice and copyright in `LICENSE-XSTATE.txt`, unchanged, and
the README says what derives; that adds no licence to xstate's own code.

## Risks / Trade-offs

- [XState's behaviour is a property of JavaScript] -> the oracle runs XState itself, every case
  and twin is committed with its output, and the lane fails when XState's output changes under a
  pinned version; a new XState version is a change of its own.
- [The actor layer's guarantees lean on xactor's (fuel, timers, clock rules)] -> a change in
  xactor can change what an actor does; the actor cases and the five actor programs catch it, and
  a committed output already names xactor's category (`webcpp.xactor`).
- [Boost.JSON's limits (a hashed object index above a small size, numbers as doubles, a value that
  holds pointers and so cannot be sent whole as binary)] -> accepted for 0.1.0; a document type of
  webcpp's own is the owner's direction for a later change (Open Questions).
- [MSVC is not available locally] -> its object-size limit was handled from counts made with GCC
  and Clang, and the continuous integration has MSVC 14.3 and 14.5 lanes; their results are not
  re-measured in this proposal (`reference/STATUS.md`).
- [A config that creation refuses but XState accepts] -> deliberate (D2, D9); the list is the
  table "What creation refuses" with XState's behaviour in a column.
- [Fuel changes the shape of a call: a host call can return `out_of_fuel` with an actor parked]
  -> `resume()`, the held inputs and the test that every budget gives what plenty gives.

## Migration Plan

None for users: a new library of webcpp. The code came from an earlier private project; its
namespace, macros, error category and licence notices were renamed at the move, so that, for
instance, `example/actors/diff_callbacks.expected` names the category of an error of xactor's
as `webcpp.xactor`.

## Open Questions

- Which of the unported parts of XState (the overview's list) comes first: `enqueueActions`,
  callback and observable actors, persistence of actors, `machine.provide()`?
- The owner has asked that the state of machines and actors be able to live in memory shared
  between processes or relocatable, with allocators the user customises, and has proposed a
  document type of webcpp's own (behaving as JavaScript does, with JavaScript's key order) in
  place of Boost.JSON for that; how xstate moves to it, and whether its actors then require
  exceptions (Boost.Interprocess throws), is a change of its own. 0.1.0 compiles without
  exceptions.
- Will the site run its state machines on xstate compiled to WebAssembly, as planned, and what
  does it need from the API that 0.1.0 lacks?
- Should an invoke's `onSnapshot` be completed with `syncSnapshot`, or the config's acceptance of
  it be refused at creation until it is?
