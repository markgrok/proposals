---
title: "pratt: design"
tags: [webcpp, pratt, parser, design]
status: active
created: 2026-10-08
---

# pratt: design

`pratt` is a library of webcpp's own (not a port): a header-only C++20
engine for Pratt parsing (top-down operator precedence), generic through
concepts, plus a ready-to-use calculator built on it. Its first planned
real consumer is a parser for openui-lang, a view-description language
whose sample block is `root = Stack()`.

The owner's draft prompt, with its verified reference implementation
(Appendices A-D), is `PRATT_DRAFT.md` beside this file. The appendices are
the architectural contract; this design states what is kept, what is
generalized, and every deviation, each with its reason. Where the draft
and this design differ, this design wins; where this design and the
superproject's `AGENTS.md` differ, `AGENTS.md` wins.

## 1. Decisions the owner took while designing (2026-10-08)

1. Targets: native, emscripten, wasip2 and wasip3. Emscripten joins the
   declaration when the superproject gains its emscripten lane (the
   trystero milestone pins Emscripten and runs tests under node); until
   then pratt declares and proves native, wasip2 and wasip3. A target is
   never declared without a lane that proves it.
2. Allocation, a rule for every webcpp library: avoid dynamic allocation
   as far as the job allows, and let the user customize the allocator of
   what is allocated. pratt is born with it (section 4).
3. The calculator's token stream is a lazy lexer: nothing is tokenized
   ahead, no token vector exists.
4. The calculator's environment is a concept, `environment_for`, plus a
   ready-made `environment` with an `Allocator` parameter.
5. The parser bounds its recursion: a configurable depth limit with a
   default measured on the real calculator in the worst configuration,
   and a new enumerator `nesting_too_deep = 8`.
6. Division and remainder by zero are an error, a new enumerator
   `division_by_zero = 9`, not the number type's infinity or NaN.
7. The proof grammars and REPLs as in section 6, with a superproject tool
   extension: an example's `<name>.input` feeds its standard input.
8. Exceptions and RTTI follow the superproject's policy: without
   exceptions on wasip2 only; RTTI never restricted. Since the same
   headers serve wasip2, no header throws, tries or catches.

## 2. Where it lives and how it is built

- Work happens in a worktree of the superproject, `~/dev/webcpp-pratt`,
  on branch `lib/pratt`, created from `main`. The superproject's own
  checkout `~/dev/webcpp` is never touched by this work, so the milestone
  in progress there is not disturbed. The branch is rebased onto `main`
  whenever `main` moves.
- Inside the worktree, `libs/pratt` is a git repository of its own
  (`git init -b main`), untracked by the superproject, until the owner's
  steps: the repository `webcpporg/pratt` and the submodule
  `../pratt.git`, done at publication (the owner has authorized the
  controller to create repositories; it confirms with the owner first).
- The only change outside `libs/pratt` is the tool extension of
  section 6.4, on branch `lib/pratt`, which reaches `main` with pratt.

### 2.1 The tree

```
libs/pratt/
  build.jam                         # project /webcpp/pratt, alias pratt
  meta/libraries.json               # "port-of": null
  meta/include-boundaries.json      # the engine never sees the calculator
  include/webcpp/pratt.hpp          # the engine only: errors + parser
  include/webcpp/pratt/errors.hpp   # error, its category, make_error_code
  include/webcpp/pratt/parser.hpp   # concepts, parser, parse, helpers
  include/webcpp/pratt/calculator.hpp  # namespace calc
  test/Jamfile  test/*_test.cpp  test/sexpr.hpp  test/.clang-tidy (if measured necessary)
  example/Jamfile
  example/openui/grammar.hpp        # the openui micro-subset, shared
  example/openui_repl.cpp  .input  .expected
  example/calculator_repl.cpp  .input  .expected
  example/<guide examples>.cpp  .expected
  doc/Jamfile  doc/mrdocs.yml  doc/pratt.adoc (+ chapter files)
  README.md  AGENTS.md  LICENSE_1_0.txt  .gitignore  .gitattributes
  .github/workflows/ci.yml          # library.yml with library: pratt
```

- `build.jam`: the alias `pratt` with usage requirements
  `<include>include <library>/boost//headers`, xactor's form. Boost.Decimal
  and Boost.System are headers of the installed Boost; no new dependency.
