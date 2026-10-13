## Context

webcpp::xactor began on 2026-10-07 as the first library of the webcpp superproject. Its code was the
actor system of an earlier private project, whose state-machine layer is what became webcpp::xstate.
That system was moved into a repository of its own, renamed from its old namespace to
`webcpp::xactor`, its tests converted from Boost.Test to Boost.Core's lightweight_test with an
equivalence record, every public symbol given a Doc Comment for a strict MrDocs reference, and its
documentation page rewritten so that the library stands alone. The first commit is `284b68a`
(2026-10-07); release 0.1.0 is the repository at `62f3e77`.

This document records, retroactively, the decisions 0.1.0 is built on. The library has no
specification or plan apart from its own documents: `doc/*.adoc` (the page, with 36 numbered
guarantees), the Doc Comments of its headers, `AGENTS.md`, `README.md` and `test/CONVERSION.md`.

## Goals / Non-Goals

**Goals:**

- A run is a function of its deliveries: one thread, one FIFO mailbox for each actor, one FIFO queue
  of the actors with work, so two runs, or two drivers, deliver the same messages in the same order
  and write the same envelope log.
- Every execution ends: a budget of fuel pays for every message and timer, so no chain, fan-out or
  ping-pong of messages runs forever.
- Time is the host's: a run with timers is as repeatable as one without.
- Failures are values (`result<T>`), the library raises nothing of its own, and it works natively,
  on wasm32-wasip2 and on wasm32-wasip3, where Boost.Asio is not available.
- Stand alone: generic over the program's message type, depending on Boost's headers and webcpp's
  configuration only, naming no other webcpp library in its code.

**Non-Goals:**

- Threads, parallelism or a clock of its own: the host brings messages and time.
- A child's end reaching its parent, a registry of actors by name, the inspection of what each actor
  does, supervision: left to the program or to a library built on xactor (`doc/introduction.adoc`).
- A state-machine adapter: any class with a `handle` is an actor, and a program joins a machine to it
  in a few lines (`doc/static-actors.adoc`).
- Persisting actors, or placing messages and state in a shared or relocatable segment: not part of
  0.1.0 (the values of `status` are fixed so that such a system can rebuild actors from numbers).
- A native Boost.Asio driver on WASI: Boost.Asio 1.92 does not compile for either WASI target.

## Decisions

### D1. A library of webcpp's own, standing alone

xactor ports nothing; its evidence is its own tests. `build.jam` declares `/webcpp/xactor//xactor`,
an alias adding `include`, `/boost//headers` and `/webcpp/config//config`, and nothing is compiled.
No header, comment or example names another webcpp library; the libraries built on xactor depend on
it, never the reverse (the page names xstate once, as a library that builds on it).

### D2. Determinism by construction

One FIFO mailbox for each actor and one FIFO queue of the actors with messages waiting, each at
most once. `run_one` takes the front actor, hands it the first message of its mailbox, and puts it
back at the end while its mailbox holds more, so no actor starves another however much it sends
itself, and a message sent in a turn is never handled inside it. The scheduler starts no thread,
reads no clock, disk or network, and keeps no static mutable state, so two schedulers in one process
are independent.

### D3. Fuel for each execution

All the messages one host delivery causes are an execution, named by the correlation the host gives
it; its fuel (default 1,000,000) is kept under that correlation and never refilled. `send`, `reply`
and `wake_at` cost one unit, `spend(n)` n units, everything else nothing, and a host delivery
nothing. Payment is all or nothing: a refused call enqueues, arms and spends nothing. A message to an
actor that has ended is paid and dropped: dropping is not an error of the sender's, which cannot
know the actor has ended. Fuel is kept for each execution, not globally, so one input's work is
bounded whatever another input does.

### D4. Errors are values, from a category of its own

`result<T>` is `boost::system::result<T, error_code>`. xactor has two codes in the category
`webcpp.xactor`: `invalid_argument` = 1 (a call that names what it cannot act on) and
`resource_limit` = 2 (fuel). A handler may return any `error_code`, which the scheduler keeps as it
is. A failing handler ends its actor with the status `error`, not the run: `run_one` still returns
`true`. `run_one` returns a `result<bool>` all the same, so a failure of the scheduler itself has a
channel the drivers already forward; there is none today. The category is a constant of fixed
identity (the ASCII of "xactor" and two zero bytes), so it holds no mutable state.

### D5. The actor is a class, its message a closed variant

`actor_logic<Message>` has one pure virtual `handle(turn&, const envelope&)`; it is neither copyable
nor movable, because the scheduler owns it where it was constructed and never moves it (a test may
keep a pointer to it). `Message` is a `std::variant` that can be moved, nothing more: not copyable,
not default-constructible; the log records only `payload.index()`, so a log entry has a fixed width
and two logs compare cheaply. A logic never sees the scheduler, only its `turn`, which lives for one
`handle`.
A closed variant lets `std::visit` check that every alternative is handled.

### D6. Drivers decide when, the scheduler decides what

