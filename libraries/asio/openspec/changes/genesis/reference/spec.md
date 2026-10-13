---
title: "webcpp::asio: Boost.Asio natively, the browser's event loop on Emscripten"
tags: [webcpp, asio, emscripten, spec]
status: active
created: 2026-10-10
---

# webcpp::asio: specification (draft)

Sources cited below: `B/` is Boost 1.92's headers, `/opt/homebrew/include/boost`; `BS/` is
Boost 1.92's sources, `~/dev/webcpp/.local/boost-source`; `E/` is Emscripten 6.0.11,
`~/dev/webcpp/.local/emsdk/upstream/emscripten`; `B2/` is b2 5.5.3, `/opt/homebrew/share/b2/src`;
`P/` is the review's probes, `~/dev/webcpp/.superpowers/sdd/plan-reviews/asio-beast/`, built with
em++ 6.0.11 (`-std=c++20 -O1 -DBOOST_ASIO_DISABLE_THREADS`) and run with node 26.7.0. The review
is `asio-beast-review.md` beside them.

## 1. Purpose

A program written against Boost.Asio, `main()` and `io_context::run()` included,
runs unchanged in a browser. Natively, webcpp::asio is Boost.Asio itself, through
a bridge that costs nothing. Compiled with Emscripten, it is still Boost.Asio's own
`io_context`, whose `run` already suspends into the browser's event loop (section 4.1);
webcpp::asio adds only what Boost lacks there: the configuration Asio needs to build, what
wakes `run` from a browser event, and a limit on the handlers one turn runs, so that the page
keeps rendering. The program's code runs only when the browser delivers an event (a message,
a promise, a timer), never by polling, and its stack and objects stay as they were. Timers are
Boost's on both targets, so that a library written on webcpp::asio (trystero, first), and
every Boost library built on Asio, never knows where it runs.

## 1.1 Why: Boost.Asio's world in the browser, and the browser's in C++

Boost.Asio is the ground of asynchronous C++: Boost.Beast, Boost.Cobalt, Boost.MySQL,
Boost.Redis and many others are written on it, as templates over its executors, its
completion tokens and its stream concepts. webcpp::asio keeps all of that and changes one
thing, where the waiting happens, so that:

- **C++'s asynchronous libraries reach the browser.** What is written on Asio's model
  runs in a page with the same code, because the io_context, the executors and the timers
  are Boost's own:
  - **Boost.Cobalt** runs on webcpp::asio's io_context and executors unchanged, its
    timers included (Cobalt's `io::steady_timer` wraps Asio's,
    `B/cobalt/io/steady_timer.hpp:21-34`). Its `co_main` runs unchanged too (measured,
    `P/cobalt_main.cpp`, JSPI); webcpp::asio gives it the turn limit by replacing the one
    function of Cobalt's that runs the io_context (section 5.2).
  - **Boost.Beast** is templates over stream concepts (`AsyncReadStream`,
    `AsyncWriteStream`): its messages, parsers, serializers and buffers compile and run in
    the browser as they are, and its timeouts are Boost timers
    (`B/beast/core/basic_stream.hpp:251`, `B/beast/websocket/impl/stream_impl.hpp:72`), which
    fire there. Its HTTP and WebSocket algorithms run unchanged over a byte stream a program
    names as their next layer: a socket of webcpp::asio's whose transport a later library
    provides (section 7), or a WebRTC channel's, in webcpp::webrtc. Beast's own `tcp_stream`
    wraps Boost's socket (`B/beast/core/basic_stream.hpp:210-211`), never webcpp's, so a
    relay does not reach it. Over plain TCP nothing runs, since a browser gives no raw
    sockets; there webcpp::beast (section 6) gives Beast's interfaces over the browser's own
    `WebSocket` and `fetch`.
  - Any other library written on Asio's model (completion tokens, executors) follows.
- **The browser's asynchronous model reaches C++.** The web's APIs are asynchronous:
  promises, events, streams. webcpp::asio makes a promise and an event source Asio
  operations (section 4.7): a JavaScript promise is awaitable with any completion token
  (`co_await`, a callback, a Cobalt task), an event source is an operation that completes on
  the event, so that C++ writes the web's programs in the web's own model, and in C++'s.

Emscripten 6.0.11 already makes Boost.Asio's `io_context::run` suspend into the browser
(section 4.1). A later step, outside this library, offers Boost.Asio what webcpp::asio adds
upstream (the threads configuration on Emscripten, the turn limit, the wake from browser
events); webcpp::asio is where that is proven first.

## 2. The owner's decisions (2026-10-10)

1. **JSPI and Asyncify are build variants** of the emscripten target, decided at build
   time; each artifact is named after its variant, and the developer ships the one a
   page's loader picks for its browser.
2. **What a browser cannot do compiles and fails as a value**: TCP and UDP sockets and
   name resolution return `operation_not_supported`, through a specialisation point
   where a later strategy (relays) can provide them transparently.