- `meta/libraries.json`: key and name `pratt`, authors `["R"]`,
  maintainers as xactor's, a one-sentence description, category
  `["Parsing"]`, `"cxxstd": "20"`, `"port-of": null`.
- `meta/include-boundaries.json`: `include/webcpp/pratt.hpp`,
  `include/webcpp/pratt/errors.hpp` and `include/webcpp/pratt/parser.hpp`
  must not include `webcpp/pratt/calculator` or `boost/decimal`, "the
  engine knows no grammar and no number type".
- `include/webcpp/pratt.hpp` includes the engine only, so a user of the
  engine does not pull in Boost.Decimal; the calculator is included by
  name.

## 3. The engine (`errors.hpp`, `parser.hpp`)

Appendix A is kept as the architecture: the concepts `token_stream` and
`grammar_for`; `parser<Grammar, Stream>` whose `expression(rbp)` knows no
concrete operator and only calls `nud`, `led` and `lbp`; `parse()`; and the
category helpers `literal`, `prefix`, `infix_left`, `infix_right` (the
`bp - 1` of canonical Pratt), `postfix` and `grouping`, each expressed
through `expression()` recursion. No inheritance, no `std::function`, no
type erasure; a grammar consulting runtime tables stays possible.

Packaging and deviations:

- **Errors** (`errors.hpp`). The enum `error`, the draft's name and
  values, plus two:

  | Enumerator | Value | Message |
  | --- | --- | --- |
  | `unexpected_token` | 1 | unexpected token |
  | `unexpected_end` | 2 | unexpected end of input |
  | `expected_closing_token` | 3 | expected closing token |
  | `trailing_input` | 4 | trailing input after expression |
  | `invalid_assignment_target` | 5 | invalid assignment target |
  | `undefined_variable` | 6 | undefined variable |
  | `invalid_character` | 7 | invalid character |
  | `nesting_too_deep` | 8 | expression nested too deeply |
  | `division_by_zero` | 9 | division by zero |

  Values are fixed and never reused. The category follows xactor's form,
  not the draft's function-local static: a `constexpr` category object
  with its own 64-bit id (`"pratt"` in ASCII, `0x7072617474000000`), the
  name `webcpp.pratt`, an accessor `error_category()` and
  `make_error_code(error)` as xactor names them, and the
  `boost::system::is_error_code_enum` specialization.
- **The depth limit.** `parser` counts the nesting of `expression()` and
  refuses the call that would exceed its limit with
  `error::nesting_too_deep`, before any recursion, so the stack never
  overflows. The limit is a constructor argument, forwarded by `parse()`
  and by `calc::evaluate()`, defaulting to `default_max_depth`, an
  `inline constexpr int` of `parser.hpp`.
  - Why: Pratt recursion costs stack per nesting level; an input such as
    ten thousand `(` would overflow the stack, a crash and not an error.
    Measured with the reference engine on 2026-10-08: wasip2 with the
    default 64 KiB stack overflows past depth 290 at `-O0` and 1022 at
    `-O2`; natively, with 8 MiB, past 10 000.
  - The default is measured, not chosen: the largest power of two whose
    deepest calculator parse uses at most half the default stack of
    wasip2 and wasip3 at `-O0`. The expectation is 64. A test pins that a
    parse at exactly the default depth passes on every target in its
    debug build, and that one level more is `nesting_too_deep`.
  - The documentation says that a program raising the limit raises its
    stack with it (`-Wl,-z,stack-size=` on wasm, the thread's stack
    natively), with the measured cost per level.
- Doc Comments on every public symbol (section 8).

## 4. Allocation

- The engine allocates nothing: `parser` holds a pointer to the grammar,
  the stream by value and a depth counter.
- The calculator's parse allocates nothing: the lazy lexer, the tokens and
  the nodes are views and numbers. Only an environment that stores names
  allocates, and its allocator is the user's.
- The proof grammars and the REPLs are examples: the openui micro-subset
  keeps the draft's `std::string` rendering as its node, its output
  format. The real openui-lang parser defines its own AST under the rule.
- Proof: a test program replaces `operator new` for itself alone and
  counts zero allocations across `calc::evaluate` with a fixed-storage
  environment; another counts, through a counting allocator, that every
  allocation of `calc::environment` goes through its allocator.

## 5. The calculator (`calculator.hpp`, namespace `calc`)

