## Context

webcpp::pratt was designed and built on 2026-10-08, from an owner's draft prompt that carried a
verified reference implementation of the engine, a calculator, a test grammar and a REPL
(`reference/draft.md`, Appendices A to D), and a design that states what the library keeps from
the draft, what it generalizes and every deviation, with its reason (`reference/design-source.md`;
where the two differ, the design wins). It was built task by task in a worktree of the
superproject, `libs/pratt` a repository of its own, 42 commits from 2026-10-08 to 2026-10-12, all
as `R`. Emscripten joined as a target on 2026-10-10 once the superproject had its lane, and the
exception configuration (`WEBCPP_PRATT_NO_EXCEPTIONS`) on 2026-10-08 and 2026-10-11.

This document records, retroactively, the decisions 0.1.0 is built on.

## Goals / Non-Goals

**Goals:**

- A reusable Pratt engine that is impossible to use without going through nud, led and lbp: it
  knows no token, no operator, no tree and no number type; a program supplies the token stream,
  the node and the grammar, checked as concepts at compile time, with no base class, no virtual
  function and no type erasure.
- Every failure a value (`boost::system::result`), no exception of its own, no allocation in the
  engine, no global or static mutable state, determinism: the same input and grammar give the
  same result on every target.
- A bounded recursion: a nesting of ten thousand `(` is `nesting_too_deep`, not a crash, with a
  default limit measured on wasm's default 64 KiB stack in a debug build.
- A batteries-included calculator whose numbers are exact in decimal by default, whose lexer is
  lazy, and whose variables live in an environment the program chooses, allocator included.
- Native, Emscripten, wasip2 and wasip3, each proven by a lane; documentation whose every
  program was compiled and run.

**Non-Goals:**

- A parser generator, a grammar DSL, a lexer toolkit or an AST: the engine builds whatever node
  the grammar says and does nothing else with it.
- Anything that recurses outside `expression()` being bounded: a nud or a led that recurses
  through functions of its own must bound that itself.
- A correctly rounded `pow` for integral exponents: the calculator trades it for the same bits on
  every platform (D8).
- The parser of openui-lang itself: the micro-subset in `example/` is its seed and its proof that
  the engine parses calls, named arguments and assignments with no recursive descent around it.
- Undoing the writes of a failed evaluation (a caller that needs all or nothing evaluates in a
  copy of the environment).

## Decisions

### D1. A library of webcpp's own, two layers kept apart

pratt ports nothing (`"port-of": null`): no oracle, no twins, no "Differences" appendix; its page
and the Doc Comments of its headers are what a change is checked against. The engine
(`pratt.hpp`, `errors.hpp`, `parser.hpp`) and the calculator (`calculator.hpp`, namespace `calc`)
live in one library, but `meta/include-boundaries.json` forbids the engine's three headers to
include `webcpp/pratt/calculator` or `boost/decimal` ("the engine knows no grammar and no number
type"), and the lint enforces it. `<webcpp/pratt.hpp>` includes the engine only, so a program of
the engine alone never reads Boost.Decimal; the calculator is included by name.

### D2. Concepts, not inheritance

A token stream models `token_stream` (`token_type`, `peek() const`, `advance()`, `at_end() const`)
and a grammar models `grammar_for<G, S>` (`node_type`, `lbp(t)`, `nud(p, t)`, `led(p, t, left)`).
The parser holds the stream by value and a pointer to the grammar. There is no `std::function`
and no virtual call between the engine and a grammar, and a grammar consulting runtime tables
stays possible. Alternative rejected: a base class of tokens and nodes, which would make every
program pay for a hierarchy and fix the node type.

### D3. Errors as values, in the form xactor uses

Every operation that can fail returns `pratt::result<T>` (`boost::system::result<T>`); the
engine's own errors are the enumerators of `pratt::error`, values 1 to 9, fixed and never reused
(a stored value keeps its meaning across versions; 0 is no error). The category follows xactor's
form, not the draft's function-local static: a `constexpr` object with its own 64-bit identity
(the ASCII of "pratt" and three zero bytes, `0x7072617474000000`), the name `webcpp.pratt`, an
accessor `error_category()`, `make_error_code(error)` and the `is_error_code_enum`
specialization. Both `message(int)` and the buffer form `message(int, char*, size_t)` are
provided. The two errors added to the draft's seven are `nesting_too_deep` (8, decision D4) and
`division_by_zero` (9, D8). An error the engine builds into a `result<node>` is built in place
(`boost::system::in_place_error`), because a node type may convert from an error too, as a
Boost.Multiprecision number does, and `result` then refuses the implicit conversion as ambiguous.

### D4. The depth limit counts `expression()` calls, and the default is measured

Pratt recursion costs stack per nesting level, so an input of ten thousand `(` would overflow a
stack: a crash, not an error. `parser` counts its active `expression` calls with an RAII guard
and refuses the call that would exceed `max_depth` with `nesting_too_deep`, before it reads a
token; a negative limit behaves as 0, which refuses every parse. The limit is a constructor
argument, forwarded by `parse` and `calc::evaluate`. Every helper recurses through
`expression` alone, so a grammar written with them is bounded whole; a grammar that recurses
otherwise bounds itself (stated in the documentation). `default_max_depth` is not chosen: it is
the largest power of two at most half of the deepest parse that wasm's default 64 KiB stack
holds in a debug build (`-O0`) in the costliest shape of the calculator's grammar and of
openui-lang's (203 calls, openui-lang's call shape on wasip2, half is 101, hence 64). The
measurement, with wasi-sdk 34 and wasmtime 47.0.3, is a table above the constant in
`parser.hpp` and in the "Limits" chapter; `depth_test` is its canary (twice the default must
still fit), which fails on wasm, as a trap, the day the margin is gone. A program that raises the
limit raises its stack with it (`-Wl,-z,stack-size=` on wasm).