3. **WebRTC is a library of its own,** webcpp::webrtc, built on webcpp::asio.
4. The project rules apply from the first commit: exceptions on and off, every variant
   tested, allocators (run-time objects take the user's allocator), the order of what a
   library builds on (Boost.Asio, Beast and Cobalt first), wrapped outside interfaces
   (the browser's APIs behind Boost's interfaces), fixed-width types, and asynchronous
   code on xactor's scheduler where a library has actors.
5. **Name resolution on Emscripten** (the owner, 2026-10-10, option A of the review's C4):
   the resolver completes with entries that carry the host name and the service name and an
   unspecified endpoint; a plain socket's connect on such an endpoint fails with
   `operation_not_supported`, so decision 2's "fails as a value" holds where a TCP
   connection would be needed; only webcpp::beast's browser streams accept such entries, so
   Beast's layered client code runs unchanged (section 7).

## 3. What it is, target by target

webcpp::asio's targets are native and emscripten. It has no WASI target: Boost.Asio 1.92 does
not compile for wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34 (`AGENTS.md:1443-1446`). A WASI
backend (`wasi:io/poll` on p2, p3's async) is a later increment with its own spec; until then a
library on webcpp::asio (webcpp::beast, trystero's migrated backends) declares native and
emscripten at most, and the lanes of section 9 prove those two.

|                                                                                             | Native                                                                                                               | Emscripten                                                                                                                                                                                                                                                       |
| ------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `webcpp::asio::io_context`                                                                  | a class derived from `boost::asio::io_context`; its run family is Boost's                                            | the same class; Boost's scheduler and reactor, the reactor made at construction, the run family composed with the turn limit (section 4)                                                                                                                         |
| executors, `post`, `dispatch`, `bind_executor`, completion tokens, `co_spawn`, Boost.Cobalt | Boost.Asio's                                                                                                         | Boost.Asio's, unchanged                                                                                                                                                                                                                                          |
| `webcpp::asio::steady_timer`, `system_timer`, `basic_waitable_timer`                        | aliases of Boost's                                                                                                   | aliases of Boost's: the reactor's timer queue, which `run` waits on (section 5)                                                                                                                                                                                  |
| promises and events as operations                                                           | none: nothing of the browser exists                                                                                  | section 4.7                                                                                                                                                                                                                                                      |
| WebSocket and HTTP clients                                                                  | Boost.Beast                                                                                                          | webcpp::beast, Beast's interfaces over the browser's `WebSocket` and `fetch` (section 6)                                                                                                                                                                         |
| `ip::tcp`, `ip::udp`: sockets, acceptor, `resolver`                                         | aliases of Boost's                                                                                                   | webcpp's own class templates with Boost's interfaces: each socket and acceptor operation completes with `operation_not_supported` unless a transport provides it, and the resolver completes with the entries webcpp::beast's browser streams accept (section 7) |
| `ssl`: `context`, `stream_base`, `verify_*`, `host_name_verification`                       | aliases of Boost's (`<webcpp/asio/ssl.hpp>`, which brings OpenSSL natively, so the convenience header leaves it out) | a record of the program's TLS settings, which only webcpp::beast's browser streams read (WEBCPP_BEAST_SPEC.md section 4.2)                                                                                                                                       |

A program includes `<webcpp/asio.hpp>` and names `webcpp::asio::` where it would name
`boost::asio::` for the types above; everything else of Boost.Asio it keeps naming
directly. The io_context is a class of its own on both targets, so that a program sets its
options the same way everywhere; its executor is `boost::asio::io_context::executor_type` and
it converts to `boost::asio::io_context&`, so code written against either interoperates. Every
other name is an alias of Boost's natively.

Asio needs `BOOST_ASIO_DISABLE_THREADS` on Emscripten: without it Asio decides it has threads
under any `__GNUC__` that is not Linux (`B/asio/detail/config.hpp:1173-1175`) and
`<boost/asio/io_context.hpp>` stops at `#error Only Windows and POSIX are supported!`
(`B/asio/detail/signal_blocker.hpp:27`, measured by the review). webcpp::asio's usage
requirements carry `<toolset>emscripten:<define>BOOST_ASIO_DISABLE_THREADS`, as trystero's do
today (`libs/trystero/build.jam:87`). Asio's inline namespace differs with and without threads
(`B/asio/detail/config.hpp:1191-1195`), so a translation unit that missed the define fails at
link, never silently.

## 4. The io_context on Emscripten

### 4.1 What Boost.Asio already does there (measured)

Asio on Emscripten uses the `select_reactor` (`B/asio/detail/reactor.hpp:31,50`; Emscripten
defines no `__linux__`, so no epoll), which is also the timer scheduler
(`B/asio/detail/timer_scheduler_fwd.hpp:38`): every `basic_waitable_timer` of the context, the
ones Beast and Cobalt create included, sits in its queues. Its `run` waits in `select` for the
nearest timer (`B/asio/detail/impl/select_reactor.ipp:218-266,355-365`). Emscripten implements
`select` with `poll` (`E/system/lib/libc/musl/src/select/select.c:58-72`) and `poll` as a
suspending import for every timeout, zero included (`E/src/lib/libsyscall.js:630,644`), which
waits on the descriptors' wait queues and a `setTimeout` (`libsyscall.js:683-718`). A post
interrupts the reactor through its pipe (`B/asio/detail/impl/scheduler.ipp:673-686`), and a
pipe write wakes the waiters (`E/src/lib/libpipefs.js:243,275`). Without threads, Asio's post
never takes the per-thread private queue (`scheduler.ipp:357-377`), so a post made from a
browser event while `run` is suspended reaches the shared queue and interrupts the reactor.

So plain `boost::asio::io_context::run()` suspends into the browser's event loop on both
variants: a 200 ms Boost timer fired after 201 ms while a JavaScript `setInterval` ran three
times, two posts from JavaScript `setTimeout`s woke `run`, and `run` returned
(`P/timer.cpp`, JSPI and Asyncify, also under `-sNODERAWFS=1`); Cobalt's own `co_main` slept
150 ms on a timer (`P/cobalt_main.cpp`, JSPI).

What Boost lacks there, and webcpp::asio adds, is four things:

1. `BOOST_ASIO_DISABLE_THREADS` (section 3).
2. **The reactor, made at construction.** With threads disabled, a `run` that has work but no
   reactor (no timer or socket ever made) waits in `null_event::do_wait`, `::pause()`
   (`B/asio/detail/impl/null_event.ipp:41-49`), which nothing wakes: measured, `P/reenter.cpp`
   never returned after its work guard was reset from an event, and the same program with one
   unarmed `steady_timer` (`P/reenter_reactor.cpp`) returned. webcpp::asio's io_context holds
   one unarmed `boost::asio::steady_timer`, through public API, so that its reactor exists from
   construction (the reactor's constructor registers it as the scheduler's task,
   `B/asio/detail/impl/select_reactor.ipp:122-124`, `scheduler.ipp:194-203`).
3. **The turn limit** (section 4.3).
4. **The bridge**, through which a browser event posts to the io_context (section 4.6).

### 4.2 The run family

`run()`, `run_one()`, `run_for()`, `run_until()`, `run_one_for()` and `run_one_until()`
(`B/asio/io_context.hpp:347,361,374,393,407,421`) keep Boost.Asio's meaning: `run()` returns
when the io_context has no outstanding work or is stopped, `run_one()` after one handler,
`run_for(d)` after `d` at most. `poll()` and `poll_one()` are Boost's on every target.

```cpp
namespace webcpp::asio {

struct io_context_options {
    /** At most this many handlers run before the run family yields to the browser. */
    std::size_t max_handlers_per_turn = 1000;  // measured default, section 4.3
};

class io_context : public boost::asio::io_context {
public:
    io_context();
    explicit io_context(io_context_options options);
    template <class Allocator>
    io_context(std::allocator_arg_t, const Allocator& allocator, io_context_options options = {});

    // On Emscripten these hide Boost's and compose them (below); natively they are Boost's.
    count_type run();
    count_type run_one();
    template <class Rep, class Period>
    std::size_t run_for(const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
    std::size_t run_until(const std::chrono::time_point<Clock, Duration>& abs_time);
    template <class Rep, class Period>
    std::size_t run_one_for(const std::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
    std::size_t run_one_until(const std::chrono::time_point<Clock, Duration>& abs_time);

    std::size_t max_handlers_per_turn() const noexcept;
    void max_handlers_per_turn(std::size_t n) noexcept;
};

}  // namespace webcpp::asio
```

Natively the options are accepted and change nothing: no page waits on a native loop. On
Emscripten `run()` is a composition of Boost's own calls:

1. call `poll_one()` until it returns 0 or `max_handlers_per_turn` handlers have run;
2. if the io_context is stopped, return;
3. if the limit was reached, yield to the browser through a macrotask (section 4.3) and go
   to 1;
4. otherwise call Boost's `run_one()`, which suspends in the reactor's `select` until a
   timer, a post or I/O makes a handler ready, and runs it; if it returned 0 (stopped, or out
   of work), return; else go to 1.

`run_one()` is step 4 alone; the timed forms use `run_one_until(deadline)` in step 4 and
return at the deadline. The counts returned are the handlers run, as Boost's. Code that calls
`run` through a `boost::asio::io_context&` gets Boost's own `run`, which suspends correctly
but has no turn limit (section 5.2 is how Cobalt's `co_main` gets it).

### 4.3 Never starving the page

`run()` and `poll()` run until no handler is ready, including those posted meanwhile, and a
zero-timeout `select` suspends only to a microtask (`E/src/lib/libsyscall.js:683-718`: the
promise resolves at once), which never yields to rendering. Measured: a self-posting handler
under Boost's `run` ran 300,000 times before a `setTimeout(0)` ran, on both variants
(`P/starve.cpp`). Boost's `run` cannot be told to yield (`run` is not virtual,
`B/asio/io_context.hpp:347,393`), hence the composition of section 4.2.

The yield is a suspending import, `EM_ASYNC_JS` (an `__asyncjs__` import,
`E/system/include/emscripten/em_js.h:73`, which Asyncify treats as asynchronous by default,
`E/tools/link.py:63`), whose promise a `MessageChannel` message resolves: a macrotask, never a
microtask. Measured with the composition above over Boost's own io_context and a limit of
1,000 (`P/turn_limit.cpp`): the `setTimeout(0)` ran at iteration 1,000 under JSPI and 999 under
Asyncify, against 300,000 without the limit, and `run` still ran all 300,001 handlers.
`MessageChannel` exists in node 26 and in every browser. The default of 1,000 is the
measured one of section 11; the core increment measures it again in the browser lane
against a frame's budget and records the result.

### 4.4 The two suspension variants

| Variant | What suspends | Build | Exceptions | Browsers |
| --- | --- | --- | --- | --- |
| `jspi` | JavaScript Promise Integration: the suspending imports (`__syscall_poll`, the yield) are wrapped in `WebAssembly.Suspending`, and `main`, on whose stack `run` executes, in `WebAssembly.promising` (`E/tools/link.py:65-68`) | `-sJSPI` | WebAssembly exceptions, `-fwasm-exceptions` | those with JSPI |
| `asyncify` | Emscripten's Asyncify: the compiler instruments every function that may reach a suspending import | `-sASYNCIFY` | JavaScript-based exceptions, `-fexceptions` | every browser |

**Exceptions, per variant.** Asyncify does not work with WebAssembly exceptions: emcc only
warns that "ASYNCIFY=1 is not compatible with -fwasm-exceptions" (`E/emcc.py:486`), and the
program traps at run time with `RuntimeError: unreachable` (measured, `P/timer_catch.cpp`,
`run` called inside a `catch` block). b2's emscripten toolset compiles `<exception-handling>on`
with `-fwasm-exceptions` unless `<exception-handling-method>js`
(`B2/tools/emscripten.jam:161-164`), so the feature is composite:

```jam
feature.feature webcpp-suspend : jspi asyncify : propagated composite ;
feature.compose <webcpp-suspend>asyncify : <exception-handling-method>js ;
```

Measured with b2 5.5.3 (`P/b2probe-rev/Jamroot`, a dry run): `webcpp-suspend=asyncify
exception-handling=on` compiles and links with `-fexceptions`, `webcpp-suspend=jspi` with
`-fwasm-exceptions`. A usage requirement cannot do it: b2 takes only free features from usage
requirements (`AGENTS.md`, chapter 6, "A library works without exceptions"). A test throws and
catches across a suspension on each variant: inside a `try` that encloses `run`, inside a
`catch` block that calls `run`, and a handler's throw caught in `main` after `run` suspended
(measured: `P/timer_try.cpp`, `P/timer_catch.cpp`, `P/turn_limit.cpp`, all three on both
variants). Builds with `exception-handling=off` compile with `-fno-exceptions` on both.

**Asyncify, exactly.** The suspending imports are found by Emscripten itself:
`__syscall_poll` is marked asynchronous in its library (`E/src/lib/libsyscall.js:630`,
collected at `E/tools/link.py:3186-3187`) and the yield is an `__asyncjs__` import, so there is
no list of imports to give. The path from `main` to them passes the virtual
`scheduler_task::run` (`B/asio/detail/scheduler_task.hpp:34`), so `ASYNCIFY_IGNORE_INDIRECT`
(`E/src/settings.js:827`) would be unsound and is not set; JavaScript-based exceptions then add
`invoke_*` to the instrumented imports (`E/tools/link.py:1709-1714`). The flags are
`-sASYNCIFY` and `-sASYNCIFY_STACK_SIZE=<n>`, `n` measured by the core increment against the
deepest suspension its tests reach (the default is 4,096 bytes, `E/src/settings.js:834`). The
cost is size: 283,409 bytes against 97,429 for the same program (`P/timer.cpp`, 2.9 times),
297,636 against 102,988 for `P/turn_limit.cpp`. The size table of the CI records both variants
of every example.

**Build output and names** follow Boost's own build exactly (owner, 2026-10-10): b2 separates
builds by directory, one level per toolset, variant and non-default property, and an
artifact's file name carries its tags joined by `-` through b2's `common.format-name`
(`<base> <toolset> <threading> <runtime> <property:webcpp-suspend>`). Measured with b2 5.5.3
(`P/b2probe-rev/Jamroot`): `bin/emscripten-6.0.11/debug/link-static/target-os-none/chat-60-d-jspi.js`,
`.../webcpp-suspend-asyncify/chat-60-d-asyncify.js`, and `chat-60-jspi.js` in release. The
toolset tag is the version alone, `60`, since `common.jam` has no emscripten case
(`B2/tools/common.jam:908-1005`), and `d` is the debug variant (`common.jam:1043`); the spec
takes b2's result as it is. The default variant, `jspi`, adds no directory level.

**The loader.** webcpp::asio ships `webcpp-asio-loader.mjs`, which a page imports: it detects
JSPI (`typeof WebAssembly.Suspending === "function"`, true in node 26.7.0, review) and loads
the matching artifact, whose names it reads from a manifest the build writes beside them, not
from a pattern. The glue is an ES module: programs link with `-sMODULARIZE -sEXPORT_ES6`
(both off by default, `E/src/settings.js:1310,1340`).

### 4.5 Who may call the run family, and what an event may call

- **The run family is called on `main`'s stack**, or on the stack of another export the
  program lists for suspension: in `-sJSPI_EXPORTS` under JSPI (only `main` and
  `__main_argc_argv` by default, `E/tools/link.py:65-68`, `E/src/settings.js:929-936`), whose
  call then returns a Promise, and called with `ccall(..., {async: true})` under Asyncify
  (`E/src/lib/libccall.js:93`). Anywhere else the first suspension fails: measured,
  `io_context::poll()` called from a JavaScript event while a timer was pending threw
  `SuspendError: trying to suspend without WebAssembly.promising` under JSPI, and under
  Asyncify returned 0 while starting an unwind that overwrote `Asyncify.currData`
  (`E/src/lib/libasync.js:416`), which corrupts a suspended `main` (`P/starve.cpp`). Under
  Asyncify a wake must also come from JavaScript with no compiled code on the stack
  (`E/src/lib/libasync.js:365`), which the bridge's post-only rule gives.
- **Nothing calls `poll`, `run` or `dispatch` from a JavaScript event.** While `run` is
  suspended, an event sees `running_in_this_thread() == true`, because the frame `run` pushed
  stays on the call stack (`B/asio/detail/impl/scheduler.ipp:346-349`), and
  `boost::asio::dispatch` runs its handler inline inside the event (measured,
  `P/reenter.cpp`). The bridge (section 4.6) only posts.
- **What webcpp::asio detects.** Its run family keeps a flag while a call is active, and the
  bridge one while it delivers an event. A run family call while either is set (a nested
  `run` from a handler, a call from an event delivered through the bridge) is a precondition
  violation: `BOOST_ASSERT`, and with assertions off it returns 0 and runs nothing. A test on
  both variants checks it through `BOOST_ENABLE_ASSERT_HANDLER`. A call from an export of the
  program's own that is not listed for suspension cannot be detected before it suspends; the
  documentation names it.
- **No busy wait.** A program does not block the browser's thread in a way that does not
  suspend: a busy loop, a blocking system call.
- **The lint's `io_context::run` rule is unchanged** (`AGENTS.md:784`): a library never calls
  the run family, it is given an executor; a program's `run` says why after `lint-run:`
  (`io.run();  // lint-run: the program's event loop, suspended into the browser's`).
- **xactor's `asio_driver`** drains an `io_context{1}` of its own with `poll`
  (`libs/xactor/include/webcpp/xactor/drivers.hpp:157-228`). That context has no timer and no
  socket, so no reactor: its `poll` never reaches `select` and never suspends
  (`scheduler.ipp:194-203`: the reactor is the scheduler's task only once made). Calling it
  from a handler of the program's io_context is safe on both variants.
- **trystero's browser backend** polls from inside its events today
  (`libs/trystero/include/webcpp/trystero/browser/event_loop.hpp:327-345`); on webcpp::asio
  that is removed, and its programs call `run` (section 10).

### 4.6 How the browser wakes it: the bridge

Every event source webcpp::asio and the libraries on it install (a WebSocket's `onmessage`,
a `fetch` promise, a WebRTC callback in webcpp::webrtc, the operations of section 4.7) is a
JavaScript function made by the io_context's bridge, an `execution_context` service of the
io_context (Boost's public service mechanism, so it is made with the context's allocator and
destroyed with it):

- registered by id, never reused, so that a late event of a destroyed object finds nothing;
- **it only posts**: delivering an event looks the id up and posts its handler to the
  io_context, bound to the registration's associated allocator; it never polls, runs or
  dispatches. The post interrupts the suspended `run` through the reactor's pipe
  (section 4.1);
- an exception that escapes the delivery is caught at the JavaScript boundary, which
  restores WebAssembly's stack pointer (`stackSave` and `stackRestore`, as trystero's
  `webcpp_trystero_js_handler` does, `libs/trystero/include/webcpp/trystero/browser/src.hpp:48-65`)
  and reports it with `reportError`.

Timers need no bridge: they are the reactor's (section 5).

### 4.7 Promises and events as operations

```cpp
namespace webcpp::asio::browser {

/** Completes with the promise's value, or with error::promise_rejected and the reason. */
template <class CompletionToken>
auto async_await(const boost::asio::any_io_executor& executor, emscripten::val promise,
                 CompletionToken&& token);
// completion signature: void(boost::system::error_code, emscripten::val)

/** Completes with the next event of `type` on `target`, a DOM EventTarget. */
template <class CompletionToken>
auto async_wait_event(const boost::asio::any_io_executor& executor, emscripten::val target,
                      std::string_view type, CompletionToken&& token);
// completion signature: void(boost::system::error_code, emscripten::val)

}  // namespace webcpp::asio::browser
```

- Both go through `boost::asio::async_initiate`, so every completion token works
  (`use_awaitable`, Cobalt's `use_op`, deferred), and both deliver through the bridge: the
  settlement or the event posts the handler.
- **Errors:** a rejection completes with `webcpp::asio::browser::error::promise_rejected`
  (category `webcpp.asio`, value 1, never reused) and the rejection's reason as the value.
- **Cancellation:** a promise cannot be cancelled. A `terminal`, `partial` or `total`
  cancellation completes the operation at once with `operation_aborted` and drops the
  registration, so a later settlement finds nothing; for an event, the listener is removed.
- **Lifetime:** a pending operation is outstanding work, as natively: a promise that never
  settles keeps `run` from returning until it is cancelled. Destroying the io_context
  destroys the pending handler without calling it, as Boost's shutdown does.
- **Allocation:** the registration and the operation's state use the handler's associated
  allocator.
- They need embind (`emscripten::val`); webcpp::asio's usage requirements link it on
  Emscripten (`<toolset>emscripten:<linkflags>-lembind`, as trystero's do,
  `libs/trystero/build.jam:88`).

### 4.8 Live objects, and after `main`

Without `run`, nothing runs, as natively (owner, 2026-10-10): work posted before `run` is
called waits for it, and an event that arrives while no `run` is active is posted and waits
for the next one. As natively, `run` returns when no operation is pending, even with a
connection open: a WebSocket with no pending read does not keep `run` alive, and its messages
queue, up to the bound of WEBCPP_BEAST_SPEC.md section 4.3. After `main` returns, the runtime
stays alive (`EXIT_RUNTIME` is off by default, `E/src/settings.js:106`), events keep being
posted, and nothing runs them unless an export listed for suspension (section 4.5) calls the
run family again.

## 5. Timers and Boost.Cobalt

### 5.1 Timers

Boost's timers are the timers, on both targets: `webcpp::asio::basic_waitable_timer`,
`steady_timer` and `system_timer` are aliases of Boost's, so their interface, any completion
token, the per-operation cancellation slot and the allocator associations are Boost's
unchanged. On Emscripten they sit in the reactor's timer queue, which `run` waits on
(section 4.1); the reactor caps a wait at five minutes
(`B/asio/detail/impl/select_reactor.ipp:358-360`), so an idle program holds one `setTimeout`
of at most 300 s.

### 5.2 Boost.Cobalt's `co_main`

Cobalt's `co_main` cannot be replaced from a header. Cobalt is a compiled library
(`BS/libs/cobalt/src/`); `co_main`'s promise is fixed by
`coroutine_traits<boost::cobalt::main, int, Char>` (`B/cobalt/detail/main.hpp:161-164`);
`main` is a hidden friend defined in Cobalt's header (`B/cobalt/detail/main.hpp:106-124`), which
calls `main_promise::run_main`, compiled in `BS/libs/cobalt/src/main.cpp:26-63`, which owns a
`boost::asio::io_context` and calls Boost's `ctx.run()` (lines 28, 61). Cobalt's own `co_main`
runs unchanged on Emscripten, without the turn limit (measured, `P/cobalt_main.cpp`, JSPI).

webcpp therefore builds Cobalt's sources, as `tools/boost_json.cpp` builds Boost.JSON's
(`Jamroot:288-293`), and supplies its own definition of `main_promise::run_main`, in a
translation unit of webcpp::asio's, in place of Cobalt's `src/main.cpp`: the same steps as
Cobalt's (the executor, the signal set, the post that resumes the coroutine), with a
`webcpp::asio::io_context` and its run family in place of `ctx.run()`. A Cobalt program then
changes nothing, not even an include, and gets the turn limit on Emscripten; natively the
definition behaves as Cobalt's. Measured (`P/cobalt_run_main.cpp` with `P/cobalt_spin_main.cpp`,
JSPI): Cobalt's sources but `main.cpp`, built with em++ beside a definition of `run_main`
that composes `poll_one` and `run_one` with a macrotask yield every 1,000 handlers, ran an
unchanged `co_main` of 5,000 posts and a 50 ms timer, yielded 5 times, and returned its
`co_return 7` as the process's exit status. Why this and not a `co_main` of webcpp's own: a function with
Cobalt's signature cannot coexist with Cobalt's header, and one with another return type
changes every Cobalt program.

- The definition reads `main_promise`'s members (`result`, `exec`, `exec_`, `signal_set`,
  the cancellation `signal`, `B/cobalt/detail/main.hpp:36,145-148`), which are Boost's
  implementation, so a probe compiles it against the pinned Boost (1.92) and the CI fails
  when Boost changes them.
- Cobalt's sources are not in the headers-only install (`tools/ci/actions/boost/install.sh`:
  "Only headers are installed"); the Boost action keeps `libs/cobalt/src` of the archive it
  already downloads and checks (`install.sh:25-29`), and the Jamroot builds
  `/webcpp//boost_cobalt` from it, every source but `main.cpp`, on native and emscripten.
- Cobalt's main waits on a `basic_signal_set` (`BS/libs/cobalt/src/main.cpp:35-53`), which
  makes the reactor, so a Cobalt program never meets the `::pause()` of section 4.1.
- If the definition cannot be kept, Cobalt's own `co_main` is the fallback: it works
  unchanged, without the turn limit.

## 6. WebSocket and HTTP: webcpp::beast

WebSocket and HTTP are webcpp::beast's (owner, 2026-10-10), a library of its own on
webcpp::asio, as Boost.Beast is on Boost.Asio: Boost.Beast's own names and interfaces on
both targets, Beast itself natively, and in the browser the same layered code over the
browser's `WebSocket` and `fetch`. Its specification is WEBCPP_BEAST_SPEC.md. webcpp::asio
gives it the io_context, the bridge (section 4.6), Boost's timers, the `ssl` record
(section 3) and the transport point (section 7).

## 7. Sockets and resolution: the transport point

Boost's own socket types compile on Emscripten, but they do not fail as decision 2 asks:
Boost's `tcp::socket` connects through Emscripten's SOCKFS, which opens a WebSocket to the
address (`E/src/lib/libsockfs.js:262-288`), and `getaddrinfo` invents addresses in 172.29.x.x
(`E/src/lib/libcore.js:952-985`). So on Emscripten `webcpp::asio::ip::tcp` and `ip::udp` are
protocol classes of webcpp's own, with Boost's members (`v4()`, `v6()`, `endpoint` as
`boost::asio::ip::basic_endpoint<tcp>`), whose `socket`, `acceptor` and `resolver` are class
templates of webcpp's own. The sockets and the acceptor are over a policy,
`webcpp::asio::browser::transport<Protocol>`, whose primary template completes every operation
with `boost::asio::error::operation_not_supported`; the resolver is below.

The subset reproduced, with Boost's signatures:

| Type | Members |
| --- | --- |
| `basic_stream_socket<Protocol, Executor>` | constructors (executor or context, with a protocol or an endpoint), `get_executor`, `open`, `is_open`, `close`, `cancel`, `shutdown`, `connect` and `async_connect`, `read_some` and `async_read_some`, `write_some` and `async_write_some`, `send`, `receive` and their `async_` forms, `set_option`, `get_option`, `local_endpoint`, `remote_endpoint`, `available`, `non_blocking` |
| `basic_datagram_socket<Protocol, Executor>` | the same, with `bind`, `send_to`, `receive_from` and their `async_` forms in place of the stream operations |
| `basic_socket_acceptor<Protocol, Executor>` | `open`, `bind`, `listen`, `accept` and `async_accept`, `close` |
| `basic_resolver<Protocol, Executor>` | `resolve` and `async_resolve` (host and service, with flags; an endpoint), `cancel` |

The synchronous overloads fail too and never block: the `error_code&` form sets the error,
the throwing form raises it through `boost::throw_exception` and exists only with exceptions.
`open` and `close` succeed (they hold no resource), so that a program's error paths run as
natively.

**The resolver** (decision 5) resolves nothing: `resolve` and `async_resolve(host, service)`
complete with one entry whose `host_name()` and `service_name()` are the arguments and whose
endpoint has an unspecified address and the service's port (WEBCPP_BEAST_SPEC.md section 4.1
gives the port). A program writes `using tcp = net::ip::tcp;` and `tcp::resolver`
(`P/websocket_client_async_ssl.cpp:87`), so this is the resolver Beast's client meets. A plain
socket's `connect` on that endpoint, as on any endpoint, fails with `operation_not_supported`:
decision 2's "fails as a value" holds where a TCP connection would be needed, and only
webcpp::beast's browser streams accept the entry. A resolver that failed like the sockets,
with a resolver of webcpp::beast's own beside it, was the alternative; it was rejected because
every Beast client would change a line. Reverse resolution (`async_resolve(endpoint)`) fails
with `operation_not_supported`.

**A transport.** A specialisation of `transport<Protocol>` (a later library: relays that carry
TCP or UDP over a WebSocket or a WebRTC channel) provides the operations, and the program's
code does not change. A specialisation must be visible in every translation unit that uses
the socket type, before its first use, or the program violates the one definition rule; so a
relay library is enabled for a whole program through webcpp::asio's configuration, a b2
feature in its usage requirements that `<webcpp/asio/config.hpp>` reads, designed with that
library. Beast's `websocket::stream` over one of these sockets needs `teardown` and
`async_teardown` for it (`B/beast/websocket/teardown.hpp:23-30`: a user-defined socket brings
its own); webcpp::beast declares them (WEBCPP_BEAST_SPEC.md section 3).

## 8. Allocation, exceptions, determinism

- **Allocation.** The io_context takes the user's allocator: `io_context(std::allocator_arg,
  a, options)` passes it to Boost's constructor (`B/asio/io_context.hpp:233,256,282`), which
  allocates the context's services and its I/O objects' state with it. Operations allocate
  through their handler's associated allocator, as Boost's do. The counting test counts every
  `operator new` and every allocation of the given resource, natively and on both Emscripten
  variants, and proves that none goes elsewhere, except what the runtime allocates with its
  own `malloc`, listed by name: Emscripten's `select` callocs its poll array on every call
  (`E/system/lib/libc/musl/src/select/select.c:71`), Asyncify allocates its unwind data on
  every suspension (`E/src/lib/libasync.js:415-416`, "TODO: reuse"), and Asio's
  `basic_resolver_results` allocates its entries with `new`
  (`B/asio/ip/basic_resolver_results.hpp:159-166`).
- **Exceptions on and off:** built and tested both ways, on both variants with the exception
  model of section 4.4. A handler's exception propagates out of `run`, natively and in the
  browser, as Boost's does: handlers run on `main`'s stack, never inside an event. The
  bridge's boundary catches only what escapes a delivery (section 4.6).
- The library reaches the world (clocks, sockets, the browser) because that is its
  purpose; each include says so after `lint-world:`.

## 9. Testing

- **Native lanes:** `webcpp::asio::io_context` derives from Boost's and shares its executor
  type, every other name is Boost's (static assertions), and the guide's programs run.
- **Emscripten, both variants, under node:** the run family's semantics (returns when out of
  work, `run_one`, `run_for`, `run_until`, `run_one_for`, `run_one_until`, `stop`,
  `restart`); Boost timers, ordered and cancelled, Beast's and Cobalt's among them; posts from
  JavaScript events waking `run`; work posted and events arrived before `run` waiting for it
  (nothing runs without it); `run` returning with a connection open and no operation pending;
  the turn limit (a self-posting handler lets a `setTimeout(0)` run within
  `max_handlers_per_turn` handlers); the nested and in-event calls of section 4.5 caught by the
  assertion handler; a throw and a catch across a suspension (section 4.4); a promise resolved,
  rejected, cancelled and never settled; an event awaited and cancelled; allocation counts
  (section 8).
- **A browser lane** (headless Chrome, as trystero's interop lane): no polling (an idle
  program schedules no task and no timer beyond the reactor's one `setTimeout` of at most
  300 s, counted through a `setTimeout` wrapper installed before the module), rendering
  continues while a self-posting handler runs, and the loader picking the variant. The
  WebSocket and `fetch` lanes are webcpp::beast's.
- **Size:** the size table records each example on both variants (section 4.4).
- **Twins:** each example of the guide prints the same natively and in the browser, a
  Cobalt `co_main` program among them.

## 10. Increments

1. The core: the io_context (`BOOST_ASIO_DISABLE_THREADS`, the reactor at construction, the
   run family and its turn limit), the bridge, the detection of section 4.5, the build
   variants with their exception models, their artifact names and the loader, Boost's timers
   proven on both variants.
2. Boost.Cobalt: its build, the `run_main` of section 5.2 and its probe, the `co_main` twin.
3. Promises and events as operations (section 4.7).
4. The transport point, its error primary, and the resolver (section 7).
5. Then, each with its own spec: webcpp::beast (WebSocket, then HTTP), webcpp::webrtc and
   webcpp::crypto, and trystero on webcpp::asio, webcpp::beast, webcpp::webrtc and
   webcpp::crypto alone, its event-side polling removed.

## 11. Settled by the owner (2026-10-10)

- The default variant is `jspi`; the Asyncify lanes run at the nightly and release levels
  of the CI, not on every push.
- `max_handlers_per_turn` has a measured default, of the order of 1,000, and a program may
  change it (an option of the io_context).
- Without `run`, nothing runs, on every target: use without `run` is not supported.
- Name resolution on Emscripten is option A of the review's C4 (decision 5, section 7;
  `WEBCPP_DESIGN.md:855`).