Appendix B's semantics are kept: numbers and variables; `+ - * / % ^`
with right-associative `^`; parentheses; unary `+`/`-` binding below `^`
(`-2 ^ 2 == -4`); the comparisons `== != < <= > >=` giving `Number(1)` or
`Number(0)`; right-associative `=` writing into the environment; prefix
and postfix `++`/`--` with C semantics, on variables only; the binding
powers `assign 10, compare 20, additive 30, multiplicative 40, unary 45,
power 50, postfix 70`; the lvalue-tracking node (a bare variable keeps its
name; an undefined variable errors only when read). Everything goes
through the engine and the helpers.

- **`number<N>`**, a documented concept: construction from `int`; binary
  `+ - * /`; unary `-`; the six comparisons; and `pow`, `fmod` and `trunc`
  found by ADL with the two-step pattern (`using std::pow; pow(a, b)`), so
  `boost::decimal`'s overloads serve the default and `std::`'s serve
  builtin types. Every `calc` template is constrained by it.
- **`token<Number>`**: the kind (the draft's kinds plus `invalid` and the
  end), the value, and the `std::string_view` of its text.
- **`stream<Number>`, the lazy lexer** (decision 3): it holds the source
  view, the position and one token of lookahead; `peek()` returns it,
  `advance()` returns it and lexes the next. A number is built in `Number`
  arithmetic only: the digits make an integer mantissa (`v = v * 10 + d`),
  divided by `10^k` when a fraction follows, never through `double`.
  - A character no token starts with lexes as a token of kind `invalid`,
    whose `nud` and `led` return `invalid_character` and whose `lbp` is
    the highest, so the engine reaches it wherever it stands: `1 $ 2` and
    `$` both give `invalid_character`, as in the reference, and the
    engine stays the only place that decides.
- **`node<Number>`**: value, `has_value`, and the name as a view.
- **`environment_for<E>`**, a concept (decision 4):
  - `typename E::number_type`, which models `number`;
  - `e.lookup(std::string_view) -> const number_type*`, `nullptr` when the
    name is not set;
  - `e.assign(std::string_view, number_type) -> result<void>`, which may
    fail (a fixed storage that is full); the calculator returns that
    error as it is.
- **`environment<Number = boost::decimal::decimal64_t, Allocator =
  std::allocator<...>>`**: a `std::map` keyed by a string using the same
  allocator, with transparent lookup; models `environment_for`. With the
  standard `Allocator` parameter it takes any allocator, a polymorphic
  one included, whatever mechanism the allocators milestone settles.
- **`grammar<Environment>`**: holds a pointer to the environment.
- **Exact power.** When the exponent is integral (`trunc(b) == b`), `^`
  computes by repeated squaring in `Number` arithmetic, halving the
  exponent with `trunc`, and a negative exponent is `1 / x^n`; only a
  non-integral exponent calls the generic `pow`. So
  `43 + 12 + 2 * (44 - 12) ^ 4` is exactly `2097207` in decimal and in
  `double`. Its test is written first.
- **Division by zero** (decision 6): `/` and `%` with a zero divisor
  return `division_by_zero`.
- **`evaluate(std::string_view src, Environment& env, int max_depth =
  default_max_depth) -> result<typename Environment::number_type>`**:
  constrained by `environment_for`, so `evaluate(src, env)` works for any
  environment and the number type follows it.

## 6. The proof grammars, the REPLs and the input extension

### 6.1 sexpr (test only)

`test/sexpr.hpp`: Appendix C's grammar, token `char`, node `std::string`,
engine and helpers unchanged; its four acceptance cases.

### 6.2 The openui-lang micro-subset (example tree)

`example/openui/grammar.hpp`, shared by the REPL and `test/openui_test.cpp`:
Appendix D's shape. Identifiers, non-negative integers, double-quoted
strings without escapes, and `( ) , : =`; right-associative `=`; `:` a
low-binding-power pair; `(` with both denotations, nud a grouping and led
a call with the highest binding power whose argument loop lives in the
led. Its lexer is lazy, as the calculator's: an unknown character is an
`invalid` token (`invalid_character`), an unterminated string an
`unterminated` token whose nud and led return `unexpected_end`. Its node
is the S-expression rendering of Appendix D. Errors reuse the library's
enumerators. No recursive descent around the engine.

### 6.3 The REPLs