The scheduler's `run_one` is the one step; drivers only call it. `fifo_driver` (`run_until_idle`,
`run_turn(n)`) is the driver of a program; `test_driver` has `step()` and `step(log)` and nothing
else, so a test sees what exactly one delivery changed; `asio_driver` posts each `run_one` onto a
strand of an `io_context` it owns and drains it with `poll`, never `run`, `run_one` or `run_for`
(which would wait for work under the work guard). The context offers no executor, so a program's own
I/O objects run on a context of their own. `asio_driver` and its Boost.Asio includes are declared
only when `__wasi__` is not defined.

### D7. Time and timers

`clock_tick(now, correlation)` moves time and releases every timer due, in (deadline, arming order);
`release_next` releases the first due timer only, so a host that runs the system between two
releases lets one timer's handling cancel or replace a later one of the same tick. A keyed timer
replaces the same actor's timer under the key and takes a new arming order. Time can move back; the
call opens the correlation as `deliver` does, and releasing costs nothing since arming was paid.
Timers live in an ordered map keyed by (deadline, arming sequence), so no two entries collide.

### D8. Lifecycle, families and fixed numbers

An actor is `active`, `done` (finished with an output), `error` or `stopped`, with the fixed values
1 to 4; an ended actor stays as it ended and its mailbox, queue entry and timers are dropped. Actors
spawned in a turn are children; a family is stopped descendants first, siblings in spawn order,
without recursion (an explicit stack follows a family of any depth), by the host's `stop` or a
parent's `stop_child`. `finish` and `stop` end the actor alone: its children are its logic's to stop.
xactor does not tell a parent that a child ended. The values of `errc` and `status` are fixed and
never reused.

### D9. Exceptions are a build choice of the program

The library raises no exception of its own; every header compiles without exceptions, which the
superproject's lint checks by compiling every public header, test and example with
`exception-handling=off`. `config.hpp` (included first by every public header, after including
`<webcpp/config.hpp>`) defines `WEBCPP_XACTOR_NO_EXCEPTIONS` when webcpp has no exceptions or when a
program defines it for xactor alone; the macro changes nothing in xactor and exists so one setting
serves every webcpp library. An exception xactor raised would go through `boost::throw_exception`.
Operations that allocate (`spawn`, `deliver`, `send`, `wake_at`, the log's `record`) can still throw
`std::bad_alloc` where exceptions are on. The guarantee of running without RTTI was withdrawn
(changelog, Unreleased). The policy changed three times in the first days: exceptions off on wasip2
only (`88cbbdb`), every target with exceptions and the headers checked without (`992eaed`), the
macro and its tests (`1d41ce8`), the include of webcpp's configuration (`1b1b159`).

### D10. Tests on lightweight_test, with an equivalence record

The four suites were converted from Boost.Test to Boost.Core's lightweight_test (a header; it also
builds on WASI), one program for each subject, each case a function of the file's anonymous
namespace called from `main`, which returns `boost::report_errors()`. `test/CONVERSION.md` records
case by case that no assertion was lost (55 cases, 295 assertions in the source), and three planted
defects (a move-only guarantee, fuel accounting, a lifecycle status) each fail the same cases in the
old and the new suites. A check a case cannot go on without is
`if (!BOOST_TEST(...)) { return; }`; a helper calls `webcpp::test::require`, which ends the program
with the errors counted so far.

### D11. One page, a strict reference, numbered guarantees

`doc/xactor.adoc` includes one section for each concept (introduction, quick start, the model, logic,
scheduler, turns, fuel, timers, lifecycle, drivers, static actors, determinism, testing,
guarantees), the MrDocs reference, a glossary and a changelog. Every listing includes a compiled
example by `tag::` markers and every output an `.expected` file. The guarantees keep their numbers
and the anchors `xactor-invariant-<n>`; tests and code comments cite them as `(doc: #...)`, and the
page's build fails on a citation that names no anchor. Every public symbol has a Doc Comment, and the
reference is built strict. The page states no count of the tree's, since a typed count goes stale.

## Risks / Trade-offs

- [A handler throws] -> xactor catches nothing; an exception leaves `run_one` and the driver in the
  middle of the run. Documented (`doc/determinism.adoc`); a handler returns an error instead.
- [An actor reads the world] -> determinism is the actor's to keep; `doc/determinism.adoc` lists six
  rules and a program checks them by comparing the logs of two runs.
- [A host reuses a correlation] -> two inputs then share one reservoir; the host gives each input its
  own correlation (documented).
- [`output_of` returns a pointer] -> valid only until the scheduler takes in another actor.
- [A default member initializer on `timer::payload`] -> clang with libstdc++ would instantiate it
  through `std::erase_if` and require a default-constructible message; the member has no `{}` and a
  test with a move-only message checks it (guarantee 28).
- [The Asio driver is untestable on WASI] -> every other program includes `<webcpp/xactor.hpp>` and
  so `drivers.hpp`, whose WASI build checks that nothing of Boost.Asio enters it.

## Migration Plan

None: a new library. The earlier private project's actor layer moved here and is replaced by it.

## Open Questions

None for 0.1.0. See `reference/STATUS.md` for open items found while reading the tree.