### D5. Helpers write the usual operator kinds, through `expression()` alone

`literal`, `prefix`, `infix_left`, `infix_right` (the right operand parsed with `bp - 1`, the
canonical Pratt trick), `postfix` and `grouping` each take the parser and a binding power, parse
their operands through `expression`, and call a `make` of the grammar's that builds the node.
They carry `NOLINT(misc-no-recursion)` on their own lines only, as `expression` does. Parser
members `peek()` and `advance()` serve a nud or a led that reads past its own token (the argument
loop of a call).

### D6. A lazy lexer, and an invalid character is a token of the highest power

The calculator's `stream` and the micro-subset's hold the source, a position and one token of
lookahead, and lex the next token only when `advance` consumes the current one; no vector of
tokens exists. A character that starts no token is a token of kind `invalid`, one character
long, whose lbp is `std::numeric_limits<int>::max()` and whose nud and led return
`invalid_character`, so the engine reaches it wherever it stands: after a complete expression
`1 $ 2` is `invalid_character`, not `trailing_input`, and the engine stays the only place that
decides. The first error in reading order is the one returned, and a write made before the error
stays made. Alternative rejected: lexing everything first, which would find the invalid character
before any other error and allocate a token vector.

### D7. The calculator computes in a type of the program's choice, decimal by default

