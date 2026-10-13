# xactor Specification

## Purpose
webcpp::xactor is a header-only, deterministic actor system for C++20. An actor is a logic that
handles one message at a time in a turn that lets it send and reply, spawn and stop children, arm
timers and finish with an output; the scheduler gives each actor its messages in an order that
depends on the order they were sent alone, every execution is paid from a budget of fuel, and time
moves only when the host brings it. It runs natively, on wasm32-wasip2 and on wasm32-wasip3, needs
only Boost's headers and webcpp's configuration, and is generic over the program's message type.

## Requirements

### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20, with every name in the namespace `webcpp::xactor` (the
internals of `errors.hpp` in `webcpp::xactor::detail`), under `include/webcpp/`, and MUST need
nothing to build or link: no source file, no compiled Boost library. The code of the documentation
writes the names as `xactor::scheduler`, after `namespace xactor = webcpp::xactor;`.

#### Scenario: A program needs nothing compiled
- **WHEN** `example/xactor_quick_start.cpp` is compiled with `-std=c++20`, the include directories of xactor, of webcpp's configuration and of Boost 1.92, and no library
- **THEN** it compiles and links, and printing its output gives `delivered 7 messages` and `total 6, fuel left 94`

### Requirement: Library: The convenience header and the public headers
The library SHALL ship `include/webcpp/xactor.hpp`, which includes `<webcpp/xactor/config.hpp>` first
and then `actor_logic.hpp`, `budgets.hpp`, `drivers.hpp`, `envelope.hpp`, `envelope_log.hpp`,
`errors.hpp`, `ids.hpp`, `scheduler.hpp` and `status.hpp`, all of `include/webcpp/xactor/`, so that
including `<webcpp/xactor.hpp>` alone makes the whole library available. Each header of
`<webcpp/xactor/>` MAY also be included alone. There MUST be no header under a `detail/` directory:
the one internal namespace, `webcpp::xactor::detail`, lives in `errors.hpp`.

#### Scenario: Every header compiles alone
- **WHEN** a translation unit contains only `#include <webcpp/xactor/scheduler.hpp>` (or any one of the other nine headers, or `<webcpp/xactor.hpp>`), compiled with `-fsyntax-only -fno-exceptions -DBOOST_NO_EXCEPTIONS`
- **THEN** it compiles without error

### Requirement: Library: config.hpp comes first in every header
Every public header, `xactor.hpp` included, SHALL include `<webcpp/xactor/config.hpp>` before any
other include of the library, and `config.hpp` SHALL include `<webcpp/config.hpp>`, webcpp's
configuration, before it defines anything. A header MUST NOT depend on an include it does not make.

#### Scenario: config.hpp brings webcpp's configuration
- **WHEN** `test/config_test.cpp` includes only `<webcpp/xactor/config.hpp>`
- **THEN** `WEBCPP_CONFIG_HPP` is defined, or the file stops with `#error "config.hpp does not include <webcpp/config.hpp>"`

### Requirement: Library: Build target and dependencies
`build.jam` SHALL declare `project /webcpp/xactor` and one alias `xactor` with no sources, whose
usage requirements are `<include>include`, `<library>/boost//headers` and
`<library>/webcpp/config//config`. A b2 project uses xactor through `/webcpp/xactor//xactor`. The
library MUST depend on Boost's headers (1.92 or newer: Boost.System for `result` and the error
codes, Boost.Asio for the Asio driver outside WASI) and on webcpp's configuration, and on nothing
else: no threads, no clock, no global or static mutable state, and no other webcpp library.

#### Scenario: The alias adds headers and dependencies
- **WHEN** `build.jam` is read
- **THEN** it holds `alias xactor : : : : <include>include <library>/boost//headers <library>/webcpp/config//config ;` and names no other library

### Requirement: Library: Standing alone
No header, comment or example of the library SHALL name another webcpp library: the libraries built
on xactor depend on it, never the reverse. The documentation page MAY name webcpp::xstate once, as a
library that builds its actor layer on xactor, and link to its page.

#### Scenario: The introduction names xstate
- **WHEN** `doc/introduction.adoc` is read
- **THEN** it says that webcpp's xstate builds its actor layer on xactor, and no header under `include/` contains the word xstate

