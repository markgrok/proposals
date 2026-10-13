## Why

A program that reads a small language - a calculator line, a configuration expression, a view
description such as `root = Stack(gap: 4)` - needs a parser, and a hand-written
recursive-descent parser spreads precedence and associativity over one function per level. Pratt
parsing (top-down operator precedence) turns both into numbers and a new operator into a new
answer to one question, but C++ had no small, generic, allocation-free engine of it that
reports every failure as a value, bounds its recursion so that a deeply nested input is an
error rather than a stack overflow on a 64 KiB WebAssembly stack, and runs natively, on WASI
(wasip2, wasip3) and on Emscripten. webcpp needs one for the parser of openui-lang, a
view-description language whose parser it plans to build, and as the model of a library whose
engine and whose user-facing component are kept apart.

## What Changes

- **New library `webcpp::pratt`**, webcpp's own (not a port), header-only C++20, in two layers
  kept apart by an include boundary:
  - **The engine** (`<webcpp/pratt.hpp>`, `errors.hpp`, `parser.hpp`): the concepts
    `token_stream` and `grammar_for`, the class `parser<Grammar, Stream>` whose
    `expression(rbp)` knows no operator and only calls the grammar's `nud`, `led` and `lbp`,
    `parse`, the six category helpers (`literal`, `prefix`, `infix_left`, `infix_right`,
    `postfix`, `grouping`), `default_max_depth` (64, measured) and the errors `pratt::error`
    1 to 9 in the category `webcpp.pratt`. It allocates nothing, throws nothing, and bounds its
    recursion with a depth limit.
  - **The calculator** (`<webcpp/pratt/calculator.hpp>`, namespace `webcpp::pratt::calc`): a
    grammar written with the helpers alone, a lazy lexer `stream<Number>`, the concepts `number`
    and `environment_for`, the allocator-aware `environment<Number, Allocator>` and
    `evaluate`; exact in `boost::decimal::decimal64_t` by default, with integral powers by
    repeated squaring and division by zero an error.
- **Configuration**: `<webcpp/pratt/config.hpp>` defines `WEBCPP_PRATT_NO_EXCEPTIONS`.
- **Proof grammars and programs**: a second grammar for the tests (S-expressions over
  characters), a micro-subset of openui-lang (`example/openui/grammar.hpp`) with two REPLs, the
  calculator's REPL and the guide's eight programs, each with the output it prints.
- **Documentation**: a page (`doc/pratt.adoc`, eight chapters and a MrDocs reference), the
  README, `AGENTS.md` and a changelog appendix.
- **Targets**: native, Emscripten, wasip2 and wasip3, each proven by a lane.

## Capabilities

### New Capabilities

- `pratt`: webcpp::pratt 0.1.0, a generic Pratt parser engine for C++20 with a decimal
  calculator built on it.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`).
- **Implementation:** https://github.com/webcpporg/pratt at `d78451b`, the submodule
  `libs/pratt` of the superproject; `reference/source/` is that tree (identical to
  `git archive d78451b`).
- **Dependencies:** Boost's headers, 1.92 or newer (Boost.System for `result` and error codes,
  Boost.Decimal for the calculator's default number type, Boost.Core's `lightweight_test` for
  the tests, Boost.Multiprecision in one test) and webcpp::config
  (`/webcpp/config//config`). Nothing is compiled or linked for it. The superproject's
  `webcpp.example` tool feeds an example its `.input` file as standard input (a change made in
  the superproject for pratt's REPLs).
- **Status:** implemented, in release 0.1.0 (`reference/STATUS.md`).
- **Other proposals:** none depends on it yet; the parser of openui-lang, which the
  micro-subset seeds, is a future library or change.