`calc::number<N>` is a documented concept: copyable, constructible from `int`, `+ - * /`, unary
`-`, the six comparisons, and `pow`, `fmod` and `trunc` found by ADL after `using std::pow;` and
so on (the two-step call), each returning exactly `N`, so that `boost::decimal`'s overloads
serve its types and `std::`'s serve the builtin floating-point ones. `int` does not model it
(std's functions return `double`), nor does a type with expression templates; Boost.Multiprecision's
`cpp_dec_float<50>` with `et_off` does. The default number type is
`boost::decimal::decimal64_t`, for exactness: `0.1 + 0.2 == 0.3` holds. Every `calc` template is
constrained by `number`.

### D8. Exact numbers and powers, division by zero an error

The lexer builds a literal in `Number`'s own arithmetic, never through `double`: the first
`digits10 + 1` significant digits (`max_digits10` where more, 17 for `decimal64_t` and `double`)
make one integer mantissa, divided once by ten to the number of fraction digits; later integer
digits scale up by ten, later fraction digits are dropped. In a binary type a scaled-up
mantissa is computed to twice the precision (Dekker's split and product) so that the largest
`double` written out in full is finite. Every operation is a statement of its own, so that no
compiler contracts a multiplication and an addition into a fused multiply-add, which would round
differently on each target. An integral exponent is computed by repeated squaring in `Number`
(exact while the result fits the precision, the same bits on every platform); only a non-integral
exponent, or a negative one whose positive power is out of range, calls `pow`. The price,
documented: past the precision the relative error grows with the exponent where a correctly
rounded `pow` would not. `/` and `%` by zero, and `0 ^ -n`, are `division_by_zero`, where a
floating-point type would answer an infinity or a NaN that the next line computes on.

### D9. Environments are a concept, with a ready-made allocator-aware one

`environment_for<E>` asks for `number_type` (a `number`), `lookup(string_view) const` returning
`const number_type*` (`nullptr` when unset) and `assign(string_view, number_type)` returning
`result<void>`, so that an environment can refuse (a fixed storage that is full) with an error of
its own category, which the calculator returns unchanged. `calc::environment<Number, Allocator>`
is a `std::map` with transparent `std::less<>` whose nodes and keys allocate through `Allocator`
rebound; the key is a class of its own, not a bare `std::basic_string`, because libstdc++'s
`std::pair` asks whether its members are implicitly default-constructible and clang then
instantiates a default argument that is a hard error for an allocator without a default
constructor (commit `8987b62`).

### D10. Allocation: the engine none, the calculator's parse none, the environment the user's

The engine holds a pointer, a stream by value and a counter. The calculator's lexer, tokens and
nodes are views and numbers. Only an environment that stores names allocates, through the
user's allocator. Proof: a test program replaces the global `operator new` for itself alone
(over-aligned forms included) and counts zero calls across a session over a fixed-storage
environment, and over `calc::environment` only the allocator's own, which gives everything back.
A test or example never states how many allocations a standard library makes (MSVC's map
allocates when built); it compares what a step adds.

### D11. No exceptions in a header, a configuration macro for symmetry

No header contains `throw`, `try` or `catch`; the lint compiles every public header, test and
example without exceptions. Every target is built with exceptions, as WebAssembly's exception
handling on WASI. `config.hpp` includes `<webcpp/config.hpp>` first and defines
`WEBCPP_PRATT_NO_EXCEPTIONS` when webcpp has no exceptions or when a program defines it for pratt
alone; pratt raises none of its own, so the macro changes nothing in it and exists so that a
program can set it for every webcpp library alike. RTTI is never restricted.

### D12. Proof grammars, REPLs and the standard-input extension

Three grammars prove the engine generic: the test `table` grammar (binding powers from a table,
token `char`, node `std::string`), `sexpr` (the draft's Appendix C), and the openui-lang
micro-subset of the examples, whose `(` holds both of Pratt's denotations (a grouping as nud, a
call as led) and whose argument loop lives in the led over `expression(0)`, `peek` and `advance`.
Two REPLs (openui-lang's and the calculator's) share one line loop. To run them as examples, the
superproject's `webcpp.example` gained an `.input` file beside the source that is fed to the
program's standard input on every target (without one, standard input is empty), and pratt's
`.gitattributes` keeps `*.input` byte for byte because an input holds `\r\n` on purpose. The
language is named openui-lang, alone, in every file and commit.

### D13. The page teaches, the examples are tests

`doc/pratt.adoc` teaches concept by concept; every listing is a compiled example, every output an
`.expected`, every count an attribute, and every `@see "<title>"` of a Doc Comment names a
top-level chapter ("Introduction", "Writing a grammar", "The calculator", "Errors", "Limits",
"Allocation", "Testing"), checked by doc-check. The reference is built strict by MrDocs from the
Doc Comments.

## Risks / Trade-offs

- [The default depth no longer fits the stack when a frame grows] -> `depth_test`'s canary fails
  on wasm; the table, the constant and the "Limits" chapter move together (`AGENTS.md`).
- [A compiler fuses a multiply and an add in the calculator] -> every operation is its own
  statement; a build with `-ffp-contract=fast` or `-ffast-math` may still change a `double`
  result in its last place, which the page says.
- [Repeated squaring loses accuracy at huge exponents] -> documented as the price of identical
  bits on every platform; `pow` computes the cases where the squared power is out of range.
- [Boost.Decimal's `fmod` is inexact past the precision] -> `is_odd` treats an integer whose
  successor rounds back to it as even.
- [A grammar recurses outside `expression()`] -> not bounded by the limit; stated in the
  documentation and in `AGENTS.md`.
- [A failed evaluation leaves its earlier writes] -> stated; evaluate in a copy for all or
  nothing.
- [MSVC's map allocates when constructed] -> tests compare increments, never totals.

## Migration Plan

None: a new library.

## Open Questions

None for 0.1.0. The parser of openui-lang, which the micro-subset seeds, is not part of this
proposal.