### Requirement: Library: Errors are values and the library throws nothing of its own
Every operation of the library that can fail SHALL return a `result<T>`, and the library MUST NOT
throw an exception of its own: a throw in a header goes through `boost::throw_exception` alone, and
every public header, test and example compiles without exceptions. Where exceptions are on, an
operation that allocates (`spawn`, `deliver`, `send`, `wake_at`, the log's `record`) MAY still throw
`std::bad_alloc`, as a standard container does. An exception thrown by a `handle` is not caught: it
leaves `run_one` and the driver.

#### Scenario: The quick start builds without exceptions
- **WHEN** `example/xactor_quick_start.cpp` is compiled with `-fsyntax-only -fno-exceptions -DBOOST_NO_EXCEPTIONS`
- **THEN** it compiles without error

### Requirement: Library: Configuration macro for exceptions
`config.hpp` SHALL define `WEBCPP_XACTOR_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined
(the compiler has no exceptions, or the program defined `WEBCPP_NO_EXCEPTIONS` for every webcpp
library) and MUST keep the definition when a program defines the macro for xactor alone in a build
that has exceptions. The macro changes nothing in xactor, which raises none of its own; it exists so
that one setting serves every webcpp library. Under `__MRDOCS__`, the header MUST define the macro
and undo it at once, so that the reference lists it.

#### Scenario: Defined exactly when webcpp has no exceptions
- **WHEN** `test/config_test.cpp` is compiled as the build asks, with neither `WEBCPP_TEST_XACTOR_*` macro
- **THEN** `defined(WEBCPP_HAS_EXCEPTIONS)` differs from `defined(WEBCPP_XACTOR_NO_EXCEPTIONS)` and the file compiles

#### Scenario: Defined by the developer for xactor alone
- **WHEN** `config_test.cpp` is compiled with `-DWEBCPP_XACTOR_NO_EXCEPTIONS -DWEBCPP_TEST_XACTOR_DEVELOPER_DEFINED` in a build with exceptions
- **THEN** `WEBCPP_XACTOR_NO_EXCEPTIONS` is still defined and the file compiles

#### Scenario: Defined by WEBCPP_NO_EXCEPTIONS
- **WHEN** `config_test.cpp` is compiled with `-DWEBCPP_NO_EXCEPTIONS -DWEBCPP_TEST_XACTOR_ALL_OFF`
- **THEN** `WEBCPP_XACTOR_NO_EXCEPTIONS` is defined and the file compiles

### Requirement: Library: No promise about RTTI
The library MUST NOT promise that it runs without RTTI: the guarantee was removed before 0.1.0
(changelog, "Unreleased": "removed the guarantee that xactor runs without RTTI; build it with RTTI"),
and a program SHALL build xactor with RTTI.

#### Scenario: The changelog says so
- **WHEN** `doc/changelog.adoc` is read
- **THEN** its first entry under "Unreleased" is marked "Breaking change" and says that the guarantee that xactor runs without RTTI was removed

### Requirement: Library: No hidden state, clock, disk, network or thread
The library SHALL read no clock, no disk and no network, start no thread, block in no turn, call none
of `io_context::run`, `run_one` or `run_for`, and keep no static mutable state, so that two schedulers
in one process are independent. The category of its errors is a constant (`constexpr`) of fixed
identity.

#### Scenario: Two schedulers are independent
- **WHEN** the same deliveries are fed to two schedulers built with the same budgets in one process and each is run to idle with its own `envelope_log`
- **THEN** the two logs are equal (`from_fifo == from_asio` in `both_drivers_produce_the_same_envelope_log` is the same comparison across drivers)

### Requirement: Library: Targets
`test/Jamfile` and `example/Jamfile` SHALL declare the targets `webcpp.targets native wasip2
wasip3`. The programs built natively use any toolset with exceptions unless the user asks for
`exception-handling=off`; the wasip2 and wasip3 programs are built with the toolsets `clang-wasip2`
and `clang-wasip3` (the Clang of wasi-sdk 34 for wasm32-wasip2 and wasm32-wasip3), with WebAssembly's
exception handling, and run by wasmtime (`testing.launcher=wasmtime`). A WASI build MUST leave out
`asio_driver` and Boost.Asio's headers.

#### Scenario: The commands of the page
- **WHEN** a developer in the superproject runs `b2 libs/xactor/test libs/xactor/example`, then the same with `toolset=clang-wasip2 testing.launcher=wasmtime`, then with `toolset=clang-wasip3 testing.launcher=wasmtime`
- **THEN** each builds the tests and the examples for its target and runs them (the page documents these three commands)

### Requirement: Library: Repository files of a webcpp library
The repository SHALL hold `README.md`, `AGENTS.md`, `LICENSE_1_0.txt` (Boost Software License 1.0),
`build.jam`, `meta/libraries.json`, `.gitignore` (`doc/html/`), `.gitattributes` (`* text=auto
eol=lf`, `*.bat text eol=crlf`) and `.github/workflows/ci.yml`; every header, source, test, example, Jam, AsciiDoc and workflow file begins with `Copyright (c)
2026 WebCpp.org` and the Boost licence notice (`README.md`, `AGENTS.md`, `test/CONVERSION.md` and
`meta/libraries.json` carry none).
`meta/libraries.json` MUST hold the key and name `xactor`, the authors `["R"]`, the description "A
header-only, deterministic actor system: actors that handle one message at a time, with fuel per
execution, timers and a lifecycle.", the category `["Concurrent"]`, the maintainer `R
<github.overboard312@passmail.net>`, `"cxxstd": "20"` and `"port-of": null`.

#### Scenario: The metadata
- **WHEN** `meta/libraries.json` is read
- **THEN** `key` is `xactor`, `cxxstd` is `20`, `port-of` is `null` and `category` is `["Concurrent"]`

### Requirement: Library: AGENTS.md states the library's rules
`AGENTS.md` SHALL point to the superproject's `AGENTS.md` and state only what is specific to xactor:
that it stands alone; that determinism is its point (a change that makes a run depend on anything
but the enqueue order, `clock_tick` and `release_next` is a bug); that errors are values; that
`config.hpp` comes first; that the values of `errc` and `status` are fixed and never reused; that
Boost.Asio is native only and a driver drains its `io_context` with `poll`; the targets; the rules
of the tests; the rules of the page and of the reference.

#### Scenario: The Asio rule
- **WHEN** `AGENTS.md` is read
- **THEN** it says that `drivers.hpp` includes Boost.Asio for `asio_driver` alone, outside WASI, each include marked `lint-world:`, and that the lint bans `run`, `run_one` and `run_for`

### Requirement: Errors: The errc enumerators
The library SHALL declare `enum class errc : int` in `errors.hpp` with exactly two enumerators:
`invalid_argument = 1`, a call that names what it cannot act on, which changes nothing (a null logic
given to `scheduler::spawn`; an address that names no actor, given to `deliver`, `stop`, `status_of`
or `turn::send`; a `turn::reply` to a message from the host; a `turn::finish` or `turn::stop` once
the actor has ended; a `turn::stop_child` of an actor that is not a child; a timer or a child asked
by an actor that has ended), and `resource_limit = 2`, the execution has no fuel left for what was
asked, which changes nothing. The values start at 1, so an `error_code` of value 0 is no error.

#### Scenario: The values
- **WHEN** `static_cast<int>(xactor::errc::invalid_argument)` and `static_cast<int>(xactor::errc::resource_limit)` are read
- **THEN** they are 1 and 2

### Requirement: Errors: The category webcpp.xactor
`error_category()` SHALL return a constant `boost::system::error_category` with the fixed identity
`0x786163746f720000` (the ASCII of "xactor" followed by two zero bytes), whose `name()` is
`"webcpp.xactor"` and whose `message(value)` is the name of the enumerator, `"unknown"` for a value
`errc` does not define (0 included). The overload `message(value, buffer, size)` MUST copy the name
into `buffer` cut to fit `size` bytes with its terminating zero, write nothing when `buffer` is null
or `size` is 0, and return the whole name.

#### Scenario: Messages
- **WHEN** `error_category()` is asked `name()`, `message(1)`, `message(2)`, `message(99)` and `message(0)`
- **THEN** they are `webcpp.xactor`, `invalid_argument`, `resource_limit`, `unknown` and `unknown`

#### Scenario: A cut message
- **WHEN** `message(2, buffer, 6)` is called with a `char buffer[6]`
- **THEN** `buffer` holds `resou` and the call returns `resource_limit`

### Requirement: Errors: Conversion to error_code
`make_error_code(errc)` SHALL return the `error_code` of the value and the category
`error_category()`, and `boost::system::is_error_code_enum<errc>` MUST be `std::true_type`, so that an
`errc` converts to an `error_code` and compares with one.

#### Scenario: Comparing an error with an enumerator
- **WHEN** a failed `send` returns an error `sent.error()`
- **THEN** `sent.error() == xactor::errc::resource_limit` compiles and is true for a refused send, and `make_error_code(errc::invalid_argument).category() == error_category()`

### Requirement: Errors: result and failure
`result<T>` SHALL be an alias of `boost::system::result<T, boost::system::error_code>`, and
`failure<T>(errc)` a `noexcept` function returning a failed `result<T>` that holds
`make_error_code(code)`. An operation of a `scheduler` or of a `turn` that fails MUST change nothing;
a driver returns the failure that stopped it after the deliveries it has already made. `*` of a
failed `result` is undefined and its `value()` reports the failure through `boost::throw_exception`.

#### Scenario: A failed result
- **WHEN** `failure<int>(errc::resource_limit)` is built
- **THEN** `has_value()` is false and `error().message()` is `resource_limit`

### Requirement: Errors: A handler may return any error_code
A `handle` MAY return any `boost::system::error_code`, of xactor's category or of its own, and the
scheduler SHALL keep it as it is, for `scheduler::error_of`.

#### Scenario: A failing actor
- **WHEN** an actor whose `handle` returns `failure<void>(errc::resource_limit)` is delivered a `start`
- **THEN** `status_of` is `status::error`, `error_of` equals `make_error_code(errc::resource_limit)`, and `output_of` is `nullptr` (`a_handler_error_ends_its_actor_and_not_the_run`)

### Requirement: Identities: actor_ref
`actor_ref` SHALL be a struct holding `std::uint32_t value = 0`, an actor's address in its scheduler,
with `operator==` defaulted. Addresses start at 1 and follow the order the actors were spawned in,
by the host or by a turn; an address is never reused (an actor that has ended keeps its address and
its status); address 0 names no actor and is the `from` of a message the host sent. An `actor_ref`
keeps nothing alive.

#### Scenario: The first two addresses
- **WHEN** two actors are created with `create_actor` in a new scheduler
- **THEN** their addresses are 1 and 2, and a default `actor_ref{}` has value 0

### Requirement: Identities: correlation_id
`correlation_id` SHALL be a struct holding `std::uint64_t value = 0`, the execution every message
caused by one host delivery belongs to, with `operator==` and `operator<=>` defaulted so that it keys
an ordered container. The host chooses the values and names one in each `deliver`, `clock_tick` and
`release_next`; two deliveries with the same correlation share what their execution has left, so a
host gives each input a correlation of its own.

#### Scenario: Ordering
- **WHEN** `correlation_id{1} < correlation_id{2}` is evaluated
- **THEN** it is true

### Requirement: Identities: sequence_number
`sequence_number` SHALL be a struct holding `std::uint64_t value = 0`, a message's place in the order
messages entered a mailbox across every actor of a scheduler, counted from 1, with a `[[nodiscard]]
next() const noexcept` returning the number whose value is one more, and `operator==` and
`operator<=>` defaulted. The scheduler numbers a message as it enqueues it; a message dropped
because its target has ended takes no number.

#### Scenario: The next number
- **WHEN** `sequence_number{4}.next()` is read
- **THEN** its value is 5

### Requirement: Budgets: The fuel of an execution
`budgets` SHALL be a struct with one member, `std::uint32_t fuel = 1'000'000`, the fuel each execution
of a scheduler starts with. An execution starts with the whole of it and is never refilled, so the
work of one execution is bounded, whatever its actors send.

#### Scenario: The default
- **WHEN** `xactor::budgets{}.fuel` is read, and a `scheduler` is built with `budgets{}`
- **THEN** both are 1000000, and `limits().fuel` is 1000000

### Requirement: Model: The message type
Every type of the library that carries a message SHALL be a template over one `Message`, the
program's closed `std::variant` of plain structs, and the library MUST ask of it only to be a
`std::variant` that can be moved and whose `index()` the log records. The library names no message,
id or error of any user. A `Message` that is neither copyable nor default-constructible MUST pass
every path that carries an envelope.

#### Scenario: A move-only message
- **WHEN** `scheduler<std::variant<ticket>>`, where `ticket` holds a `std::unique_ptr<std::uint64_t>`, runs a counter delivered `ticket{3}` through the FIFO driver, a `clock_tick` and the Asio driver (the FIFO driver on WASI), a `release_next` and a `test_driver` step, and a `clock_tick(3)` and `run_turn(5, &log)`
- **THEN** the counter's status is `done`, `*std::get<ticket>(*output_of(...)).number` is 0 and the log holds 4 entries (`a_message_need_only_be_a_variant_that_can_be_moved`)

### Requirement: Model: The envelope
`envelope<Message>` SHALL be a struct, made by the scheduler, with the members `actor_ref to`,
`actor_ref from`, `correlation_id correlation`, `sequence_number sequence` and `Message payload`, in
that order, each value-initialized. `from` is address 0 when the host sent the message, and the
actor itself for the message of one of its timers.

#### Scenario: A built envelope
- **WHEN** an `envelope<ticket_message>` is built with designated initializers `.to = {.value = 1}, .from = {.value = 0}, .correlation = {.value = 9}, .sequence = {.value = 1}, .payload = ticket{7}` and recorded in a log with fuel 3
- **THEN** the log holds one entry (`a_move_only_message_passes_every_path_that_carries_an_envelope`)

### Requirement: Actors: actor_logic
`actor_logic<Message>` SHALL be a class with a defaulted constructor, a virtual defaulted destructor,
its copy and move constructors and assignments deleted, and one pure virtual
`result<void> handle(turn<Message>& turn, const envelope<Message>& cause)`. The scheduler calls
`handle` once for each message delivered to the actor and delivers the actor's next message only
after it returns; `turn` and `cause` live only for the call. What the logic is constructed with is
the actor's input and its members are the actor's state. The logic never sees the scheduler, its
mailbox or the driver.

#### Scenario: Not movable
- **WHEN** a class derived from `actor_logic<M>` is copied or moved
- **THEN** it does not compile, because the scheduler owns a logic where it was constructed and never moves it

### Requirement: Actors: A handler error ends only its actor
An error returned by `handle` SHALL end the actor with the status `status::error` and be kept for
`scheduler::error_of`, unless the turn has already ended the actor, in which case the actor MUST stay
as it ended and `error_of` stay empty. It ends this actor and only it: every other actor keeps its
turns, and `run_one` still returns `true`.

#### Scenario: One failing, one counting
- **WHEN** a failing actor is delivered two `start` and a counting actor one, and the system is run
- **THEN** 2 messages are delivered, the failing actor is `error` after 1 turn, the other is `active` after 1 turn

#### Scenario: An error after the actor ended
- **WHEN** an actor calls `turn.finish(...)` (or `turn.stop()`) and then returns `failure<void>(errc::resource_limit)`
- **THEN** its status is `done` (or `stopped`) and `error_of` is empty (`an_error_after_the_actor_ended_changes_nothing`)

### Requirement: Actors: create_actor
`create_actor<Logic>(system, inputs...)` SHALL construct a `Logic` from `inputs` (forwarded) with
`std::make_unique`, run it through `system.spawn` as a new actor with no parent at the next address,
and return its `actor_ref` as a `result<actor_ref>`; `Message` is deduced from `system`. The actor is
`active` at once, with no separate start: a caller that needs one delivers the actor its first
message. It never fails for a null logic since the logic it gives `spawn` is never null.

#### Scenario: A created actor receives messages
- **WHEN** `create_actor<counter>(system, seen)` is called in a scheduler of fuel 10, `deliver(*actor, start{}, correlation_id{.value = 1})` is called and a `fifo_driver` runs to idle
- **THEN** `status_of(*actor)` was `active` before the run and `seen` is 1 after it (`create_actor_spawns_an_active_actor_that_receives_messages`)

### Requirement: Scheduler: Construction
`scheduler<Message>` SHALL be constructed with `explicit scheduler(budgets limits) noexcept`, as a
system with no actor, at the time 0, whose executions each start with `limits.fuel`. `limits()`
returns the budgets it was constructed with. It starts no thread and reads no clock.

#### Scenario: A new scheduler
- **WHEN** a `scheduler<M>` is built with `budgets{}`
- **THEN** `limits().fuel` is 1000000, `now()` is 0, `idle()` is true, `pending_timers()` is 0, `next_deadline()` has no value and `fuel_of(correlation_id{9})` is 1000000

### Requirement: Scheduler: spawn
`spawn(std::unique_ptr<actor_logic<Message>>)` SHALL take an actor into the system, `active`, with no
parent, at the next address, owning the logic from then on and never moving it (a test that keeps a
pointer to the logic reads its members after each delivery), and return its `actor_ref`; for a null
pointer it MUST return `errc::invalid_argument` and take no address.

#### Scenario: A null logic
- **WHEN** `system.spawn(nullptr)` is called on a new scheduler
- **THEN** the error equals `make_error_code(errc::invalid_argument)` and `status_of(actor_ref{.value = 1})` has no value (`spawn_refuses_a_null_logic`)

#### Scenario: A spawned actor
- **WHEN** a `counter` is spawned
- **THEN** `status_of` is `status::active`, `output_of` is `nullptr` and `error_of` is empty (`a_spawned_actor_is_active_and_has_no_output`)

### Requirement: Scheduler: deliver
`deliver(actor_ref to, Message payload, correlation_id correlation)` SHALL put a message from the
host (`from` address 0) in the mailbox of `to`, put the actor in the queue if it was not there, and
open the execution `correlation` with the budget's fuel when it is new; a host delivery costs no
fuel, and nothing runs until a driver delivers it. A message to an actor that has ended MUST be
dropped and the call succeed. For an address that names no actor (0, or past the last) it MUST return
`errc::invalid_argument` and change nothing.

#### Scenario: Addresses that name no actor
- **WHEN** a scheduler with one actor is given `deliver` to `actor_ref{.value = 0}` and to `actor_ref{.value = 2}`
- **THEN** each returns an error equal to `make_error_code(errc::invalid_argument)` and `idle()` is true (`deliver_refuses_an_address_that_names_no_actor`)

#### Scenario: Delivery to a done actor
- **WHEN** a finisher actor finished on its first `start`, and a third `start` is delivered afterwards
- **THEN** `deliver` has a value, the run delivers 0 messages, the actor had 1 turn, the log has 1 entry and `idle()` is true (`a_done_actor_receives_nothing_and_nothing_is_logged`)

### Requirement: Scheduler: Delivery order
The scheduler SHALL give each actor a FIFO mailbox, and keep the actors with messages waiting in one
FIFO queue, each at most once. `run_one` takes the actor at the front of the queue, hands it the first
message of its mailbox, and puts it back at the end of the queue while its mailbox holds more. A
message an actor sends waits in a mailbox and MUST NOT be handled inside the sender's turn. So the
messages of one mailbox are handled in the order enqueued, and the delivery across actors is FIFO.

#### Scenario: One mailbox
- **WHEN** 32 lines numbered 0 to 31 are delivered to one `recorder` actor and run one by one with `run_one(nullptr)`
- **THEN** 32 messages are delivered, the actor saw "0" to "31" in order and `idle()` is true (`fifo_order_delivers_n_posts_in_order`)

#### Scenario: Across actors
- **WHEN** actors `a` and `b` are each delivered the lines 1 and 2 before anything runs, and a `fifo_driver` runs to idle
- **THEN** 4 messages are delivered and the shared record is `a1`, `b1`, `a2`, `b2` (`fifo_order_holds_across_actors`)

### Requirement: Scheduler: run_one
`result<bool> run_one(envelope_log* log)` SHALL deliver exactly one message to its actor and return
`true`, or return `false` when no message is waiting. It records the delivery in `log` when `log` is
not `nullptr`, before the actor handles it, with the fuel the message's execution has, then calls the
actor's `handle` with a `turn`. A handler's error MUST end its actor with `status::error` and not the
run, so it returns no error today; it returns a `result` so that a failure of the scheduler itself
has a channel the drivers already forward.

#### Scenario: An idle scheduler
- **WHEN** `run_one(nullptr)` is called on a scheduler with no message waiting
- **THEN** it returns a result holding `false`

### Requirement: Scheduler: Sequence numbers
The scheduler SHALL number messages from 1 in the order they enter a mailbox across all actors,
whether sent by the host, by a turn, or released by a timer; the numbering of a timer's message is
separate from the arming order of timers. A message dropped because its target is not active takes no
number.

#### Scenario: The log of a send
- **WHEN** an actor `sender` that forwards a line to `receiver` on `start` is delivered `start` in the execution 5 of a scheduler with fuel 7, and run to idle with a log
- **THEN** the log entries are: sequence 1, to `sender`, from 0, correlation 5, fuel 7, payload index 0; sequence 2, to `receiver`, from `sender`, correlation 5, fuel 6, payload index 1 (`the_log_records_each_delivery_as_it_was_made`)

### Requirement: Scheduler: Observers
The scheduler SHALL offer, all `[[nodiscard]]`: `idle()` (`noexcept`: no message waits in any mailbox,
so `run_one` would deliver nothing), `now()` (the time the last `clock_tick` or `release_next` brought,
0 before any), `pending_timers()` (how many timers are armed and not yet released), `next_deadline()`
(the earliest deadline of an armed timer, `std::nullopt` while none is), `limits()` and
`fuel_of(correlation_id)` (the fuel an execution has left, the whole budget for one not yet opened).

#### Scenario: Time before and after ticks
- **WHEN** `now()` is read on a new scheduler, after `clock_tick(42, ...)` and after `clock_tick(7, ...)`
- **THEN** it is 0, 42, then 7 (`now_is_the_last_clock_tick`)

#### Scenario: The earliest deadline
- **WHEN** an actor arms timers at 30, 10 and 20 (in that order) and ticks to 10, then 30 are brought
- **THEN** `next_deadline()` is 10, then 20, then has no value (`next_deadline_is_the_earliest_armed_timer`)

### Requirement: Scheduler: status_of, output_of and error_of
`status_of(actor_ref)` SHALL return the actor's `status` as a `result<status>`, `errc::invalid_argument`
for an address that names no actor. `output_of(actor_ref)` MUST return a pointer to the output the
actor's turn gave `finish`, `nullptr` unless its status is `done` and for an address that names no
actor; the pointer stays valid until the scheduler takes in another actor. `error_of(actor_ref)` MUST
return the error the handler returned, an empty code unless the status is `error` and for an address
that names no actor.

#### Scenario: No actor at the address
- **WHEN** `status_of` is called with `actor_ref{.value = 0}` and with `actor_ref{.value = 1}` on a new scheduler, and `stop(actor_ref{.value = 1})` is called
- **THEN** none has a value (`an_address_that_names_no_actor_has_no_status`)

#### Scenario: Finishing with the input
- **WHEN** a `finisher` constructed with "from the input" is delivered one `start` and run
- **THEN** the run delivers 1 message, the status is `done` and `std::get<written>(*output_of(address)).text` is `from the input` (`finish_makes_the_actor_done_with_its_input_as_output`)

### Requirement: Scheduler: The host's stop
`stop(actor_ref)` SHALL end an actor from outside with its whole family as `turn::stop_child` does
and return the actors it stopped, in the order it stopped them, as a `result<std::vector<actor_ref>>`;
for an address that names no actor it MUST return `errc::invalid_argument` and change nothing. Each
active member is stopped after its own descendants, siblings in spawn order, and its pending messages
and timers are dropped; a member that has already ended stays as it ended, and its descendants are
stopped all the same.

#### Scenario: A whole tree
- **WHEN** a tree R spawns C, C spawns G1 and G2, G2 spawns GG2, and the host calls `stop(R)`
- **THEN** it returns G1, GG2, G2, C, R in that order, `pending_timers()` is 0 and `idle()` is true (`the_host_s_stop_stops_the_whole_family`)

#### Scenario: Stop drops pending messages
- **WHEN** a `counter` is delivered a `start` and then stopped by the host before any turn
- **THEN** its status is `stopped`, `idle()` is true, a run delivers 0 and the actor had 0 turns (`stop_drops_the_pending_envelopes`)

#### Scenario: An ended actor stays as it ended
- **WHEN** an actor that finished with "kept" is stopped by the host
- **THEN** the call has a value, the status is still `done` and `output_of` is not `nullptr` (`stop_leaves_an_ended_actor_as_it_ended`)

### Requirement: Turns: What a turn offers
`turn<Message>` SHALL be constructed by the scheduler for each message it delivers, with
`turn(scheduler<Message>& owner, const envelope<Message>& cause) noexcept` (a program makes none),
and offer: `send(to, payload)` (1 fuel), `reply(payload)` (1), `spawn_child<Logic>(inputs...)` (0),
`self()`, `parent()`, `stop_child(child)` (0), `stop()` (0), `finish(output)` (0),
`wake_at(deadline, payload)` and `wake_at(deadline, payload, key)` (1), `cancel_timer(key)` (0),
`now()` (0), `spend(units)` (`units`) and `fuel()` (0). The turn lives for one `handle`, holds
references to the scheduler and to the message, and a logic MUST NOT keep it or a reference into the
message beyond the call.

#### Scenario: What each call costs
- **WHEN** a `pricer` actor, asked once, calls in order `send`, `reply`, `wake_at`, `wake_at` with a key, `spend(3)`, `spawn_child`, `stop_child`, `cancel_timer`, `self`/`parent`/`now`/`fuel`, and then `finish` (or `stop`), in a scheduler of fuel 100, recording the fuel before and after each
- **THEN** the costs are 1, 1, 1, 1, 3, 0, 0, 0, 0 and 0 (`each_call_of_a_turn_costs_what_it_says`, both endings)

### Requirement: Turns: send
`send(actor_ref to, Message payload)` SHALL put the message in the mailbox of `to`, its sender this
actor, for one unit of fuel, to be handled in a turn of its own later; `to` MAY be this actor. For
an address that names no actor it MUST return `errc::invalid_argument` and pay nothing; when the
execution has no fuel left, `errc::resource_limit`. A message to an actor that has ended is paid and
dropped, and the call succeeds.

#### Scenario: Sends that name no actor
- **WHEN** a `refuser` actor, the only actor (address 1) of a scheduler of fuel 5, is handed a message from the host and calls `reply`, `send(actor_ref{.value = 0}, ...)` and `send(actor_ref{.value = 2}, ...)`
- **THEN** all three return `errc::invalid_argument`, `turn.fuel()` is 5 afterwards and `fuel_of(correlation 1)` is 5 (`a_reply_to_the_host_and_a_send_to_no_actor_are_refused_unpaid`)

#### Scenario: A send to an ended actor
- **WHEN** a messenger sends a line to an actor that has finished, in the execution 2 of a scheduler of fuel 100
- **THEN** the send has a value, cost 1, `fuel_of(2)` is 99, the finished actor still had 1 turn and the log has 1 entry (`a_message_to_an_ended_actor_is_paid_and_dropped`)

### Requirement: Turns: reply
`reply(Message payload)` SHALL send to the sender of the message being handled, in its execution, as
`send` does (to this actor itself for the message of one of its timers). A message from the host has
no sender: a reply to it MUST return `errc::invalid_argument` and pay nothing.

#### Scenario: An echo
- **WHEN** the quick start's `echo` receives `ask{.value = 2}` from the counter
- **THEN** it replies `answer{.value = 2}` to the counter, for one unit of fuel

### Requirement: Turns: spawn_child, self and parent
`spawn_child<Logic>(inputs...)` SHALL construct a `Logic` from `inputs` (forwarded) and run it as a new
`active` actor of the same system at the next address, a child of the calling actor, at no fuel; the
child records the caller as its parent and the caller records its children in spawn order. Once the
caller has ended it MUST return `errc::invalid_argument` before constructing any logic. `self()`
returns the address of the actor handling the message and `parent()` the parent's address as a
`std::optional<actor_ref>`, `std::nullopt` for an actor the host spawned.

#### Scenario: A child knows its parent
- **WHEN** a parent actor spawned by the host spawns a child on its `start`
- **THEN** the child's `parent()` has the parent's address, the child's `self()` has the child's address, and the parent's own `parent()` has no value (`a_spawned_child_knows_its_parent`)

#### Scenario: A finished actor spawns nothing
- **WHEN** an actor calls `finish` and then `spawn_child<counted>(constructions)`
- **THEN** the spawn returns `errc::invalid_argument`, `status_of(actor_ref{.value = 2})` has no value and `constructions` is 0 (`a_retired_actor_spawns_nothing`)

### Requirement: Turns: stop_child
`stop_child(actor_ref child)` SHALL stop a child of the calling actor and every active actor descended
from it, each after its own descendants, siblings in spawn order, drop their pending messages and
timers, and return the actors it stopped in that order; a member that had already ended MUST stay as
it ended while its descendants are still stopped. It costs no fuel. For anything but a child of the
caller (the caller itself, a grandchild, an address that names no actor) it MUST return
`errc::invalid_argument` and change nothing.

#### Scenario: A family descendants first
- **WHEN** in the tree R, C, G1, G2, GG2, the actor R handles "stop C"
- **THEN** the returned actors are G1, GG2, G2, C, their status is `stopped`, R is `active` and `pending_timers()` goes from 5 to 1 (`stop_child_stops_a_family_descendants_first`)

#### Scenario: Refused targets
- **WHEN** R handles "stop R", "stop G1" or "stop nobody" (address 0)
- **THEN** each returns `errc::invalid_argument`, `pending_timers()` is still 5 and every actor is `active` (`stop_child_refuses_itself_a_grandchild_and_no_actor`)

#### Scenario: A done child
- **WHEN** C finished and R then handles "stop C"
- **THEN** the returned actors are G1, GG2, G2 and C's status is still `done` (`stop_child_of_a_done_child_still_stops_its_descendants`)

#### Scenario: A stranger
- **WHEN** a parent handles "stop stranger", the stranger being an actor the host created
- **THEN** the call returns `errc::invalid_argument` and the stranger is `active` (`stop_child_refuses_an_actor_that_is_not_a_child`)

### Requirement: Turns: finish and stop
`finish(Message output)` SHALL end the calling actor as `done`, keep the output for `output_of` and
drop its timers, leaving its children running; `stop()` SHALL end it as `stopped`, alone, with its
timers dropped and its children running. Both cost no fuel and both MUST return `errc::invalid_argument`,
changing nothing, once the actor has ended: an actor ends once, and a second `finish`, a `stop` after
`finish` and a `finish` after `stop` all fail. The children of an actor that ends are its logic's to
stop with `stop_child`.

#### Scenario: Finishing twice
- **WHEN** a `finisher` constructed with "first" calls `finish("first")` and then `finish("again")` in one turn
- **THEN** the second returns `errc::invalid_argument` and the output is `first` (`an_actor_finishes_once`)

#### Scenario: Three ways to end twice
- **WHEN** an actor calls `stop` then `stop`, `stop` then `finish`, or `finish` then `stop`
- **THEN** the second call returns `errc::invalid_argument`; the status is `stopped`, `stopped`, `done`, and there is an output only in the third (`an_ended_actor_neither_stops_nor_finishes_again`)

#### Scenario: Stopping oneself
- **WHEN** C of the tree handles "stop yourself"
- **THEN** C is `stopped`, R, G1, G2 and GG2 are `active` and `pending_timers()` is 4 (`an_actor_that_stops_itself_ends_alone`)

### Requirement: Turns: The rest of the turn after the actor ended
In the rest of the turn that ended the actor, the actor SHALL receive nothing more and MAY still
`send` (paid as usual), but `wake_at` and `spawn_child` MUST return `errc::invalid_argument` and pay
or construct nothing.

#### Scenario: A send after finish
- **WHEN** an actor calls `finish` and then `send` to a listening counter, in a scheduler of fuel 100
- **THEN** the send has a value and cost 1, the listener had 1 turn and the sender is `done` (`an_actor_still_sends_in_the_turn_that_ended_it`)

#### Scenario: A timer after finish
- **WHEN** an actor calls `finish` and then `wake_at(5, ...)`
- **THEN** the timer call returns `errc::invalid_argument`, `pending_timers()` is 0 and `fuel_of(1)` is the whole budget (`a_retired_actor_arms_no_timer`)

### Requirement: Turns: spend and fuel
`spend(std::uint32_t units)` SHALL pay `units` of fuel from the execution of the message being
handled for work the actor does itself, all of them or none, returning `errc::resource_limit` and
spending nothing when the execution has fewer left. `fuel()` MUST return the fuel the execution of
the message being handled has left.

#### Scenario: An actor that computes
- **WHEN** a worker that spends 3 units and then sends itself one message on each turn runs in a scheduler of fuel 10
- **THEN** it takes 3 deliveries, ends with `status::error`, `error_of` equals `errc::resource_limit` and `fuel_of(1)` is 2 (`an_actor_pays_for_its_own_work_from_its_execution_s_fuel`)

### Requirement: Turns: now
`turn::now()` SHALL return the time the last `clock_tick` or `release_next` of the scheduler brought,
0 before any, so that a delay becomes a deadline as `turn.now() + delay`.

#### Scenario: A timer sees the tick's time
- **WHEN** a keyed-timer actor's timer is released by `clock_tick(30, ...)`
- **THEN** the turn that handles it sees `now()` equal to 30 (`a_keyed_timer_replaces_the_one_with_its_key_and_a_cancelled_one_never_fires`)

### Requirement: Fuel: Executions
Every message SHALL belong to an execution named by a `correlation_id`. The first `deliver`,
`clock_tick` or `release_next` that names a correlation MUST open its execution with `budgets.fuel`,
and every message that delivery causes, through any number of actors, belongs to the same execution.
An execution not yet opened has the whole budget, whatever other executions have spent, and each
execution has fuel of its own.

#### Scenario: Each execution has its own fuel
- **WHEN** two workers that each send themselves one message per turn start in the executions 1 and 2 of a scheduler of fuel 3
- **THEN** 8 messages are delivered and both `fuel_of(1)` and `fuel_of(2)` are 0 (`each_execution_has_fuel_of_its_own`)

#### Scenario: An unopened execution
- **WHEN** a worker has used up execution 1 of a scheduler of fuel 3
- **THEN** `fuel_of(correlation_id{.value = 1})` is 0 and `fuel_of(correlation_id{.value = 7})` is 3 (`an_execution_not_yet_opened_has_the_whole_budget`)

### Requirement: Fuel: What costs
Inside a turn, `send`, `reply` and `wake_at` (with a key or without) SHALL cost one unit each and
`spend(n)` n units, all from the execution of the message being handled; `spawn_child`, `stop_child`,
`stop`, `finish`, `cancel_timer`, `self`, `parent`, `now` and `fuel` cost nothing, and so does a
delivery from the host and the release of a timer by `clock_tick` or `release_next`, since arming
was paid.

#### Scenario: Host deliveries are free
- **WHEN** three `start` are delivered in execution 1 of a scheduler of fuel 2 to an actor that sends nothing
- **THEN** 3 messages are delivered and `fuel_of(1)` is 2 (`a_host_delivery_costs_no_fuel`)

### Requirement: Fuel: All or nothing and never refilled
What the execution cannot pay SHALL fail with `errc::resource_limit` and change nothing: no message is
enqueued, no timer armed, no unit spent; a refused keyed `wake_at` MUST leave the timer under its key
armed. Nothing refills an execution: a later delivery with the same correlation shares what it has
left, and a delivery to an execution that has spent all its fuel delivers its message, whose sends
are then refused.

#### Scenario: A chain stops when the fuel runs out
- **WHEN** a worker sending one message to itself on each turn is started in a scheduler of fuel 5
- **THEN** 6 messages are delivered, 1 send is refused with `errc::resource_limit` and the actor is still `active` (`a_chain_of_messages_stops_when_its_execution_s_fuel_runs_out`)

#### Scenario: Side by side
- **WHEN** a worker sending 2 messages to itself on each turn is started in a scheduler of fuel 6
- **THEN** 7 messages are delivered and 8 sends are refused (`messages_sent_side_by_side_share_their_execution_s_fuel`)

#### Scenario: A later delivery shares what is left
- **WHEN** execution 1 of a scheduler of fuel 2 delivers 3 messages and then a second `start` is delivered in execution 1
- **THEN** the second run delivers 1 message, 2 sends were refused in total and `fuel_of(1)` is 0 (`a_later_delivery_shares_what_its_execution_has_left`)

#### Scenario: A refused keyed timer
- **WHEN** an actor armed "kept" under "k" in execution 1, and in execution 2 spends all its fuel and tries to replace it with a timer at 20
- **THEN** the replacement returns `errc::resource_limit`, and a later `clock_tick(10, ...)` fires "kept" (`a_refused_keyed_timer_leaves_the_one_with_its_key`)

### Requirement: Fuel: Reading it
`turn::fuel()` SHALL return the fuel the execution of the message being handled has left and
`scheduler::fuel_of(correlation)` the same from outside. The fuel an actor sees at the start of each
turn, and the `remaining_fuel` the log records, are the same sequence.

#### Scenario: The fuel seen on each turn
- **WHEN** a worker sending one message per turn runs in a scheduler of fuel 4
- **THEN** the fuel seen by its turns, and the `remaining_fuel` of the log entries, are 4, 3, 2, 1, 0 (`an_actor_sees_the_fuel_its_execution_has_left`)

### Requirement: Timers: Arming
`turn::wake_at(std::uint64_t deadline, Message payload)` SHALL keep the message for one unit of fuel
until the host's `clock_tick` or `release_next` reaches `deadline`, then put it in the actor's own
mailbox with the actor itself as sender; the unit of time is the host's. Nothing fires without a
tick: time never advances by itself. Once the actor has ended it MUST return `errc::invalid_argument`
and pay nothing.

#### Scenario: Nothing fires without a tick
- **WHEN** an actor arms timers at 20, 10 and 10 and the driver runs to idle
- **THEN** no timer has fired; after `clock_tick(9, ...)` none has; after `clock_tick(10, ...)` "early" and "also early" fired in that order; after `clock_tick(25, ...)` "late" fired (`clock_tick_fires_timers_in_deadline_then_sequence_order`)

### Requirement: Timers: Keyed timers
`wake_at(deadline, payload, std::string key)` SHALL arm a timer under `key`, replacing the same
actor's timer under that key, and the replacement MUST take a new arming order; keys belong to their
actor, so two actors may use the same key. `cancel_timer(std::string_view key)` MUST remove the
actor's timer under the key if it has one, costs no fuel and does nothing when there is none.

#### Scenario: Replace and cancel
- **WHEN** an actor arms "first" at 10 under "k", "second" at 20 under "k", "other" at 15 under "j" and cancels "j"
- **THEN** `pending_timers()` is 1 and `next_deadline()` is 20; after `clock_tick(30, ...)` only "second" fired, seeing `now()` 30, and `pending_timers()` is 0

#### Scenario: Keys belong to their actor
- **WHEN** two such actors run in one scheduler
- **THEN** `pending_timers()` is 2 and after `clock_tick(20, ...)` each has fired once (`keys_belong_to_their_actor`)

#### Scenario: A replacement takes a new arming order
- **WHEN** an actor arms "first" under "k", "plain" without a key, and "replacement" under "k", all due at 10
- **THEN** `pending_timers()` is 2 and after `clock_tick(10, ...)` the timers fired in the order "plain", "replacement" (`a_replacement_timer_takes_a_new_arming_order`)

### Requirement: Timers: Order of release
Due timers SHALL be released in the order of their deadlines, and timers with the same deadline in the
order they were armed, kept in an ordered map keyed by (deadline, arming sequence) so that no two
entries collide. A timer replaced, cancelled or dropped never fires.

#### Scenario: Deadline then arming order
- **WHEN** timers armed at 20 ("late"), 10 ("early") and 10 ("also early") are all due
- **THEN** they fire as "early", "also early", "late"

### Requirement: Timers: clock_tick
`result<void> clock_tick(std::uint64_t now, correlation_id correlation)` SHALL open the execution
`correlation` when it is new, set the scheduler's time to `now`, and release every timer whose
deadline is at or before `now`, in release order, into the mailbox of its owner with the owner as
sender and `correlation` as execution. It always succeeds. A time earlier than the current one is not
refused: time moves back, `now()` goes back, it releases what is due by then, and a deadline computed
afterwards from `now` is earlier than one computed before.

#### Scenario: Time moves back
- **WHEN** `clock_tick(42, ...)` and then `clock_tick(7, ...)` are called
- **THEN** `now()` is 7 afterwards (`now_is_the_last_clock_tick`)

### Requirement: Timers: release_next
`result<bool> release_next(std::uint64_t now, correlation_id correlation)` SHALL open the execution
when new, set the time to `now` (also when no timer is due), release the first timer due by then as
`clock_tick` would and return `true`, or return `false` when none is due. A host that runs the system
between two calls lets the handling of one timer cancel or replace a later timer of the same tick.

#### Scenario: One at a time
- **WHEN** timers at 20, 10 and 10 are armed and `release_next(15, ...)` is called in a loop with a run to idle after each, until it returns false
- **THEN** the number of timers heard after each is 1 then 2, the fired order is "early", "also early", `now()` is 15 and `pending_timers()` is 1 (`release_next_releases_one_due_timer_at_a_time`)

### Requirement: Timers: Released timers belong to the releasing execution
A timer released by `clock_tick` or `release_next` SHALL belong to the execution that call names, its
message's sender is its own actor, and releasing it MUST cost nothing.

#### Scenario: A free release
- **WHEN** in a scheduler of fuel 3 an actor arms 3 timers (spending the whole of execution 1), then `clock_tick(10, ticked)` and `release_next(20, released)` with executions 2 and 3 are called and run with a log
- **THEN** the log entries are sequence 2, to and from the actor, correlation 2, fuel 3, payload index 1, and sequence 3, to and from the actor, correlation 3, fuel 3, payload index 1, and `fuel_of(2)` and `fuel_of(3)` are 3 (`a_released_timer_belongs_to_the_releasing_execution_for_free`)

### Requirement: Timers: An ended actor has no timers
An actor that ends (`done`, `error` or `stopped`) SHALL lose its timers: they are dropped when it
retires.

#### Scenario: Stopping
- **WHEN** an actor with 3 armed timers is stopped by the host
- **THEN** `pending_timers()` goes from 3 to 0 (`a_retired_actor_s_timers_are_dropped`)

#### Scenario: Finishing and failing
- **WHEN** an actor arms a timer and then finishes, or fails with `errc::resource_limit`
- **THEN** its status is not `active` and `pending_timers()` is 0 (`finishing_and_failing_drop_the_actor_s_timers`)

### Requirement: Lifecycle: Statuses
`status` SHALL be `enum class status : std::uint8_t` with the fixed values `active = 1` (receives
messages; every actor starts here), `done = 2` (finished with an output through `turn::finish`),
`error = 3` (its handler returned an error) and `stopped = 4` (stopped by `scheduler::stop`, its
parent's `stop_child` or an ancestor's, or its own `stop`). The values never change and are never
reused, because a system that persists its actors rebuilds them from these numbers. `done`, `error`
and `stopped` are final.

#### Scenario: The persisted values
- **WHEN** the statuses are cast to `int`
- **THEN** they are 1, 2, 3 and 4 (`the_statuses_keep_their_persisted_values`)

### Requirement: Lifecycle: Only an active actor receives messages
Once an actor has ended, its pending messages SHALL be dropped, never delivered and never logged, and
so MUST be every later message to it. Dropping is not an error of the sender's. The actor is removed
from the queue of actors with work.

#### Scenario: A second start in the mailbox
- **WHEN** two `start` wait in the mailbox of a finisher before the first turn
- **THEN** the run delivers 1 message and the actor had 1 turn (`a_done_actor_receives_nothing_and_nothing_is_logged`)

### Requirement: Lifecycle: Families
An actor spawned in a turn SHALL be a child of that turn's actor, which records it among its children
in spawn order; the actor spawned by the host has no parent. The host's `stop` and a parent's
`stop_child` stop a family in the order of a postorder with siblings in spawn order (descendants
first), computed with an explicit stack and no recursion. xactor does not tell a parent that its
child ended: a child that must tell its parent sends it a message, to `turn.parent()`, in the turn it
ends.

#### Scenario: A tree stopped from the host
- **WHEN** the tree R, C, G1, G2, GG2 is built (R spawns C; C spawns G1 then G2; G2 spawns GG2) and `stop(R)` is called
- **THEN** the order is G1, GG2, G2, C, R

#### Scenario: The lifecycle example
- **WHEN** `example/xactor_lifecycle.cpp` runs: a supervisor spawns a doubler (actor 2) that finishes with 42, a failing child (3) that returns `invalid_argument`, and an idle child (4); then the host calls `stop` on the supervisor and delivers it another `start`
- **THEN** the output is `actor 1: active`, `actor 2: done`, `actor 3: error`, `actor 4: active`, `actor 2 finished with 42`, `actor 3 failed with invalid_argument`, `stopped: 4 1`, `actor 1: stopped`, `actor 2: done`, `actor 3: error`, `actor 4: stopped`, `delivered to a stopped actor: 0`

### Requirement: Log: logged_envelope
`logged_envelope` SHALL be a struct, one entry of an `envelope_log`, with the members
`sequence_number sequence`, `actor_ref to`, `actor_ref from`, `correlation_id correlation`,
`std::uint32_t remaining_fuel`, `std::size_t payload_index` (the `index()` of the message variant),
and a defaulted `operator==` comparing every member. It keeps which alternative arrived, not the
message, so an entry has a fixed width, the log is one type whatever the `Message`, and two logs
compare cheaply.

#### Scenario: Entries compare member by member
- **WHEN** two entries differ only in `remaining_fuel`
- **THEN** they are not equal

### Requirement: Log: envelope_log
`envelope_log` SHALL be a class with `template <class Message> void record(const envelope<Message>&,
std::uint32_t remaining_fuel)` appending the entry of a delivered message (its `payload.index()`),
`[[nodiscard]] const std::vector<logged_envelope>& entries() const noexcept`, and a friend
`operator==` comparing the entries, equal in the same order. `run_one` records each message it
delivers in the log it is given, before the actor handles it, with the fuel its execution had then,
and nothing for a message dropped; a `run_one` given `nullptr` or another log records nothing in this
one.

#### Scenario: Two logs
- **WHEN** a default `envelope_log` is compared with one that holds entries
- **THEN** they are not equal, and two default logs are equal

### Requirement: Drivers: fifo_driver
`fifo_driver<Message>` SHALL be constructed with `explicit fifo_driver(scheduler<Message>& owner)
noexcept`, holding the scheduler by reference (it MUST outlive the driver), and offer
`result<std::size_t> run_until_idle(envelope_log* log)`, which calls `run_one` until none is left and
returns how many it delivered (the scheduler then idle), and `result<std::size_t> run_turn(std::size_t
budget, envelope_log* log)`, which delivers at most `budget` messages, fewer when they run out first.
Both return the first failure of `run_one`, which stops them, and `nullptr` records nothing.

#### Scenario: Turn by turn
- **WHEN** 5 lines are delivered to a recorder and `run_turn(2, nullptr)`, `run_turn(0, nullptr)`, `run_turn(5, nullptr)` are called, then 2 more lines are delivered and `run_until_idle(nullptr)` is called
- **THEN** they return 2, 0, 3 and 2; after the first `idle()` is false, after the third true, and the recorder saw 7 lines (`run_turn_delivers_at_most_its_budget`)

### Requirement: Drivers: test_driver
`test_driver<Message>` SHALL be constructed from a scheduler like `fifo_driver` and offer exactly
`result<bool> step()` and `result<bool> step(envelope_log& log)`, each one `run_one`, returning
whether there was a message to deliver (`step()` records nothing). It has nothing else: messages
reach its scheduler through `deliver`, and time through `clock_tick` and `release_next`.

#### Scenario: One message a step
- **WHEN** 3 lines are delivered to a recorder and `step()` is called four times
- **THEN** the recorder saw 0 lines before, then 1, 2, 3 after each of the first three steps, which return true, and the fourth returns false (`test_driver_delivers_exactly_one_message_per_step`)

### Requirement: Drivers: asio_driver
`asio_driver<Message>`, declared only when `__wasi__` is not defined, SHALL be constructed from a
scheduler, own an `io_context` of concurrency hint 1, an `executor_work_guard` and a strand, and offer
`result<std::size_t> run_until_idle(envelope_log* log)`: while the scheduler is not idle and no
failure occurred, post one `run_one` to the strand and drain the context with `poll`, never `run`,
`run_one` or `run_for`; it returns how many messages were delivered or the first failure. The context
is the driver's alone and offers no executor.

#### Scenario: The same log as the FIFO driver
- **WHEN** 8 lines are delivered to one recorder in each of two schedulers, run with a `fifo_driver` and an `asio_driver` with a log each
- **THEN** each log has 8 entries and the two logs are equal (`both_drivers_produce_the_same_envelope_log`)

#### Scenario: The Asio example
- **WHEN** `example/xactor_asio.cpp` runs the rally with both drivers
- **THEN** it prints `fifo_driver delivered 5, asio_driver delivered 5` and `the two logs are equal: true`

### Requirement: Drivers: Asio is native only
`drivers.hpp` SHALL include `boost/asio/executor_work_guard.hpp`, `io_context.hpp`, `post.hpp` and
`strand.hpp` only under `#ifndef __wasi__`, each marked `// lint-world: posts handlers only`, and
declare `asio_driver` there alone; on WASI neither the driver nor Boost.Asio's headers enter the
build (Boost.Asio 1.92 does not compile for wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34). The
one program built natively only is `example/xactor_asio.cpp` (`webcpp.example xactor_asio.cpp : :
native`), and the tests that use `asio_driver` are inside `#ifndef __wasi__`.

#### Scenario: A WASI build
- **WHEN** a program that includes `<webcpp/xactor.hpp>` is built for wasm32-wasip2 or wasm32-wasip3
- **THEN** it compiles without Boost.Asio, which is the check that nothing of Asio enters a WASI build

### Requirement: Drivers: The rally example
The example `xactor_drivers` SHALL play a rally between two players on two schedulers, one run with a
`test_driver` recording each step and one with a `fifo_driver` run with `run_turn(2, log)` and then
`run_until_idle(log)`, and compare their logs.

#### Scenario: The output
- **WHEN** `example/xactor_drivers.cpp` runs
- **THEN** it prints `test_driver: 5 steps`, `  #1 to 1 from 0, fuel 100`, `  #2 to 2 from 1, fuel 99`, `  #3 to 1 from 2, fuel 98`, `  #4 to 2 from 1, fuel 97`, `  #5 to 1 from 2, fuel 96`, `fifo_driver: run_turn(2) delivered 2, idle false`, `fifo_driver: run_until_idle delivered 3, idle true` and `the two logs are equal: true`

### Requirement: Determinism: The same deliveries make the same run
Two schedulers built with the same budgets and fed the same deliveries SHALL record equal envelope
logs, whatever driver runs them and however the run is cut into turns. The order of deliveries
depends on the order of enqueuing alone.

#### Scenario: Cutting a run
- **WHEN** a rally is run with `run_turn(2, log)` and then `run_until_idle(log)`
- **THEN** the log equals that of the same rally run with a `test_driver`, step by step

### Requirement: Determinism: Rules for the actors
The documentation SHALL state the rules by which an actor keeps its run deterministic: read no clock
in a turn (take the time from `turn.now()` or from the message a timer brings); read nothing from
the world in a turn and never wait (the host reads and delivers a message); draw no random number
but from a seed that a message brought; keep no static mutable state; throw nothing and return an
error instead; keep no reference to the turn or to a message beyond the turn. A program checks that
its run is deterministic by running the same deliveries twice and comparing the logs.

#### Scenario: The page states them
- **WHEN** `doc/determinism.adoc` is read
- **THEN** it lists those six rules

### Requirement: Static actors: A state machine joined by the program
xactor SHALL hold no adapter for a state machine library: an actor logic is any class with a
`handle`, so a logic holds a machine as a member and hands it each message, and the machine's actions
reach the system through the turn, carried in an event type that holds the message and the turn by
reference. The example `xactor_msm` MUST write a turnstile with Boost.MSM's classic back-end and
functor front-end, used through its headers alone (not `/boost/msm//boost_msm`, which brings the
compiled Boost.Serialization).

#### Scenario: The turnstile
- **WHEN** `example/xactor_msm.cpp` runs: a visitor sends a coin, a push, a push, a coin and a coin to the turnstile
- **THEN** it prints `turnstile: locked`, `turnstile: unlocked`, `turnstile: locked`, `turnstile: unlocked`, `visitor: alarm` and `visitor: refund`

### Requirement: Guarantees: The numbered guarantees
The page SHALL state 36 numbered guarantees under the anchors `xactor-invariant-1` to
`xactor-invariant-36`, each keeping its number so that code and tests cite it as `(doc: #xactor-invariant-<n>)`;
the page's build MUST fail on a citation that names no anchor. They are: 1 mailbox FIFO; 2 queue
FIFO across actors; 3 an actor in the queue at most once; 4 `run_one` handles exactly one message
and a handler's error ends only its actor; 5 `deliver`, `send`, `reply` and a released timer queue an
active target; 6 one execution per correlation, opened once, never refilled, host deliveries free;
7 what costs; 8 all or nothing; 9 `turn::fuel` and `fuel_of`; 10 `reply` and `send` that name no
sender or actor; 11 timers fire in (deadline, arming order); 12 no timer fires without a tick;
13 a released timer belongs to the call's execution; 14 `run_until_idle` returns only when idle,
`run_turn(n)` handles at most n; 15 `fifo_driver` and `asio_driver` produce identical logs; 16 what
the log records; 17 equal logs for equal deliveries; 18 no clock, disk or network; 19 no turn
blocks; 20 no exception of its own; 21 no static mutable state; 22 `test_driver` exposes `step`
alone; 23 only an active actor receives messages; 24 `finish` and ending once; 25 the host's `stop`;
26 `output_of` and `error_of`; 27 the values of `status`; 28 the library names nothing of any user
and asks only a movable `std::variant`; 29 `create_actor`, `spawn_child` and `spawn`; 30 parent and
children, `self`; 31 `stop_child`; 32 keyed timers; 33 an ended actor arms and spawns nothing;
34 `now()` and `next_deadline()`; 35 `turn::stop()`; 36 `release_next`.

#### Scenario: A citation resolves
- **WHEN** `test/scheduler_test.cpp` cites `(doc: #xactor-invariant-14)` above `run_turn_delivers_at_most_its_budget`
- **THEN** `doc/guarantees.adoc` holds the anchor `[[xactor-invariant-14]]` for the sentence on `run_until_idle` and `run_turn`

### Requirement: Tests: Programs and registration
The tests SHALL be Boost.Core lightweight_test programs in `test/`, one for each subject, each
declared in `test/Jamfile` with `webcpp.run` for native, wasip2 and wasip3: `scheduler`
(`scheduler_test.cpp`), `lifecycle`, `fuel` and `create_actor`; each `main` calls its cases, which are
functions of the file's anonymous namespace named after the rule they check, and returns
`boost::report_errors()`. A case that cannot go on after a failed check MUST return
(`if (!BOOST_TEST(...)) { return; }`); a helper, which cannot return from its case, MUST call
`webcpp::test::require` (`test/require.hpp`), which ends the program with `std::exit(boost::report_errors())`
unless the check held. A program that passes prints `No errors detected.` on its standard error and
returns 0.

#### Scenario: The four programs
- **WHEN** `scheduler_test.cpp`, `lifecycle_test.cpp`, `fuel_test.cpp` and `create_actor_test.cpp` are compiled natively and run
- **THEN** each prints `No errors detected.` and exits 0

### Requirement: Tests: The scheduler program
`scheduler_test.cpp` SHALL hold 21 cases (20 on WASI, where `both_drivers_produce_the_same_envelope_log`
is absent): `fifo_order_delivers_n_posts_in_order`, `fifo_order_holds_across_actors`,
`both_drivers_produce_the_same_envelope_log`, `clock_tick_fires_timers_in_deadline_then_sequence_order`,
`test_driver_delivers_exactly_one_message_per_step`,
`a_keyed_timer_replaces_the_one_with_its_key_and_a_cancelled_one_never_fires`,
`keys_belong_to_their_actor`, `a_retired_actor_s_timers_are_dropped`, `now_is_the_last_clock_tick`,
`a_refused_keyed_timer_leaves_the_one_with_its_key`, `finishing_and_failing_drop_the_actor_s_timers`,
`a_retired_actor_arms_no_timer`, `next_deadline_is_the_earliest_armed_timer`,
`release_next_releases_one_due_timer_at_a_time`, `deliver_refuses_an_address_that_names_no_actor`,
`run_turn_delivers_at_most_its_budget`, `a_replacement_timer_takes_a_new_arming_order`,
`a_released_timer_belongs_to_the_releasing_execution_for_free`,
`the_log_records_each_delivery_as_it_was_made`, `a_message_need_only_be_a_variant_that_can_be_moved`
and `a_move_only_message_passes_every_path_that_carries_an_envelope`. Its messages are the test's own.

#### Scenario: The move-only path
- **WHEN** a parent (`ticket_parent`), a quitter and an idle echo are created with a move-only `ticket` message, the parent and quitter are delivered tickets, the run delivers 4 messages, a `clock_tick(1, ...)` releases one, and the host stops the idle actor
- **THEN** the parent is `error` with `errc::resource_limit`, its echo child and the quitter are `stopped`, `stop(*idle)` returns just `*idle`, the log has 6 entries and the first records `remaining_fuel` 3 (`a_move_only_message_passes_every_path_that_carries_an_envelope`)

### Requirement: Tests: The lifecycle program
`lifecycle_test.cpp` SHALL hold 23 cases covering status, input, output, error, stop and families:
`a_spawned_actor_is_active_and_has_no_output`, `spawn_refuses_a_null_logic`,
`an_address_that_names_no_actor_has_no_status`, `finish_makes_the_actor_done_with_its_input_as_output`,
`an_actor_finishes_once`, `a_done_actor_receives_nothing_and_nothing_is_logged`,
`a_handler_error_ends_its_actor_and_not_the_run`, `stop_drops_the_pending_envelopes`,
`stop_leaves_an_ended_actor_as_it_ended`, `the_statuses_keep_their_persisted_values`,
`a_spawned_child_knows_its_parent`, `stop_child_stops_the_child_and_its_descendants`,
`stop_child_refuses_an_actor_that_is_not_a_child`, `stop_child_stops_a_family_descendants_first`,
`stop_child_refuses_itself_a_grandchild_and_no_actor`,
`stop_child_of_a_done_child_still_stops_its_descendants`, `the_host_s_stop_stops_the_whole_family`,
`a_retired_actor_spawns_nothing`, `an_actor_that_stops_itself_ends_alone`,
`an_ended_actor_neither_stops_nor_finishes_again`, `an_error_after_the_actor_ended_changes_nothing`,
`an_actor_still_sends_in_the_turn_that_ended_it` and `a_message_to_an_ended_actor_is_paid_and_dropped`.

#### Scenario: A grandchild is not a child
- **WHEN** the child's descendants are stopped by its parent while the grandchild has a message pending
- **THEN** the stopped actors are 2, the child and grandchild are `stopped`, the parent is `active`, and the grandchild had 1 turn only (`stop_child_stops_the_child_and_its_descendants`)

### Requirement: Tests: The fuel program
`fuel_test.cpp` SHALL hold 10 cases: `a_chain_of_messages_stops_when_its_execution_s_fuel_runs_out`,
`messages_sent_side_by_side_share_their_execution_s_fuel`,
`an_actor_pays_for_its_own_work_from_its_execution_s_fuel`,
`an_actor_sees_the_fuel_its_execution_has_left`, `each_execution_has_fuel_of_its_own`,
`a_host_delivery_costs_no_fuel`, `an_execution_not_yet_opened_has_the_whole_budget`,
`a_later_delivery_shares_what_its_execution_has_left`,
`a_reply_to_the_host_and_a_send_to_no_actor_are_refused_unpaid` and
`each_call_of_a_turn_costs_what_it_says`.

#### Scenario: The price list
- **WHEN** `each_call_of_a_turn_costs_what_it_says` runs
- **THEN** the recorded list is send 1, reply 1, wake_at 1, wake_at with a key 1, spend 3, spawn_child 0, stop_child 0, cancel_timer 0, "self, parent, now and fuel" 0, and finish (or stop) 0

### Requirement: Tests: create_actor and the config compile tests
`create_actor_test.cpp` SHALL hold one case, `create_actor_spawns_an_active_actor_that_receives_messages`.
`test/Jamfile` MUST also declare `webcpp.compile` for `config_test.cpp` three times: `config` (as the
build asks), `config_developer_defined` (with `<define>WEBCPP_XACTOR_NO_EXCEPTIONS` and
`<define>WEBCPP_TEST_XACTOR_DEVELOPER_DEFINED`) and `config_all_off` (with `<define>WEBCPP_NO_EXCEPTIONS` and
`<define>WEBCPP_TEST_XACTOR_ALL_OFF`).

#### Scenario: Three compiles
- **WHEN** `config_test.cpp` is compiled in the three ways
- **THEN** each compiles without error, and none stops at an `#error`

### Requirement: Tests: Every public header compiles alone
`test/Jamfile` SHALL declare `webcpp.headers-alone xactor : ../include`, which compiles each public
header as the only include of its translation unit, so that a header that leans on an include it does
not make fails naming it.

#### Scenario: A header that leans
- **WHEN** a public header used `std::vector` without including `<vector>` and nothing else included it first
- **THEN** its headers-alone compile fails naming that header

### Requirement: Tests: The conversion record
`test/CONVERSION.md` SHALL record the conversion of the four suites from Boost.Test to lightweight_test
case by case: the correspondence of each macro, the counts of assertions (55 cases, 295 assertions in
the source, 636 executed, equal old and new, with 126, 127, 37 and 5 for `scheduler`, `lifecycle`,
`fuel` and `create_actor`), the five differences found in a one-by-one comparison of 295 pairs, the
differences in behaviour (an exception that escapes a case ends the program; a failed `require` in a
helper ends the program), and three planted defects, each shown to fail the same cases in the old and
the new suites.

#### Scenario: The planted defects
- **WHEN** `take` is changed to refuse when `remaining <= units`, or `finish` retires its actor as `status::stopped`, or `timer::payload` becomes `Message payload{};`
- **THEN** `fuel` and `scheduler` fail 8 cases with 15 failed assertions, `lifecycle` and `scheduler` fail 7 cases with 7 failed assertions, and under clang-18 with libstdc++ `scheduler` does not compile ("no matching constructor for initialization of `std::variant<ticket>`"), respectively (`test/CONVERSION.md`)

### Requirement: Tests: Examples are tests
`example/Jamfile` SHALL build every program the page shows with `webcpp.example`, run it, and compare
what it printed on its standard output with the `<name>.expected` beside it; a program that returns
anything but 0 fails even when it printed the expected lines. The programs are `xactor_quick_start.cpp`,
`xactor_lifecycle.cpp`, `xactor_fuel.cpp`, `xactor_timers.cpp`, `xactor_drivers.cpp`,
`xactor_testing.cpp` and `xactor_msm.cpp` for native, wasip2 and wasip3, and `xactor_asio.cpp` for
native only. The `// tag::<name>[]` and `// end::<name>[]` lines mark what the page includes.

#### Scenario: The testing example prints nothing
- **WHEN** `example/xactor_testing.cpp` runs
- **THEN** it prints nothing on its standard output (its `.expected` is empty), `No errors detected.` on its standard error and returns 0

### Requirement: Tests: The fuel example
The example `xactor_fuel` SHALL show a relay that sends itself a message on every turn and prints
the fuel it sees, run with a budget of 3 in executions 1 and 2, and a worker that spends 2 units on
every turn, run in execution 3.

#### Scenario: The output
- **WHEN** `example/xactor_fuel.cpp` runs
- **THEN** it prints `relay: fuel 3`, `relay: fuel 2`, `relay: fuel 1`, `relay: fuel 0`, `relay: refused, resource_limit`, `delivered 4, relay is active`, the same five relay lines again, `execution 1 has 0 left, execution 2 has 0 left` and `worker is error (resource_limit), execution 3 has 0 left`

### Requirement: Tests: The timers example
The example `xactor_timers` SHALL show a kettle whose boil is a keyed timer replaced by `delay` and
removed by `cancel`, driven with `clock_tick` and `release_next`, printing the armed timers and the
earliest deadline.

#### Scenario: The output
- **WHEN** `example/xactor_timers.cpp` runs
- **THEN** it prints `timers 2, next deadline 100`, `timers 2, next deadline 120`, `whistle at 130`, `timers 1, next deadline 150`, `boiled at 200`, `timers 0, next deadline none`, `timers 1, next deadline 320`, `whistle at 400` and `timers 0, next deadline none`

### Requirement: Tests: The quick start
The quick start SHALL have a counter ask an echo for 1, 2 and 3, the echo reply with the same number,
and the counter finish with the sum when it has three answers; the host creates both with
`create_actor`, delivers `start` in execution 1 of a scheduler of fuel 100 and runs a `fifo_driver`.

#### Scenario: The output
- **WHEN** `example/xactor_quick_start.cpp` runs
- **THEN** it prints `delivered 7 messages` and `total 6, fuel left 94`

### Requirement: Tests: The test matrix
Continuous integration SHALL run the tests and examples in lanes, one toolset each (the native
compilers, wasip2 and wasip3), each building from scratch with `b2 -a --dump-tests
--out-xml=<lane>.xml toolset=<toolset> libs/xactor/test libs/xactor/example`; webcpp's report tool
merges the lanes into a matrix, a cell for each program and lane, and a lane in which nothing was
built or that was built for another target than its name says fails by name. `.github/workflows/ci.yml`
MUST call the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main`
with `library: xactor`, on pushes to `main`, pull requests and manual dispatch, with
`permissions: contents: read`, and never cancel a run on `main`.

#### Scenario: The workflow
- **WHEN** `.github/workflows/ci.yml` is read
- **THEN** its job `ci` uses `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: xactor`, and `cancel-in-progress` is `${{ github.ref != 'refs/heads/main' }}`

### Requirement: Documentation: The page
`doc/xactor.adoc` SHALL be xactor's page, titled "xactor: a deterministic actor system for C++", built
by `webcpp.doc xactor : xactor.adoc` and `webcpp.reference xactor` (`doc/Jamfile`), and include in this
order: introduction, quick start, the model, writing an actor logic, the scheduler, turns, fuel,
timers, lifecycle, drivers, static actors, determinism, testing, guarantees, the reference (from the
Doc Comments), the glossary and the changelog. Every listing includes a compiled example and every
output an `.expected` file, and the page states no count of the tree's. Every `@see "<title>"` of a
Doc Comment and `(doc: #<anchor>)` of a `//` comment MUST name a section of the page, or the build
fails.

#### Scenario: The sections
- **WHEN** `doc/xactor.adoc` is read
- **THEN** it includes `introduction.adoc`, `quick-start.adoc`, `model.adoc`, `logic.adoc`, `scheduler.adoc`, `turns.adoc`, `fuel.adoc`, `timers.adoc`, `lifecycle.adoc`, `drivers.adoc`, `static-actors.adoc`, `determinism.adoc`, `testing.adoc`, `guarantees.adoc`, the reference, `glossary.adoc` and `changelog.adoc`

### Requirement: Documentation: The reference
Every public symbol (function, class, struct, enum, enumerator, alias, data member and macro) SHALL
have a Doc Comment, which MrDocs turns into the API reference, built strict:
`b2 libs/xactor/doc//reference` MUST fail on any symbol, parameter or return value left undocumented.
`doc/mrdocs.yml` sets `show-enum-constants: true`, so that each enumerator has a section of its own
with its value and whole Doc Comment (the values of `errc` and `status` are fixed).

#### Scenario: The enumerators
- **WHEN** `doc/mrdocs.yml` is read
- **THEN** it holds `show-enum-constants: true`

### Requirement: Documentation: The glossary
The page SHALL close with a glossary in alphabetical order, each entry saying what the term means
here, what it is in the code and which section explains it, with the anchors `glossary-<term>`
(for example `glossary-actor`, `glossary-actor-logic`, `glossary-driver`, `glossary-envelope`,
`glossary-envelope-log`).

#### Scenario: An entry
- **WHEN** the glossary entry "Envelope" is read
- **THEN** it names `xactor::envelope<Message>` with `to`, `from`, `correlation`, `sequence`, `payload` and says `from` is the address 0 for the host

### Requirement: Documentation: The changelog
`doc/changelog.adoc` SHALL be an appendix (`xactor-changelog`) listing, newest first, one line per
change that a user sees, in a section "Unreleased" (`xactor-changelog-unreleased`) between
`// tag::unreleased[]` and `// end::unreleased[]`. For 0.1.0 it lists: the removed RTTI guarantee
(breaking), the guarantee without exceptions changed to compiling only, the macro
`WEBCPP_XACTOR_NO_EXCEPTIONS`, the documentation update (a link to xstate and the changelog),
`config.hpp` including `<webcpp/config.hpp>`, and `WEBCPP_NO_EXCEPTIONS`.

#### Scenario: The first entry
- **WHEN** the section "Unreleased" is read
- **THEN** its first line starts "*Breaking change:* removed the guarantee that xactor runs without RTTI"

### Requirement: Documentation: Introduction and requirements
The introduction SHALL state what an actor is, the three promises (the same input makes the same run;
every execution ends; time is the host's), and the requirements: a C++20 compiler, Boost's headers
1.92 or newer, webcpp's configuration, a definition of `boost::throw_exception` in a build without
exceptions (Boost defines it when exceptions are on), and nothing else. It MUST say that what an
actor system may add (a child's end reaching its parent, a registry of actors by name, the
inspection of what each actor does) is left to the program or to a library built on xactor.

#### Scenario: What is left out
- **WHEN** `doc/introduction.adoc` is read
- **THEN** it says that a child that must tell its parent that it ended sends it a message, to `turn.parent()`, in the turn it ends

### Requirement: Documentation: Testing a program's actors
The testing chapter SHALL show how a program tests its actors with Boost.Core's lightweight_test and a
`test_driver`, in `example/xactor_testing.cpp`: a test case that delivers a `start` to a client of an
echo in a scheduler of fuel 10 and checks after each step what that message changed (the fuel of the
execution and the answers the client kept), and a case that runs the same deliveries twice and
checks that the two envelope logs are equal. It MUST state why a run is repeatable: there is no real
clock, no thread, a failure is a value and work is counted.

#### Scenario: The steps
- **WHEN** `the_client_keeps_what_the_echo_answers` runs with `start{.value = 7}` in execution 1
- **THEN** after the first step `fuel_of(execution)` is 9, after the second 8 with no answer kept, after the third the client has kept `{7}`, and a fourth step returns false and `idle()` is true

#### Scenario: Two runs
- **WHEN** `two_runs_record_the_same_log` runs the client and echo on the starts 1, 2 and 3 twice
- **THEN** each log has 9 entries and the two logs are equal

### Requirement: Documentation: Building the quick start
The quick start SHALL give the compiler command that builds the program from the root of a webcpp
checkout, `clang++ -std=c++20 -I libs/xactor/include -I libs/config/include quick_start.cpp -o quick_start`,
and the command for a build without exceptions, which adds `-fno-exceptions -DBOOST_NO_EXCEPTIONS` and
links webcpp's `tools/throw_exception.cpp` (a definition of `boost::throw_exception` that reports what
was thrown on the standard error and aborts). It MUST note that a compiler that does not find Boost on
its default include path is given Boost's `include` directory with `-isystem`.

#### Scenario: The commands
- **WHEN** `doc/quick-start.adoc` is read
- **THEN** it holds both commands and says that without exceptions the program also needs a definition of `boost::throw_exception`, which Boost.System's `result` names