`example/calculator_repl.cpp` and `example/openui_repl.cpp`: read lines
from standard input; skip an empty line; end with status 0 at end of
input or on the line `exit`; print each result, or `error: ` and the
category's message. The calculator keeps one `calc::environment<>` for
the session and prints a `decimal64_t` in its shortest form that keeps
the value (Boost.Decimal's `to_chars`): `2097207`, `1048503.5`. Standard
input and output appear only in these programs; the lint's world rule
covers library headers, not examples.

### 6.4 The input extension (superproject tools)

`webcpp.example` feeds `<name>.input`, when it exists beside the source,
to the program's standard input, and compares its output with
`<name>.expected` as today, on every target (wasmtime passes standard
input through). `tools/webcpp.jam` and `tools/example/run_example.py`
change; `tools/test/webcpp_jam_test.py` gains a case with a fixture
example that echoes its input, red first; AGENTS.md's example paragraph
says so.

- `openui_repl.input` holds the transcript's input lines of Appendix D,
  and `openui_repl.expected` its outputs.
- `calculator_repl.input` holds Appendix C's values and errors as a
  session, a division by zero, and a line of ten thousand `(`, which must
  answer `error: expression nested too deeply` instead of crashing.

## 7. Tests (lightweight_test, each a `webcpp.run`; written first)

- `engine_test`: precedence; left and right associativity (the `bp - 1`);
  nud/led dispatch; every enumerator 1-9; the depth limit at the default
  and one beyond, and a custom limit; `static_assert`s that the shipped
  and test grammars model the concepts, and negative ones that a type
  missing `nud`, `led` or `lbp` does not model `grammar_for`.
- `calculator_test`: Appendix C's values and errors under `decimal64_t`,
  all exact, and under `double` for the values exact in binary; the
  integral-power path, exact for large integral exponents and negative
  ones; `division_by_zero` for `/` and `%`; a fixed-storage environment
  whose `assign` fails, its error returned; the counting allocator.
- `allocation_test`: zero allocations in `evaluate` (section 4).
- `sexpr_test`, `openui_test`: their acceptance cases, including every
  line of Appendix D's transcript.
- `webcpp.headers-alone pratt : ../include ;` and the include boundary.
- The two REPLs through `.input`/`.expected` (section 6.4), and the
  guide's examples.

## 8. Documentation

- Doc Comments on every public symbol of `webcpp::pratt`: a one-sentence
  brief, `@param`, `@tparam`, `@return` where they apply, `@note`, `@see`
  with a section title of the page; only the allowed commands. MrDocs
  builds the reference strict; `doc_comments.py` passes.
- `doc/pratt.adoc`, xactor's page as the model: an introduction (what
  Pratt parsing is, why nud/led, a first example); writing a grammar
  (token stream, grammar, node, the helpers); the calculator (the number
  type and exactness, the environment and allocation, errors); the
  openui micro-subset and the REPLs; errors; limits (the depth limit, the
  stack and the measured cost per level); allocation; testing; the
  MrDocs reference. Every listing is a compiled example and every output
  an `.expected`; every count is an attribute. Links to xactor's or
  xstate's page use `{webcpp-libs}` and `{webcpp-page}`.

## 9. Lanes and process

- Lanes: native (clang, gcc-15, and the CI's libstdc++ lanes), wasip2 and
  wasip3; emscripten when its lane exists (decision 1). MSVC through the
  CI's lanes.
- Process: this design, then an implementation plan the owner reviews and
  chooses an execution method for; failing tests first; commits in
  `libs/pratt` and on `lib/pratt` as `R <github.overboard312@passmail.net>`,
  without a trailer; nothing pushed until publication; the lint, the
  docs and every lane green before it.
- No file of webcpp names the consumer's parent project; openui-lang is
  named alone.

## 10. Review focus

1. Error order: an invalid character after a complete expression
   (`1 $ 2`) must give `invalid_character`, not `trailing_input`, with
   the lazy lexer.
2. Exactness: a decimal literal with many fraction digits, a large
   integral exponent, a negative integral exponent, `0 ^ 0`.
3. The depth limit on wasip2 in a debug build, with the real calculator's
   frames, not the reference's minimal grammar.
4. `++`/`--` and `=` on an undefined or non-variable target, inside
   parentheses (`(a) = 1`, `(a)++`), as the reference behaves.
5. A REPL line longer than any buffer, and input with `\r\n` endings.
