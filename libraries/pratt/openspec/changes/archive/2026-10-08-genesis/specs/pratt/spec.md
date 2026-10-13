## Purpose

webcpp::pratt is a header-only C++20 library of webcpp's own: a generic engine for Pratt parsing
(top-down operator precedence) that knows no token, no operator, no tree and no number type, and a
calculator built on it that computes exactly in decimal by default. A program gives the engine a
token stream and a grammar, both checked as concepts, and gets back the grammar's node or an
error, as a value; the engine allocates nothing, throws nothing and bounds its recursion, and the
library runs natively, on wasip2, wasip3 and Emscripten. These requirements describe 0.1.0 as
built (`reference/source/`).

## ADDED Requirements

### Requirement: Library: Header-only library in its namespaces
The library SHALL be header-only C++20 (`"cxxstd": "20"`), with the engine's names in namespace `webcpp::pratt`, the calculator's in `webcpp::pratt::calc`, internals in `webcpp::pratt::detail` and `webcpp::pratt::calc::detail`, public headers under `<webcpp/pratt/...>` (one per responsibility), the one macro it defines prefixed `WEBCPP_PRATT_` and include guards `WEBCPP_PRATT_<PATH>_HPP` (`WEBCPP_PRATT_HPP`, `WEBCPP_PRATT_CONFIG_HPP`, `WEBCPP_PRATT_ERRORS_HPP`, `WEBCPP_PRATT_PARSER_HPP`, `WEBCPP_PRATT_CALCULATOR_HPP`). It ports nothing: `meta/libraries.json` has `"port-of": null`, and the library has no oracle, no twins and no "Differences" appendix; its page and the Doc Comments of its headers are what a change is checked against. Every source file MUST carry the notice "Copyright (c) 2026 WebCpp.org" under the Boost Software License, Version 1.0 (`LICENSE_1_0.txt`).

#### Scenario: A program uses the engine without building anything
- **WHEN** a b2 program names `/webcpp/pratt//pratt` and includes `<webcpp/pratt.hpp>`
- **THEN** it compiles and links without any library of pratt's own being built, since every header is self-contained

#### Scenario: The metadata says the library is not a port
- **WHEN** `meta/libraries.json` is read
- **THEN** it has key and name `pratt`, authors `["R"]`, category `["Parsing"]`, `"cxxstd": "20"`, maintainers `["R <github.overboard312@passmail.net>"]`, the description "A generic Pratt parser engine for C++20, with a decimal calculator built on it." and `"port-of": null`

### Requirement: Library: Public headers and the convenience header
The library SHALL ship these public headers under `include/webcpp/pratt/`: `config.hpp`, `errors.hpp`, `parser.hpp` and `calculator.hpp`, and the convenience header `include/webcpp/pratt.hpp`, which MUST include `config.hpp`, `errors.hpp` and `parser.hpp` and MUST NOT include `calculator.hpp`: the calculator brings Boost.Decimal, a heavy dependency the engine does not need, and is included by name. Every public header but `config.hpp` itself MUST include `<webcpp/pratt/config.hpp>` first, and `config.hpp` includes `<webcpp/config.hpp>` first. Each public header MUST compile alone as the only include of its translation unit (`webcpp.headers-alone pratt : ../include`).

#### Scenario: The convenience header leaves the calculator out
- **WHEN** a program includes only `<webcpp/pratt.hpp>` and calls `pratt::parse` with a grammar of its own
- **THEN** it compiles, and neither `<webcpp/pratt/calculator.hpp>` nor `<boost/decimal.hpp>` is included

#### Scenario: The calculator is included by name
- **WHEN** a program includes `<webcpp/pratt/calculator.hpp>` and calls `calc::evaluate("1 + 1", env)` with `calc::environment<> env`
- **THEN** it compiles and the call returns the value 2 as a `decimal64_t`

### Requirement: Library: Build target and dependencies
`build.jam` SHALL declare `project /webcpp/pratt` and one alias `pratt` whose usage requirements are `<include>include`, `<library>/boost//headers` and `<library>/webcpp/config//config`, and nothing else: no source file is compiled and no library is linked for pratt. The library needs Boost's headers, 1.92 or newer (Boost.System for `result` and error codes, Boost.Decimal for the calculator's default number type), and webcpp's configuration (`/webcpp/config//config`).

#### Scenario: One alias brings everything
- **WHEN** a program names `/webcpp/pratt//pratt`
- **THEN** pratt's include path, Boost's headers and webcpp's configuration reach its compile line, and nothing of pratt reaches its link line

### Requirement: Library: The engine never includes the calculator
`meta/include-boundaries.json` SHALL declare one boundary: the headers `include/webcpp/pratt.hpp`, `include/webcpp/pratt/errors.hpp` and `include/webcpp/pratt/parser.hpp` MUST NOT include `webcpp/pratt/calculator` or `boost/decimal`, with the reason "the engine knows no grammar and no number type", and the superproject's lint enforces it.

#### Scenario: The boundary file
- **WHEN** `meta/include-boundaries.json` is read
- **THEN** it holds one entry in `boundaries` whose `headers` are the three engine headers, whose `must-not-include` is `["webcpp/pratt/calculator", "boost/decimal"]` and whose `why` is "the engine knows no grammar and no number type"

### Requirement: Library: Targets and exceptions
The tests and examples SHALL be declared for the targets `native`, `emscripten`, `wasip2` and `wasip3` (`webcpp.targets native emscripten wasip2 wasip3` in `test/Jamfile` and `example/Jamfile`), every target built with exceptions (as WebAssembly's exception handling on WASI) and with RTTI never restricted. No header MUST contain `throw`, `try` or `catch`: the same headers serve a build with exceptions and one without, which the lint checks by compiling every public header, every test and every example with `exception-handling=off`; a header raises only through `boost::throw_exception`. The library uses no thread, no clock, no global or static mutable state and no allocation in the engine.

#### Scenario: A header throws nothing of its own
- **WHEN** the headers under `include/` are searched for `throw`, `try` and `catch`
- **THEN** none is found, and pratt's page says a template that no test or example instantiates is not checked by the build without exceptions

### Requirement: Config: Exceptions macro
`<webcpp/pratt/config.hpp>` SHALL include `<webcpp/config.hpp>` first and define `WEBCPP_PRATT_NO_EXCEPTIONS` exactly when `WEBCPP_HAS_EXCEPTIONS` is not defined (webcpp has no exceptions: the compiler has none, or the program defined `WEBCPP_NO_EXCEPTIONS` for every webcpp library), and it MUST keep the macro when a program defines it for pratt alone in a build that has exceptions. pratt raises no exception of its own, so the macro changes nothing in it; it exists so that a program can set it for every webcpp library alike. Under `__MRDOCS__` the header defines the macro and undoes it at once, so that the reference lists it with its Doc Comment while the parse goes on as a build with exceptions.

#### Scenario: The macro follows webcpp's exceptions
- **WHEN** `test/config_test.cpp` is compiled as the build asks (target `config`)
- **THEN** it compiles only if `defined(WEBCPP_HAS_EXCEPTIONS) != defined(WEBCPP_PRATT_NO_EXCEPTIONS)`, and fails with "WEBCPP_PRATT_NO_EXCEPTIONS is not defined exactly when webcpp has no exceptions" otherwise

#### Scenario: A developer's own definition is kept
- **WHEN** the same file is compiled with `<define>WEBCPP_PRATT_NO_EXCEPTIONS <define>WEBCPP_TEST_PRATT_DEVELOPER_DEFINED` (target `config_developer_defined`)
- **THEN** it compiles only if the macro is still defined after `config.hpp`

#### Scenario: One definition for every library
- **WHEN** the same file is compiled with `<define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_PRATT_ALL_OFF` (target `config_all_off`)
- **THEN** it compiles only if `WEBCPP_PRATT_NO_EXCEPTIONS` is defined, and `config.hpp` has included `<webcpp/config.hpp>` (`WEBCPP_CONFIG_HPP` defined)

### Requirement: Errors: The error enumeration
The library SHALL declare `enum class error : int` in `webcpp::pratt` with exactly these enumerators and values, fixed and never reused for a different meaning: `unexpected_token` = 1 (a token appears where the grammar has no nud or led for it), `unexpected_end` = 2 (the input ends where a token was still expected), `expected_closing_token` = 3 (a closing token the grammar requires is missing), `trailing_input` = 4 (input remains after a complete expression), `invalid_assignment_target` = 5 (the left side of an assignment or an increment is not a name), `undefined_variable` = 6 (a name is read before anything has written it), `invalid_character` = 7 (a character the lexer reads as no token), `nesting_too_deep` = 8 (a parse recursed past the depth limit) and `division_by_zero` = 9 (a division or a remainder by zero). The value 0 is no error, a new error takes the next value, and an `error` MUST convert to a `boost::system::error_code` and compare with one (`boost::system::is_error_code_enum<webcpp::pratt::error>` is `std::true_type`).

#### Scenario: The values are pinned
- **WHEN** `test/errors_test.cpp` casts each enumerator to `int`
- **THEN** `unexpected_token` is 1, `unexpected_end` 2, `expected_closing_token` 3, `trailing_input` 4, `invalid_assignment_target` 5, `undefined_variable` 6, `invalid_character` 7, `nesting_too_deep` 8 and `division_by_zero` 9

#### Scenario: An error compares with a result's error
- **WHEN** `const pratt::result<int> r = pratt::error::unexpected_end;` is built
- **THEN** `r.has_error()` holds and `r.error() == pratt::error::unexpected_end`

### Requirement: Errors: The category and its messages
The library SHALL provide `pratt::error_category()` (`noexcept`), returning a constant category with a fixed identity, the ASCII of "pratt" followed by three zero bytes (`0x7072617474000000`), constructed `constexpr` (`detail::category`, `detail::the_category`) and so holding no mutable state, and `pratt::make_error_code(error)` (`noexcept`). The category's `name()` MUST be "webcpp.pratt" and its `message(value)` MUST be: "unexpected token" (1), "unexpected end of input" (2), "expected closing token" (3), "trailing input after expression" (4), "invalid assignment target" (5), "undefined variable" (6), "invalid character" (7), "expression nested too deeply" (8), "division by zero" (9), and "unknown webcpp.pratt error" for any other value. The name, the identity and the messages never change.

#### Scenario: The category and a message
- **WHEN** `const boost::system::error_code code = pratt::error::nesting_too_deep;` is built
- **THEN** `code.category() == pratt::error_category()`, `std::string(pratt::error_category().name())` is "webcpp.pratt" and `code.message()` is "expression nested too deeply"

#### Scenario: Every message and the unknown one
- **WHEN** the message of each of `unexpected_token`, `unexpected_end`, `expected_closing_token`, `trailing_input`, `invalid_assignment_target`, `undefined_variable`, `invalid_character` and `division_by_zero` is read through `boost::system::error_code(...)`, and `error_category().message(0)` and `message(10)` are read
- **THEN** each is the text above, and the last two are "unknown webcpp.pratt error"

### Requirement: Errors: A message copied into a buffer
The category SHALL provide `message(int value, char* buffer, std::size_t size) const noexcept`, which copies the message of `value` into `buffer` cut to fit `size` bytes with its terminating zero, never writes past `size`, writes nothing when `buffer` is null or `size` is 0, and returns the whole message whatever the buffer.

#### Scenario: The buffer forms of "division by zero"
- **WHEN** the message of `division_by_zero` (16 characters) is requested with a null buffer and size 8, with a buffer of size 0, of size 1, of size 9 and of size 17, each buffer filled with `'x'` beforehand
- **THEN** each call returns "division by zero"; the buffer of size 0 is untouched (`'x'`); the buffer of size 1 holds `'\0'` then `'x'`; the buffer of size 9 holds "division" and `buffer[9]` is `'x'`; the buffer of size 17 holds "division by zero" and `buffer[17]` is `'x'`

### Requirement: Errors: The result type
The library SHALL define `template <class T> using result = boost::system::result<T>` in `webcpp::pratt`, the type of every operation that can fail, of every nud and of every led. Its error is an `error` or an error code of any category that a grammar or an environment returns, which pratt passes on unchanged. The engine and the calculator MUST build an error into a `result` in place (`{boost::system::in_place_error, error::...}`) wherever the node may convert from an error, as a Boost.Multiprecision number does, because `result` refuses an implicit conversion from a value that converts both to its `T` and to an error code.

#### Scenario: A node that converts from anything
- **WHEN** `pratt::parse` is called with a grammar whose `node_type` converts from any type (and so from `pratt::error`) on the inputs `(a)`, `(a`, `a)` and `((((a))))` with a limit of 4
- **THEN** the first has a value, and the others have the errors `expected_closing_token`, `trailing_input` and `nesting_too_deep`

### Requirement: Engine: The token stream concept
The library SHALL declare `template <class S> concept token_stream` in `webcpp::pratt`, satisfied by a type that names its tokens `token_type` and has `peek() const` convertible to `token_type`, `advance()` returning exactly `token_type`, and `at_end() const` convertible to `bool`. The parser reads `peek` to decide whether the next token extends an expression, calls `advance` once for each token it hands a nud or a led, and asks `at_end` once a whole expression is parsed. `peek` MUST stay valid at the end of the input, returning a token whose lbp is 0 and whose nud fails, such as an end token. A stream may hold every token in advance or lex lazily.

#### Scenario: The shipped and the test streams model it
- **WHEN** `static_assert`s check `token_stream` of the tests' `sexpr::stream` (tokens `char`), `calc::stream<decimal64_t>`, `calc::stream<double>` and `openui::stream`
- **THEN** all hold

#### Scenario: A stream missing one member does not model it
- **WHEN** `token_stream` is checked for types that lack `peek`, `advance` or `at_end` respectively
- **THEN** each check is false, while a type with all three and `token_type` is true

### Requirement: Engine: The grammar concept
The library SHALL declare `template <class G, class S> concept grammar_for`, satisfied when `S` models `token_stream` and `G` names its node `node_type`, has `lbp(t)` (const, convertible to `int`), `nud(p, t)` and `led(p, t, left)` (const, each returning exactly `result<G::node_type>`) for a `parser<G, S>& p`, a token `t` and a node moved in as `left`. The node a parse builds is the grammar's, which the library never looks into. A led is called only for a token whose lbp is greater than the binding power the expression is parsed with, so a token of lbp 0 ends every expression parsed with 0 or more.

#### Scenario: Complete grammars model it
- **WHEN** `static_assert`s check `grammar_for` of the tests' `table`, `recording`, `accepting` and `sexpr::grammar` over `sexpr::stream`, of `calc::grammar<calc::environment<>>` over `calc::stream<decimal64_t>`, of `calc::grammar<calc::environment<double>>` over `calc::stream<double>`, of `calc::grammar<fixed_environment<decimal64_t>>` over `calc::stream<decimal64_t>`, and of `openui::grammar` over `openui::stream`
- **THEN** all hold

#### Scenario: A grammar missing one member does not model it
- **WHEN** `grammar_for` is checked for grammars that lack `nud`, `led`, `lbp` or `node_type` respectively
- **THEN** each check is false

### Requirement: Engine: The parser class
The library SHALL provide `template <class Grammar, class Stream> class parser` with member types `token_type` and `node_type`, constructed as `parser(const Grammar& grammar, Stream stream, int max_depth = default_max_depth)`, which holds a pointer to the grammar (the grammar MUST outlive the parser), the stream by value (moved in) and the depth of its active `expression` calls, and allocates nothing. The constructor taking `const Grammar&&` MUST be deleted, so that a temporary grammar, which the pointer would leave dangling, does not compile.

#### Scenario: A temporary grammar is refused
- **WHEN** `std::is_constructible_v<parser<table, sexpr::stream>, table, sexpr::stream>` and `std::is_constructible_v<parser<table, sexpr::stream>, const table&, sexpr::stream>` are checked
- **THEN** the first is false and the second true

### Requirement: Engine: The expression algorithm
`parser::expression(int rbp)` SHALL parse the longest expression whose operators bind tighter than `rbp`: it consumes a token with `advance` and calls the grammar's `nud` on it; then, while `lbp(peek())` is greater than `rbp`, it consumes that token and calls the grammar's `led` with the token and the node parsed so far (moved in); it returns the node, or the first error a nud or a led returned, immediately. It knows no operator and calls the grammar's `nud`, `led` and `lbp` only. Precedence is the comparison of two numbers, and a token may have both a nud and a led.

#### Scenario: A tighter operator takes the operand between
- **WHEN** the test `table` grammar parses `a+b*c` and `a*b+c`
- **THEN** the nodes are `(+ a (* b c))` and `(+ (* a b) c)`

#### Scenario: Associativity
- **WHEN** it parses `a-b-c` (left, `infix_left`) and `a^b^c` (right, `infix_right`)
- **THEN** the nodes are `(- (- a b) c)` and `(^ a (^ b c))`

#### Scenario: The first token's nud and the order of the calls
- **WHEN** the `recording` grammar, which writes down each call before making it, parses `a+b`
- **THEN** the calls are exactly `nud a`, `led +`, `nud b`, and the node is `(+ a b)`

#### Scenario: Prefix and postfix positions
- **WHEN** the `table` grammar (prefix `-` with power 25, postfix `!` with lbp 40) parses `-2^2`, `-a*b`, `a!`, `a+b!` and `-a!`
- **THEN** the nodes are `(neg (^ 2 2))`, `(* (neg a) b)`, `(! a)`, `(+ a (! b))` and `(neg (! a))`

### Requirement: Engine: Parsing a whole input
The library SHALL provide `pratt::parse(const Grammar& grammar, Stream stream, int max_depth = default_max_depth)`, constrained by `token_stream<Stream>` and `grammar_for<Grammar, Stream>`, returning `result<typename Grammar::node_type>`: it constructs a `parser`, calls `parser::parse()`, which parses `expression(0)` and returns the node, the first error of a nud or a led, or `error::trailing_input` when `at_end()` is false after the expression. `parse` takes the grammar by `const&`, so a temporary grammar is accepted (its parser lives only for the call). A program may construct the `parser` itself to call `expression` with another power or to go on reading the stream after one expression.

#### Scenario: Errors the engine and the grammar report
- **WHEN** the `table` grammar parses ``, `a+`, `)`, `*a`, `a)` and `ab`
- **THEN** the errors are `unexpected_end`, `unexpected_end`, `unexpected_token`, `unexpected_token`, `trailing_input` and `trailing_input`

#### Scenario: The convenience of a temporary grammar
- **WHEN** `pratt::parse(sexpr::grammar{}, sexpr::stream("1+2*3"))` is called
- **THEN** it compiles and returns `(+ 1 (* 2 3))`

### Requirement: Engine: Peeking and advancing from a grammar
The parser SHALL provide `peek() const` (returning what the stream's `peek` returns, `[[nodiscard]]`, `decltype(auto)`) and `advance()` (consuming the next token and returning it as `token_type`), for a nud or a led that reads past its own token, such as the argument loop of a call.

#### Scenario: A refused call reads no token
- **WHEN** `parser.expression(0)` on `((((a))))` with `max_depth` 4 fails with `nesting_too_deep`
- **THEN** `parser.peek()` is `'a'`: the four `(` were consumed by the four active calls and the refused fifth call read nothing

### Requirement: Engine: The depth limit
The library SHALL define `inline constexpr int default_max_depth = 64` and count the active `expression` calls of a parser (`detail::depth_guard`, an RAII guard that is neither copyable nor movable and uncounts on every path): a call that would make more than `max_depth` calls active MUST return `{boost::system::in_place_error, error::nesting_too_deep}` before it reads any token, and a negative `max_depth` MUST behave as 0, which refuses every parse. The limit bounds the recursion through `expression` alone, which is the whole recursion of a grammar whose nuds and leds parse every operand through `expression` or the helpers; a nud or a led that recurses otherwise bounds its recursion itself. After an error a parser is reusable: the count returns to its value.

#### Scenario: The limit on both sides
- **WHEN** the `table` grammar parses `((((a))))` (4 pairs, 5 calls) with `max_depth` 4, `(((a)))` with 4, and `a` with 0 and with -1
- **THEN** the first is `nesting_too_deep`, the second is `a`, and the last two are `nesting_too_deep`

#### Scenario: What costs a level
- **WHEN** `---a` is parsed with `max_depth` 4, `----a` with 4, `a^b^c^d` with 4, `a^b^c^d^e` with 4, `a-b-c-d-e-f` with 2, `a-b` with 1 and `a!!!` with 1
- **THEN** the first and the third succeed (`(neg (neg (neg a)))`, `(^ a (^ b (^ c d)))`), the second and the fourth are `nesting_too_deep`, the fifth succeeds as `(- (- (- (- (- a b) c) d) e) f)`, the sixth is `nesting_too_deep`, and the seventh succeeds as `(! (! (! a)))` (a postfix costs no level)

#### Scenario: The default
- **WHEN** `pratt::parse(table{}, sexpr::stream(nested(n)))` runs with `n` equal to `default_max_depth - 1` and to `default_max_depth`, `nested(n)` being `n` opening parentheses, `a` and `n` closing ones
- **THEN** the first returns `a` and the second `nesting_too_deep`

#### Scenario: A parser is reusable
- **WHEN** a parser over `((((a))))` with limit 4 fails, then is asked `expression(0)` again; and a parser over `a+)a` with limit 2 fails with `unexpected_token`, then is asked again; and a parser over `(((a)))(((a)))` with limit 4 is asked twice
- **THEN** each second (or later) call returns `a`

### Requirement: Engine: The literal helper
The library SHALL provide `literal(make, token)`, the nud of an atom, returning `make(token)` (its return type is `decltype(make(token))`): the token is the whole expression.

#### Scenario: Atoms
- **WHEN** the `sexpr::grammar` parses `1+2*3` and the `pratt_helpers` grammar parses `1-2-3`
- **THEN** their digits are the atoms, and the nodes are `(+ 1 (* 2 3))` and `(- (- 1 2) 3)`

### Requirement: Engine: The prefix helper
The library SHALL provide `prefix(p, bp, make)`, the nud of a prefix operator: it parses one operand with `p.expression(bp)`, returns the operand's error if it has one, and otherwise returns `make(std::move(*operand))`.

#### Scenario: A sign below the power operator
- **WHEN** the `pratt_helpers` grammar (sign 30, power 40, factorial 50) parses `-2^2`, `-3!` and `2*-3`
- **THEN** the nodes are `(- (^ 2 2))`, `(- (! 3))` and `(* 2 (- 3))`

### Requirement: Engine: The infix helpers
The library SHALL provide `infix_left(p, bp, left, make)`, the led of a left-associative operator, which parses the right operand with `p.expression(bp)`, and `infix_right(p, bp, left, make)`, the led of a right-associative operator, which parses it with `p.expression(bp - 1)` so that an equal operator on the right takes it. Each returns the right operand's error if it has one, and otherwise `make(std::forward<Node>(left), std::move(*right))`, `left` being passed as it is received (a led that owns it moves it in).

#### Scenario: The same power, two associativities
- **WHEN** the `pratt_helpers` grammar parses `1-2-3` and `2^3^2`
- **THEN** the nodes are `(- (- 1 2) 3)` and `(^ 2 (^ 3 2))`

#### Scenario: The sexpr grammar
- **WHEN** the tests' `sexpr::grammar` parses `2^3^2`, `1-2-3` and `-(1+2)*3`
- **THEN** the nodes are `(^ 2 (^ 3 2))`, `(- (- 1 2) 3)` and `(* (neg (+ 1 2)) 3)`

### Requirement: Engine: The postfix helper
The library SHALL provide `postfix(left, make)`, the led of a postfix operator, returning `make(std::forward<Node>(left))`: the operand is the whole left expression and nothing more is parsed.

#### Scenario: A postfix tighter than everything
- **WHEN** the `pratt_helpers` grammar parses `-3!`
- **THEN** the node is `(- (! 3))`, the postfix taking `3` before the sign does

### Requirement: Engine: The grouping helper
The library SHALL provide `grouping(p, is_close)`, the nud of an opening token: it parses a whole inner expression with `p.expression(0)`, returns its error if it has one, returns `error::expected_closing_token` (built in place) when `is_close(p.peek())` is false, and otherwise consumes the closing token with `p.advance()` and returns the inner node.

#### Scenario: A grouping needs its closing token
- **WHEN** the `table` grammar parses `(a+b)*c` and `(a+b`, and the `pratt_helpers` grammar parses `(1+2)*3`, `(1+2`, `1+)` and `()`
- **THEN** the nodes are `(* (+ a b) c)` and `(* (+ 1 2) 3)`, the errors are `expected_closing_token` for the unclosed ones and `unexpected_token` for `1+)` and `()`

### Requirement: Engine: Allocation and determinism
The engine SHALL allocate nothing: a `parser` holds a pointer to its grammar, its stream and two integers, and `expression`, `parse` and the helpers allocate nothing themselves; what a parse allocates is what the grammar's nodes and stream allocate. A parse of the same input by the same grammar MUST give the same result every time, with no dependence on a clock, a thread, global state or an exception.

#### Scenario: A grammar over characters
- **WHEN** the first-grammar program `pratt_first_grammar` evaluates `1+2*3`, `2*3+4`, `8-2-1`, `-2*3`, `1+`, `1+*2` and `12`
- **THEN** it prints `7`, `10`, `5`, `-6`, `error: unexpected end of input`, `error: unexpected token` and `error: trailing input after expression`, each after `<text> -> `

### Requirement: Calculator: The calculator header and its number concept
`<webcpp/pratt/calculator.hpp>` SHALL declare `template <class N> concept number` in `webcpp::pratt::calc`: `N` is copyable, constructible from `int`, has binary `+ - * /` and unary `-` convertible to `N`, the six comparisons convertible to `bool`, and `pow(a, b)`, `fmod(a, b)` and `trunc(a)` found by ADL after `using std::pow;`, `using std::fmod;` and `using std::trunc;` (the two-step call, `detail::math::adl_pow`, `adl_fmod`, `adl_trunc`), each returning exactly `N` so that the calculator never computes through another type. `int` MUST NOT model it (std's functions return `double` for an `int`), nor a type whose functions return a proxy such as an expression template with `et_on`; the same type with `et_off` does.

#### Scenario: Which types model it
- **WHEN** `static_assert`s check `calc::number` of `decimal64_t`, `double`, `std::string`, `int` and Boost.Multiprecision's `number<cpp_dec_float<50>, et_off>`
- **THEN** it holds for `decimal64_t`, `double` and the multiprecision decimal, and not for `std::string` or `int`; and `std::is_convertible_v<pratt::error, decimal50>` is also true

### Requirement: Calculator: Token kinds and tokens
The calculator SHALL declare `enum class kind` with exactly these enumerators: `number`, `name`, `plus`, `minus`, `star`, `slash`, `percent`, `caret`, `lparen`, `rparen`, `assign`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `increment`, `decrement`, `invalid` and `end`, and `template <number Number> struct token` with members `kind type = kind::end`, `Number value{0}` (the value of a number token, 0 for every other kind) and `std::string_view text{}` (the token's characters, a view into the source). `kind::invalid` is a character the lexer reads as no token, one character long.

#### Scenario: A token of each kind
- **WHEN** the lexer reads `a = 43 + 12` over `decimal64_t`
- **THEN** it yields `(name, "a", 0)`, `(assign, "=", 0)`, `(number, "43", 43)`, `(plus, "+", 0)`, `(number, "12", 12)`, then the end token

### Requirement: Calculator: The lazy lexer
The calculator SHALL provide `template <number Number> class stream`, modelling `token_stream`, with `token_type = token<Number>`, constructed `explicit stream(std::string_view source)` (it lexes the first token), which holds the source, its position and one token of lookahead and lexes the next token only when `advance` consumes the current one; `peek() const` returns the next token, valid until the next `advance`; `advance()` consumes and returns it; `at_end() const` is true when the next token is the end token. At the end `advance` MUST return the end token again, every time (kind `end`, empty text, value 0), `peek` MUST NOT consume, and a token's `text` MUST be a view into the source, never a copy. The source must outlive the stream and its tokens.

#### Scenario: The end token stays
- **WHEN** a lexer over `7` is advanced past the number and then `peek`, `advance` and `at_end` are called three more times
- **THEN** `at_end()` is true and both `peek().type` and `advance().type` are `end` each time

#### Scenario: Texts point into the source
- **WHEN** a lexer over `ab + 12` advances three times
- **THEN** the tokens' `text.data()` are `source.data()`, `source.data() + 3` and `source.data() + 5`

#### Scenario: Peek does not consume
- **WHEN** a lexer over `x+` has `peek().text` read twice, then `advance().text`, then `peek().text`
- **THEN** they are `x`, `x`, `x` and `+`

### Requirement: Calculator: The lexical grammar
The lexer SHALL read a number as digits, then optionally `.` followed by at least one digit and more digits; a name as an ASCII letter or `_` followed by letters, digits and `_`; space, tab, carriage return and line feed as separators; the two-character operators `++ -- == != <= >=` (tried before the one-character ones); the one-character operators `+ - * / % ^ ( ) = < >`; and any other character, `!` without `=` and `.` without digits after it included, as a token of kind `invalid` one character long. A non-ASCII character is several bytes of UTF-8, each an `invalid` token, so a parse stops at its first byte.

#### Scenario: Two-character operators first
- **WHEN** the lexer reads `++ -- == != <= >=` and then `+++`
- **THEN** the kinds are `increment`, `decrement`, `eq`, `ne`, `le`, `ge`, and then `increment`, `plus`

#### Scenario: One-character operators
- **WHEN** the lexer reads `+-*/%^()=<>`
- **THEN** the kinds are `plus`, `minus`, `star`, `slash`, `percent`, `caret`, `lparen`, `rparen`, `assign`, `lt`, `gt` with the same one-character texts

#### Scenario: Names
- **WHEN** the lexer reads `_x1 Ab9 2y`
- **THEN** the tokens are the names `_x1` and `Ab9`, the number `2` and the name `y`

#### Scenario: A point needs a digit after it
- **WHEN** the lexer reads `3.`, `3.+1` and `.5`
- **THEN** the tokens are number `3` and invalid `.`; number `3`, invalid `.`, plus and number `1`; and invalid `.` and number `5`

#### Scenario: Unknown characters and UTF-8
- **WHEN** the lexer reads `$`, `1 $@ 2`, `!` and the two bytes `\xC3\xA9`
- **THEN** each unknown character is one `invalid` token (`$`, then `$` and `@` between the numbers 1 and 2, `!`), and the two bytes are two `invalid` tokens

#### Scenario: Whitespace
- **WHEN** the lexer reads `\t1\r\n+ \t2\r\n`, the empty string and ` \t\r\n`
- **THEN** the first yields number 1, plus, number 2 and then the end token, and the others yield only the end token

### Requirement: Calculator: Exact number literals
The lexer SHALL build a number literal in `Number`'s own arithmetic and never through `double`: the first `digits10 + 1` significant digits, or `max_digits10` where that is more (17 in both `decimal64_t` and `double`), make one integer mantissa, built one digit at a time with the multiplication by ten and the addition as two statements, and divided once by ten to the number of fraction digits; a later digit of the integer part scales the value up by ten, a later digit of the fraction is dropped; a type without `std::numeric_limits` keeps every digit. So a decimal fraction is exactly that number in a decimal type, and in `double` it is the nearest value for a literal of at most `digits10` significant digits.

#### Scenario: Decimal fractions are exact in decimal
- **WHEN** the lexer reads `1.5`, `0.1`, `12.034` and `0.000000000000001` over `decimal64_t`
- **THEN** the values are `decimal64_t(15, -1)`, `(1, -1)`, `(12034, -3)` and `(1, -15)`, and `0.1` plus `0.2` equals `0.3`

#### Scenario: Fractions are the nearest double in double
- **WHEN** the lexer reads `0.1`, `1.005`, `0.07`, `1.5`, `12.034`, `3.14159` and `0.123` over `double`
- **THEN** each equals the C++ literal of the same text (one division by ten to the number of fraction digits finds `1.005` and `0.07`, which a division by ten per digit misses by one unit in the last place)

### Requirement: Calculator: Literals past the precision and the range
The lexer SHALL give a literal longer than the type's precision a value within one unit in the last place of the nearest `decimal64_t` and within a few units of the nearest `double` (whose digits round as they are read): `1.` followed by any number of zeros is exactly 1; a literal up to the type's largest value, the largest `double` written out in full (309 digits) included, is finite (in a binary type a scaled-up mantissa is computed to twice the precision, `detail::twofold`, with Dekker's split and product, each operation its own statement so that no fused multiply-add changes the rounding); `1` followed by 400 zeros is infinite; a fraction keeps its value down to the type's smallest subnormal and is 0 below it, the division being made in steps of the largest finite power when the power of ten is itself infinite.

#### Scenario: Zeros after a fraction
- **WHEN** the lexer reads `1.` and 400 zeros, and `0.5` and 400 zeros, over `decimal64_t` and over `double`
- **THEN** the values are 1 and 1/2

#### Scenario: Integer digits scale up
- **WHEN** the lexer reads `1` and 400 zeros, and `1` and 30 zeros
- **THEN** the first is infinite in both types and the second equals `1000000000000000 * 1000000000000000`

#### Scenario: Below the smallest value
- **WHEN** the lexer reads `0.` and 400 zeros and `1`
- **THEN** the value is 0 in both types (the smallest positive `double` is about 4.9e-324 and the smallest `decimal64_t` 1e-398)

#### Scenario: Thirty digits
- **WHEN** the lexer reads `123456789012345678901234567890`
- **THEN** over `double` the value lies between `nextafter(nearest, 0)` and `nextafter(nearest, +inf)` of `std::strtod`'s, and over `decimal64_t` between the neighbours of Boost.Decimal's `from_chars`

#### Scenario: Subnormal fractions
- **WHEN** the lexer reads `0.` and 320 zeros and `1`, over `double`, and `0.` and 390 zeros and `1`, over `decimal64_t`
- **THEN** the values are `strtod`'s 1e-321 and `decimal64_t(1, -391)`

#### Scenario: The top of the range
- **WHEN** the lexer reads the largest `double` written out in full (309 digits), `17976931348623146` and 292 zeros, `1` and 309 zeros over `double`, and 16 nines and 369 zeros over `decimal64_t`
- **THEN** the first two are finite and within four units in the last place of `std::numeric_limits<double>::max()` and of `strtod`'s value, the third is infinite, and the last equals `std::numeric_limits<decimal64_t>::max()`

### Requirement: Calculator: The node
The calculator SHALL declare `template <number Number> struct node` with members `Number value{0}`, `bool has_value = true` (false for a name the environment does not hold) and `std::string_view name{}` (the name of a bare variable, a view into the source; empty for every other node). A bare name keeps its name so that `=`, `++` and `--` can write it, and a grouping keeps it too.

#### Scenario: Parentheses keep the name
- **WHEN** `(a) = 1`, `(a)++`, `++((a))` and `((a)) = 10` are evaluated in turn in one environment
- **THEN** the values are 1, 1, 3 and 10, and `a` holds 1, 2, 3 and 10 after them

### Requirement: Calculator: The environment concept
The calculator SHALL declare `template <class E> concept environment_for`, satisfied when `E::number_type` models `number`, `lookup(name) const` (name a `std::string_view`) returns exactly `const number_type*` (`nullptr` when the name is not set) and `assign(name, value)` returns exactly `result<void>`. The name is a view into the source, so an environment that keeps it copies it. `assign` may fail with an error of the environment's own, which the calculator returns unchanged.

#### Scenario: Which types model it
- **WHEN** `environment_for` is checked for `calc::environment<>`, `calc::environment<double>`, `calc::environment<decimal64_t, counting<char>>`, the tests' `fixed_environment<decimal64_t>`, a `read_only_environment<double>`, a type with `lookup` only, and `int`
- **THEN** it holds for the first five and not for the last two

### Requirement: Calculator: The standard environment
The calculator SHALL provide `template <number Number = boost::decimal::decimal64_t, class Allocator = std::allocator<std::pair<const std::string, Number>>> class environment`, modelling `environment_for`, with `number_type = Number` and `allocator_type = Allocator`, a default constructor and `explicit environment(const Allocator&)`, `get_allocator() const noexcept` (a copy of the allocator, rebound back to `Allocator`), `lookup(name) const` (a pointer valid until the name is assigned again or the environment is destroyed) and `assign(name, value)` (inserts a new name or replaces the value of a held one in place, returns success). It stores its names in a `std::map` with `std::less<>`, so a lookup by `std::string_view` builds no key; the map's allocator is `Allocator` rebound to the map's value type and the key's characters (a class `key` holding a `std::basic_string` with `Allocator` rebound to `char`) allocate through it, so any allocator serves, whatever type it is written for, and an allocator without a default constructor compiles.

#### Scenario: The default types
- **WHEN** `static_assert`s check `calc::environment<>::number_type` and `calc::environment<double>::number_type`
- **THEN** they are `decimal64_t` and `double`

#### Scenario: Inserts or replaces
- **WHEN** `v` is looked up, assigned 1, assigned 2, then `w` is assigned 3 (over `decimal64_t` and `double`)
- **THEN** the first lookup is `nullptr`, then `v` holds 1, then 2, and `w` holds 3 with `v` still 2

#### Scenario: The allocator is the user's
- **WHEN** an environment is built with a counting allocator, three names longer than any small-string buffer are assigned, one of them is assigned again, an absent name is looked up and `first_long_name * 2 + 1` is evaluated
- **THEN** each new name allocates (the count grows), and the replacement, the failed lookup and the evaluation allocate nothing; `get_allocator()` is `noexcept` and returns the counting allocator with the same count; and at the end `deallocations == allocations`

### Requirement: Calculator: The grammar and its binding powers
The calculator SHALL provide `template <environment_for Environment> class grammar`, constructed `explicit grammar(Environment& env)` (the environment MUST outlive the grammar), with member types `number_type`, `token_type` (`token<number_type>`) and `node_type` (`node<number_type>`), and these `static constexpr int` powers: `assign_bp` = 10, `compare_bp` = 20, `additive_bp` = 30, `multiplicative_bp` = 40, `unary_bp` = 45, `power_bp` = 50, `postfix_bp` = 70 and `invalid_bp` = `std::numeric_limits<int>::max()`. `lbp` MUST give `=` 10; `== != < <= > >=` 20; binary `+ -` 30; `* / %` 40; `^` 50; `++ --` 70; `invalid` the maximum; and 0 for every other token. Every operator MUST go through the engine's helpers; the calculator adds no parsing of its own.

#### Scenario: Precedence
- **WHEN** `43 + 12 + 2 * ( 44 - 12 ) ^ 4`, `2 ^ 3 ^ 2` and `-2 ^ 2` are evaluated
- **THEN** the values are 2097207, 512 and -4 (a sign binds less tightly than `^`, and `^` groups to the right)

#### Scenario: The grammar models the concept
- **WHEN** `grammar_for<calc::grammar<calc::environment<>>, calc::stream<decimal64_t>>` is checked
- **THEN** it holds

### Requirement: Calculator: The arithmetic operators
The calculator's led SHALL give `+`, `-` and `*` the sum, difference and product, `/` the quotient and `%` the remainder with the sign of the left operand (`fmod`), and a unary `+` or `-` the value or its negation; a binary operator whose operand has no value (a name the environment does not hold) MUST return `error::undefined_variable`, and the result of an operator is a node with a value and no name.

#### Scenario: The reference session
- **WHEN** one `calc::environment<Number>` evaluates `a = 43 + 12 + 2 * ( 44 - 12 ) ^ 4`, `a`, `2 ^ 3 ^ 2`, `-2 ^ 2`, `b = a / 2 - 100`, `b > 1000000`, `b == a / 2 - 100`, `10 % 3`, `i = 0`, `i++`, `i`, `++i`, `--i`, `i--`, `i`, `c = d = 5`, `c + d` and `1.5 * 2`, over `decimal64_t` and over `double`
- **THEN** the values are 2097207, 2097207, 512, -4, 1048503.5, 1, 1, 1, 0, 0, 1, 2, 1, 1, 0, 5, 10 and 3

#### Scenario: A remainder keeps the sign of its left operand
- **WHEN** `7.5 % 2` and `-7 % 3` are evaluated
- **THEN** the values are 3/2 and -1

### Requirement: Calculator: Comparisons
The calculator SHALL give each of `== != < <= > >=` the value 1 when the comparison holds and 0 when it does not, as `Number`, left-associative at power 20.

#### Scenario: Both outcomes
- **WHEN** `1 == 1`, `1 == 2`, `1 != 2`, `1 != 1`, `1 < 2`, `2 < 1`, `2 <= 2`, `3 <= 2`, `2 > 1`, `1 > 2`, `2 >= 2`, `1 >= 2` and `1 + 1 == 2` are evaluated
- **THEN** the values are 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 and 1

### Requirement: Calculator: Assignment
The calculator SHALL make `=` right-associative at power 10: it writes the value on its right into the variable on its left through the environment's `assign` and gives that value (`c = d = 5` sets both). The target MUST be checked before the right operand is read: a left node with no name (a number, a sum, an assignment, a parenthesised number) is `error::invalid_assignment_target`; a right operand with no value is `error::undefined_variable`; an error of `assign` is returned unchanged. A write to a name the environment does not hold defines it.

#### Scenario: Targets
- **WHEN** `(1) = 2`, `(a + 1) = 2` (with `a` holding a value), `(a = 1) = 2` and `5 = 3` are evaluated
- **THEN** each gives `invalid_assignment_target`

#### Scenario: A failed write leaves the name unset
- **WHEN** `x`, `-x`, `x++`, `++x` and `x = y` are evaluated with neither defined, then `x = 2`, `x = x * 3` and `+x`
- **THEN** the first five give `undefined_variable`, `x` is still unset after them, and the last three give 2, 6 and 6

### Requirement: Calculator: Increment and decrement
The calculator SHALL give a prefix `++` or `--` (operand parsed at `unary_bp`) the new value and a postfix one (power 70) the old value, as in C, writing the variable plus or minus one back through `assign`; on a node with no name the result MUST be `error::invalid_assignment_target`, on a name without a value `error::undefined_variable`, and an error of `assign` is returned unchanged.

#### Scenario: As in C
- **WHEN** `i = 0`, `i++`, `i`, `++i`, `--i`, `i--` and `i` are evaluated
- **THEN** the values are 0, 0, 1, 2, 1, 1 and 0

#### Scenario: Not a variable
- **WHEN** `3++`, `(1)++` and `++(1)` are evaluated
- **THEN** each gives `invalid_assignment_target`

### Requirement: Calculator: Division and remainder by zero
The calculator SHALL return `error::division_by_zero` for `/` and `%` when the divisor equals zero, and for `0 ^ n` with a negative integral `n`, where a floating-point type would answer an infinity or a NaN and the next line would compute on with it.

#### Scenario: Zero divisors
- **WHEN** `1 / 0`, `5 % 0`, `1 / (2 - 2)`, `0 / 0` and `5 % 0.0` are evaluated (decimal and double), and then `0 / 5`
- **THEN** the first five give `division_by_zero` and the last gives 0

### Requirement: Calculator: Powers
The calculator SHALL compute `base ^ exponent` by repeated squaring in `Number` when the exponent is integral (equal to its truncation and finite), so that every product is exact while it fits the type's precision and the result has the same bits on every platform; the sign is the base's when the exponent is odd, squares being of the absolute value; a negative integral exponent is `1 / |base|^|n|`, and when that power is 0 or not finite `pow` computes the result instead; `x ^ 0` is 1 for every `x`, `0 ^ 0` included; any other exponent (non-integral, or infinite) goes through `pow`, found as the number type's own. The parity of an integer past the type's precision is even (its successor rounds back to it).

#### Scenario: Exact integral powers
- **WHEN** `(44 - 12) ^ 4`, `2 ^ 53`, `2 ^ -3`, `10 ^ -2`, `(-2) ^ 3`, `(-2) ^ -3`, `1.5 ^ 2`, `3 ^ 1` and `1 ^ (10 ^ 300)` are evaluated
- **THEN** the values are 1048576, 9007199254740992, 1/8, 1/100, -8, -1/8, 9/4, 3 and 1, in `decimal64_t` and in `double`

#### Scenario: Zero and negative bases
- **WHEN** `0 ^ 0`, `x ^ 0` with `x = 0`, `7 ^ 0`, `0 ^ 2`, `0 ^ -1`, `0 ^ -2`, `(-1) ^ 10000000000` and `(-1) ^ 10000000001` are evaluated
- **THEN** the values are 1, 1, 1, 0, `division_by_zero`, `division_by_zero`, 1 and -1

#### Scenario: Past the precision, in decimal and in double
- **WHEN** `2 ^ 60` is evaluated in `double` and in `decimal64_t`, and `2 ^ 1000` in `double`
- **THEN** the values are 1152921504606846976.0, `decimal64_t(1152921504606847, 3)` (and `2 ^ 60 == 1152921504606846976` is 1) and `ldexp(1.0, 1000)`

#### Scenario: Parity of a huge exponent in decimal
- **WHEN** `(-1) ^ (9999999999999999 * 10 ^ 368)`, `(-1) ^ 9999999999999999`, `(-1) ^ 9999999999999998` and `(-2) ^ -1` are evaluated over `decimal64_t`
- **THEN** the values are 1, -1, 1 and `decimal64_t(-5, -1)`

#### Scenario: A negative power past the range is still computed
- **WHEN** `2 ^ -1074` (in `double`), `10 ^ -385` (in `decimal64_t`), `0.1 ^ -400` (in both) and, in `double`, `(-2) ^ -1073` and `(-2) ^ -1074` are evaluated
- **THEN** the first is `std::numeric_limits<double>::denorm_min()`, the second is `decimal64_t(1, -385)`, `0.1 ^ -400` is greater than the type's largest value, and the last two are `-2 * denorm_min` and `denorm_min`

#### Scenario: Squaring, not std::pow, in double
- **WHEN** `1.1 ^ 50` is evaluated over `double`
- **THEN** the value is `0x1.d5903bbcbf5cbp+6`, the same on every lane, five units in the last place from `std::pow`'s `0x1.d5903bbcbf5dp+6`

#### Scenario: Fractional and infinite exponents go through pow
- **WHEN** `2 ^ 0.5` is evaluated, and `0.5 ^ (10 ^ 400)` and `2 ^ (10 ^ 400) == 10 ^ 400`
- **THEN** `2 ^ 0.5` lies strictly between `141421356237309e-14` and `141421356237310e-14` in `decimal64_t` (between sqrt(2) minus and plus 1e-12 in `double`) and equals the number type's `pow(2, 1/2)`; the other two are 0 and 1

### Requirement: Calculator: Errors in reading order
The calculator SHALL return the first error found in reading order: the lexer is lazy, a token's error is found when the token is read, and an operator's own error (an undefined operand, a zero divisor) once the operator has its operands. A character that is no token is an `invalid` token of the highest binding power, whose nud and led give `error::invalid_character`, so the engine reaches it wherever it stands, even after a whole expression: `1 $ 2` is `invalid_character`, not `trailing_input`, and a missing operand before it is `invalid_character`, not `unexpected_end`. A write made before an error stays made; nothing is undone. The nud of the end token is `unexpected_end` and the nud of any other token without one is `unexpected_token`.

#### Scenario: The invalid character wherever it stands
- **WHEN** `1 $ 2`, `$`, `(1 $`, `1 +$`, `1 $`, `a = 1 $` and `2 ^ 3 $` are evaluated
- **THEN** each gives `invalid_character`, and `a` is still unset

#### Scenario: The first error wins, writes stay
- **WHEN** `5 = 3 $` is evaluated; then `i = 1`, `i++ $`, `i++ )`, `x + 1 $` and `1 / 0 $`
- **THEN** the first is `invalid_assignment_target` (`=` checks its target before it reads its right operand); `i++ $` is `invalid_character` and leaves `i` at 2; `i++ )` is `trailing_input` and leaves `i` at 3; `x + 1 $` is `invalid_character` though `x` is undefined; `1 / 0 $` is `invalid_character`

#### Scenario: The ordinary errors
- **WHEN** `x + 1`, `5 = 3`, `3++`, `( 1 + 2`, `1 2`, `1 +`, `1 $ 2` and the empty string are evaluated
- **THEN** the errors are `undefined_variable`, `invalid_assignment_target`, `invalid_assignment_target`, `expected_closing_token`, `trailing_input`, `unexpected_end`, `invalid_character` and `unexpected_end`

### Requirement: Calculator: Evaluate
The calculator SHALL provide `evaluate(std::string_view source, Environment& env, int max_depth = default_max_depth)`, constrained by `environment_for`, returning `result<typename Environment::number_type>`: it builds a `grammar<Environment>` over `env` and a `stream<number_type>` over `source`, calls `pratt::parse` with `max_depth` and returns the value, the parse's first error (in place) or `error::undefined_variable` when the whole expression is a name the environment does not hold. The source needs to live only for the call. A failed evaluation may have assigned variables in `env`; a caller that needs all or nothing evaluates in a copy of the environment and keeps it on success.

#### Scenario: The depth limit is handed to the engine
- **WHEN** `((1))` is evaluated with `max_depth` 2 and with 3
- **THEN** the first is `nesting_too_deep` and the second is 1

#### Scenario: A bare undefined name
- **WHEN** `x` is evaluated in an environment that does not hold it
- **THEN** the result is `undefined_variable`, while `a = 5` (which writes a name the environment does not hold yet) succeeds

### Requirement: Calculator: Deterministic arithmetic
The calculator SHALL keep the multiplication by ten and the addition of a digit in the lexer, every operation of Dekker's split and product, and every step of the integral power, as statements of their own, so that no compiler contracts a multiplication and an addition into a fused multiply-add that would round differently on each target; the same bits on every platform assume each compiler's default for contraction, and a build with `-ffp-contract=fast` or `-ffast-math` may change a `double` result in its last place, which the page says. An integral power is the same bits on every platform where a library's `pow` may round differently from one to the next; the price is that past the precision each product rounds and the relative error grows with the exponent (in `double`, `1.000000001 ^ 1000000000` differs from `std::pow` in its ninth significant digit).

#### Scenario: Squaring is pinned
- **WHEN** `1.1 ^ 50` is evaluated over `double` on native, wasip2, wasip3 and Emscripten
- **THEN** the value is `0x1.d5903bbcbf5cbp+6` on every lane

### Requirement: Calculator: Errors from an environment
An error that an environment's `assign` returns SHALL be returned by `evaluate` unchanged, from `=`, `++` and `--` alike, whatever its category; an environment may refuse a write that the calculator cannot undo.

#### Scenario: A fixed storage that is full
- **WHEN** the tests' `fixed_environment<decimal64_t>` (two slots) evaluates `p = 2`, `q = p * 3`, `p + q == 8`, `p++`, `--q`, `p = q = 1.5`, `r = 1`, `r++` and `p + q`
- **THEN** the values are 2, 6, 1, 2, 5 and 1.5, `r = 1` gives the error `boost::system::errc::not_enough_memory`, `r` stays unset, `r++` gives `undefined_variable`, and `p + q` gives 3

#### Scenario: A read-only environment
- **WHEN** an environment whose names all hold 7 and whose `assign` returns `errc::read_only_file_system` evaluates `n + 1`, `n = 1`, `n++`, `n--`, `++n`, `--n` and `1 + (n = 2)`
- **THEN** the first is 8 and each of the others is that error

#### Scenario: A number an error converts to
- **WHEN** `calc::environment<decimal50>` evaluates `1 / 3 + 2 ^ 100`, `1 / 0`, `5 % 0`, `0 ^ -1` and `x`
- **THEN** the first is `decimal50("1267650600228229401496703205376") + decimal50(1) / decimal50(3)`, and the others are `division_by_zero`, `division_by_zero`, `division_by_zero` and `undefined_variable`

### Requirement: Calculator: Allocation
The calculator's parse SHALL allocate nothing: its lexer is lazy and holds a view, and its tokens and nodes are views and numbers. For a `Number` that allocates nothing (`decimal64_t`, `double`), the environment's allocator is the calculator's only source of memory; a `Number` whose arithmetic allocates (Boost.Multiprecision's `mpfr_float` with `et_off`) allocates through its own allocator. A test or an example MUST NOT state how many allocations a standard library makes: it compares what a step adds, and that every allocation is given back.

#### Scenario: A fixed environment allocates nothing
- **WHEN** `test/allocation_test.cpp`, whose program replaces the global `operator new` (and the over-aligned forms) to count calls, evaluates its twenty lines over a `fixed_environment<decimal64_t>`
- **THEN** 16 lines give a value and the count of `operator new` calls added is 0

#### Scenario: The standard environment allocates only through its allocator
- **WHEN** the same twenty lines run over `calc::environment<decimal64_t, counting<char>>`
- **THEN** 17 lines give a value, the allocator's count is greater than 0, the global `operator new` calls added equal the allocator's allocations, and its deallocations equal its allocations

#### Scenario: An over-aligned allocation is counted
- **WHEN** the test calls `::operator new(3 * sizeof(over_aligned), std::align_val_t{alignof(over_aligned)})`
- **THEN** the count grows by 1, the pointer is aligned, and `::operator delete(memory, alignment)` gives it back

### Requirement: Limits: The measured default depth
`default_max_depth` (64) SHALL be the largest power of two at most half of the deepest parse that wasm's default stack of 64 KiB holds in a debug build (`-O0 -fno-inline`), in the shape that costs the most of the calculator's grammar and of openui-lang's; the measurement (wasi-sdk 34, clang 23.1.0, wasmtime 47.0.3, over `calc::environment<>`) is a markdown table in a comment above the constant and in the "Limits" chapter, which repeats it verbatim. In it, a grouping of N levels runs to 232 on wasip2 and wasip3 at `-O0` (282 bytes a level), 450 on wasip2 at `-O2` (146) and 16854 natively at `-O0` with an 8 MiB stack (497); a chain of signs to 263 (249), 450 (146) and 18016 (465); a chain of `^` to 240 (273), 405 (162) and 12741 (657). Over every shape the deepest debug parse that wasm's stack holds is at least 203 calls (openui-lang's call shape on wasip2), the calculator's least is 214 (a grouping with a `pow` inside), half of 203 is 101 and the largest power of two at most 101 is 64. In a debug build on wasm a level of grouping costs 272 bytes, a level of `^` 256 and a level of sign 240, plus at most about 7.5 KiB at any depth. A change to the frames of `expression`, the helpers or the grammars may move the default: measure again on wasip2, wasip3 and Emscripten and update the table, the value and the chapter together.

#### Scenario: The default depth is evaluated, one more is refused
- **WHEN** `test/depth_test.cpp` evaluates over `fixed_environment` for `decimal64_t` and `double` a grouping of 63 pairs, a chain of 63 signs (`- - ... - 1`, spaced since `--` is a decrement) and a chain of 63 `^` (`2 ^ 2 ^ ... ^ 2`), each `default_max_depth` calls deep; and then 64 of each
- **THEN** the first three evaluate, and the others are `nesting_too_deep`

#### Scenario: The canary
- **WHEN** the same test, after writing to standard error that a trap means `default_max_depth` must be measured again, evaluates the three shapes at 127 levels with `max_depth` 128 (twice the default)
- **THEN** each evaluates; on wasip2 and wasip3 in a debug build with the default stack a failure is a stack overflow trap, which means the 2x margin is gone

#### Scenario: Ten thousand parentheses
- **WHEN** a line of ten thousand `(` is evaluated
- **THEN** the result is `nesting_too_deep`, not a stack overflow

#### Scenario: Raising the limit
- **WHEN** the page's example evaluates 64 pairs of parentheses with the default and with `max_depth` 128
- **THEN** `pratt_depth` prints `64 pairs: error: expression nested too deeply` and `64 pairs, max_depth 128: 1`; and the chapter says a program that raises the limit raises its stack with it (`-Wl,-z,stack-size=` on wasm, the thread's stack natively)

### Requirement: Examples: The guide's programs
The library SHALL ship these programs under `example/`, each declared with `webcpp.example`, built and run on every target and compared with the `.expected` beside it (a program that returns anything but 0 fails too): `pratt_first_grammar`, `pratt_helpers`, `pratt_calculator`, `pratt_double`, `pratt_environment`, `pratt_allocator`, `pratt_depth` and `pratt_testing`. Their tagged regions (`tag::...[]`) are what the page includes, and their `.expected` files hold the outputs listed below.

#### Scenario: The helpers
- **WHEN** `pratt_helpers` parses `1-2-3`, `2^3^2`, `1+2*3`, `-2^2`, `-3!`, `2*-3`, `(1+2)*3`, `(1+2`, `1+)` and `()`
- **THEN** it prints `(- (- 1 2) 3)`, `(^ 2 (^ 3 2))`, `(+ 1 (* 2 3))`, `(- (^ 2 2))`, `(- (! 3))`, `(* 2 (- 3))`, `(* (+ 1 2) 3)`, `error: expected closing token`, `error: unexpected token` and `error: unexpected token`, each after `<text> -> `

#### Scenario: The calculator in decimal
- **WHEN** `pratt_calculator` evaluates its lines in one environment
- **THEN** it prints, among others, `43 + 12 + 2 * (44 - 12) ^ 4 -> 2097207`, `0.1 + 0.2 -> 0.3`, `0.1 + 0.2 == 0.3 -> 1`, `2 ^ -3 -> 0.125`, `1 / 3 -> 0.3333333333333333`, `price * quantity -> 59.97`, `quantity++ -> 3`, `quantity -> 4`, `a + b -> 4`, `1 / 0 -> error: division by zero`, `i++ $ -> error: invalid character`, `i -> 2` and `x + 1 $ -> error: invalid character`

#### Scenario: The calculator in double
- **WHEN** `pratt_double` evaluates over `calc::environment<double>`
- **THEN** it prints `0.1 + 0.2 -> 0.30000000000000004`, `0.1 + 0.2 == 0.3 -> 0`, `2 ^ 60 -> 1152921504606846976`, `2.25 ^ 0.5 -> 1.5` and `1 / 0 -> error: division by zero`

#### Scenario: An environment of the program's own
- **WHEN** `pratt_environment`'s two-variable fixed storage (names up to 8 characters, `number_type = double`, `static_assert(calc::environment_for<two_variables>)`) evaluates `width = 3`, `height = 4`, `width * height`, `width = 5`, `width * height`, `depth = 2` and `depth`
- **THEN** it prints `3`, `4`, `12`, `5`, `20`, `refused: the environment is full` (the `errc::not_enough_memory` of `assign`, compared with `value.error() == boost::system::errc::not_enough_memory`) and `error: undefined variable`

#### Scenario: A counting allocator
- **WHEN** `pratt_allocator` evaluates `x = 6`, `y = 7`, `x * y + 1`, `x = 8` and `x * y` over an environment with its counting allocator
- **THEN** it prints `allocated` for the first two, `allocated nothing` for the next three, and `environment destroyed -> deallocations == allocations: yes`

#### Scenario: A test of a grammar
- **WHEN** `pratt_testing` runs its three cases (`a = b = 1` as `(= a (= b 1))`, `Stack(1, 2)(3)` as `(call (call Stack 1 2) 3)`, `a = Text(gap: 4)` as `(= a (call Text (: gap 4)))`; the four errors; `((x))` within depth 3 and `(((x)))` beyond it) with Boost.Core's `lightweight_test`
- **THEN** it prints nothing on its standard output and returns `boost::report_errors()`, 0

### Requirement: Examples: The openui-lang micro-subset
The library SHALL ship `example/openui/grammar.hpp`, in namespace `webcpp::example::openui`, a micro-subset of openui-lang on the engine: identifiers (a letter or `_`, then letters, digits and `_`), integers, double-quoted strings without escapes, and `( ) , : =`. Its `kind` has `identifier`, `integer`, `string`, `lparen`, `rparen`, `comma`, `colon`, `assign`, `invalid`, `unterminated` (a `"` the line does not close, running to the end of the line) and `end`; its lazy `stream` separates tokens by space, tab and carriage return; its `grammar` has the node `std::string`, the S-expression of the parse, and the powers `assign_bp` 10, `pair_bp` 20, `call_bp` 90 and `invalid_bp` the maximum (also for `unterminated`). The nud of an identifier, integer or string is its text (a string keeps its quotes); of `(` a `grouping`; of `invalid` `invalid_character`; of `unterminated` and `end` `unexpected_end`; of anything else `unexpected_token`. The led of `=` is `infix_right`, of `:` `infix_left`, of `(` a call, and the led of `invalid` and `unterminated` as their nuds. A call reads its arguments in a loop inside the led, each a whole expression `p.expression(0)`, separated by `,` and closed by `)`, through `peek` and `advance` only, so every token goes through the engine and nothing descends around it. The node allocates (it is a string); it is the examples' output format, and the parser of openui-lang to come defines a tree of its own.

#### Scenario: The reference session parses as recorded
- **WHEN** `test/openui_test.cpp` parses `root = Stack()`, `root = Stack(spacing: 8, Button(label: "Pay"), Text("Total"))`, `view = Row(Button(label: "ok"), gap: 4)`, `Stack(1, 2)(3)` and `(x)`
- **THEN** the nodes are `(= root (call Stack))`, `(= root (call Stack (: spacing 8) (call Button (: label "Pay")) (call Text "Total")))`, `(= view (call Row (call Button (: label "ok")) (: gap 4)))`, `(call (call Stack 1 2) 3)` and `x`

#### Scenario: The recorded failures
- **WHEN** it parses `Stack(1`, `= 3`, `root = Stack(label: "unterminated)` and `root @ 3`
- **THEN** the errors are `expected_closing_token`, `unexpected_token`, `unexpected_end` and `invalid_character`

#### Scenario: Shapes of `(`, `=` and `:`
- **WHEN** it parses `Stack()`, `Text("a, b: c")`, `Text("a(b")`, `(a)(b)`, `a = b = c`, `a = b: c`, `a: b = c` and `Stack(a,\tb)\r`
- **THEN** the nodes are `(call Stack)`, `(call Text "a, b: c")`, `(call Text "a(b")`, `(call a b)`, `(= a (= b c))`, `(= a (: b c))`, `(= (: a b) c)` and `(call Stack a b)`

#### Scenario: Every other misplaced token
- **WHEN** it parses the empty string, `Stack(1,`, `Stack(1 2)`, `(a`, `a)`, `a b`, `Stack(,)`, `a: `, `@`, `Stack(1 @`, `Stack(@`, `a = @`, `"`, `Text "open` and `Text("open, 1))`
- **THEN** the errors are `unexpected_end`, `unexpected_end`, `expected_closing_token`, `expected_closing_token`, `trailing_input`, `trailing_input`, `unexpected_token`, `unexpected_end`, `invalid_character` four times, and `unexpected_end` three times

### Requirement: Examples: The REPL loop
The library SHALL ship `example/repl.hpp` with `webcpp::example::repl(std::istream& in, std::ostream& out, ProcessLine process_line)`, returning 0: it reads lines with `std::getline`, drops one trailing carriage return, ends at the line `exit`, exactly (nothing after it is read), skips a line that is empty or holds only spaces or tabs without calling `process_line`, hands every other line as a `std::string_view` and `out` to `process_line`, and ends at the end of `in`. The line `exit ` with a trailing space is an ordinary line, a name to process rather than the end.

#### Scenario: The line shapes
- **WHEN** a session holds a line ending in `\r\n`, a line of four spaces, a line of tab, space and tab, the line `exit ` and then `exit` and `ignored`
- **THEN** the first is processed without its `\r`, the two blank lines are skipped, `exit ` is processed, and the loop returns 0 at `exit` before `ignored`

### Requirement: Examples: The openui-lang REPL
The library SHALL ship `example/openui/repl.hpp` (`openui::repl(in, out)`, parsing each line with `pratt::parse(grammar{}, stream(line))` and writing its S-expression, or `error: ` and the error's message), and the programs `openui_repl` and `openui_repl_eof`, whose `.input` files the build feeds to their standard input and whose outputs are compared with their `.expected`.

#### Scenario: The session
- **WHEN** `openui_repl` reads `openui_repl.input`, whose lines include `root = Stack()`, `Stack(1`, `= 3`, `root @ 3`, `Text("crlf")` ending in `\r\n`, two blank lines, `exit ` with a trailing space, `exit` and `ignored`
- **THEN** it prints `(= root (call Stack))`, the other trees, `error: expected closing token`, `error: unexpected token`, `error: unexpected end of input`, `error: invalid character`, `(call Text "crlf")` and `exit` (the name, not the end), and stops before `ignored`

#### Scenario: An input without a newline and without exit
- **WHEN** `openui_repl_eof` reads `a = 1`, an empty line and `Stack(a)` with `\r\n` line ends and no line feed after the last
- **THEN** it prints `(= a 1)` and `(call Stack a)` and returns 0

### Requirement: Examples: The calculator REPL
The library SHALL ship `calculator_repl`, which keeps one `calc::environment<>` for the session, writes each value in Boost.Decimal's shortest round-tripping form (`boost::decimal::to_chars`: `2097207`, `1048503.5`, with its sign for a negative value, or `error: ` and the reason when it does not fit its 48-byte buffer) and each failure as `error: ` and its message, through the same loop. Line 30 of `calculator_repl.input` is ten thousand `(` and a `1`, which the page leaves out (`lines=1..29;31..33`).

#### Scenario: The session
- **WHEN** `calculator_repl` reads `calculator_repl.input`: the reference session's lines, `x + 1`, `5 = 3`, `3++`, `( 1 + 2`, `1 2`, `1 +`, `1 $ 2`, a blank line, `a = 2`, `a * 3`, `1 / 0`, the line of ten thousand `(`, `exit `, `exit` and `ignored`
- **THEN** it prints the 18 values `2097207`, `2097207`, `512`, `-4`, `1048503.5`, `1`, `1`, `1`, `0`, `0`, `1`, `2`, `1`, `1`, `0`, `5`, `10`, `3`, then `error: undefined variable`, `error: invalid assignment target`, `error: invalid assignment target`, `error: expected closing token`, `error: trailing input after expression`, `error: unexpected end of input`, `error: invalid character`, `2`, `6`, `error: division by zero`, `error: expression nested too deeply` and `error: undefined variable` (for the name `exit`)

### Requirement: Tests: The suites and their declarations
`test/Jamfile` SHALL require `<library>/webcpp/pratt//pratt`, declare `webcpp.targets native emscripten wasip2 wasip3`, `webcpp.headers-alone pratt : ../include`, the three compile-only targets `config`, `config_developer_defined` and `config_all_off` (the same `config_test.cpp` with different defines), and these `webcpp.run` programs, each a `lightweight_test` program that returns `boost::report_errors()`: `errors`, `engine`, `sexpr`, `lexer`, `calculator`, `allocation`, `depth` and `openui` (with `<include>../example`, so that the test shares the grammar with the example). `test/require.hpp` provides `webcpp::test::require(bool held)`, which ends the program with `boost::report_errors()` when a check a helper cannot go on without has failed (a helper cannot return from its caller, and a test may be built without exceptions). Each case is a function named after the rule it checks.

#### Scenario: Each suite
- **WHEN** the eight programs are compiled and run
- **THEN** each prints `No errors detected.` on standard error and returns 0 (`depth` also prints its two canary lines there first)

### Requirement: Tests: The tests' own grammars and environments
The tests SHALL include `test/sexpr.hpp` (a second grammar with token and node types of its own: tokens are the input's characters, `'\0'` past the end, and nodes are S-expressions; digits, `( )`, prefix `-` at 40, left `+ - *` at 10, 10, 20 and right `^` at 30, shown to reach the engine unchanged), `test/environments.hpp` (`fixed_environment<Number>`, two names of at most 64 characters in fixed storage that allocates nothing and refuses a third with `errc::not_enough_memory`, and `counting<T>`, an allocator that counts into an `allocation_count` and takes its memory from the global `operator new`, the aligned form for an over-aligned `T`, and compares equal when its counts are the same), and the engine test's `table` grammar (binding powers from a table, an operator being infix-left, infix-right or postfix).

#### Scenario: The sexpr grammar
- **WHEN** `test/sexpr_test.cpp` parses `1+2*3`, `2^3^2`, `-(1+2)*3` and `1-2-3`
- **THEN** the nodes are `(+ 1 (* 2 3))`, `(^ 2 (^ 3 2))`, `(* (neg (+ 1 2)) 3)` and `(- (- 1 2) 3)`

### Requirement: Tests: Lint configuration of tests and examples
`test/.clang-tidy` and `example/.clang-tidy` SHALL inherit the superproject's configuration (`InheritParentConfig: true`) and switch off one option, `bugprone-exception-escape.CheckMain`, because a test's or an example's `main` lets what a failing case or the standard library may throw end the process uncaught, as Boost's own examples do. The check stays on for everything else. `.gitattributes` MUST keep `*.input` byte for byte (`-text`), because an input holds `\r\n` on purpose, and `.gitignore` holds `doc/html/`.

#### Scenario: The inputs keep their line ends
- **WHEN** `example/openui_repl.input` is checked out
- **THEN** its line `Text("crlf")` still ends in `\r\n`

### Requirement: Docs: The page and its chapters
The library SHALL ship the page `doc/pratt.adoc` (title "pratt: Pratt parsing for C++"), built by `webcpp.doc pratt : pratt.adoc` and `webcpp.reference pratt` in `doc/Jamfile`, which includes these chapters in order: "Introduction" (nud, led and lbp; a first grammar; requirements, among them Boost 1.92 or newer, webcpp's configuration, exceptions the program's choice, RTTI never restricted and nothing else), "Writing a grammar" (the token stream, the grammar, the node, the six helpers and parsing), "The calculator" (the language and its operators with their powers, exactness and the number type, the environment, errors in reading order), "The openui-lang micro-subset and the REPLs", "Errors" (the table of the nine values), "Limits" (the depth limit, the stack each level costs, raising the limit), "Allocation" and "Testing" (testing a grammar, how pratt is tested, targets, the test matrix); then the "Reference" generated by MrDocs from the Doc Comments, and the changelog appendix. Every program it shows was compiled and run, natively and for wasm32-wasip2 and wasm32-wasip3, and printed the output shown; every listing includes a compiled example by tag, and every count is an attribute (`{n-tests-native}`, `{n-examples}`, ...).

#### Scenario: The page builds
- **WHEN** `b2 libs/pratt/doc` is run
- **THEN** the page is built into `doc/html/index.html` with doc-check, and the reference lists each name of `webcpp::pratt` and `webcpp::pratt::calc`

### Requirement: Docs: Doc Comments and the reference
Every public symbol of `webcpp::pratt` and `webcpp::pratt::calc` SHALL have a Doc Comment (`/** ... */`) with a one-sentence brief, `@param`, `@tparam`, `@return`, `@pre`, `@note` and `@see` where they apply, and every `detail` symbol at least a brief; each `@see "<title>"` MUST name a top-level chapter of the page, so renaming a chapter renames its `@see` lines in the same commit. MrDocs builds the reference strict, and `doc/mrdocs.yml` sets `show-enum-constants: true` so that each enumerator of `error` has a section of its own with its value and its whole Doc Comment.

#### Scenario: A chapter named by `@see`
- **WHEN** the `@see` lines of the headers are collected
- **THEN** each is one of "Introduction", "Writing a grammar", "The calculator", "Errors", "Limits", "Allocation" and "Testing"

### Requirement: Docs: README, AGENTS.md and the changelog
The repository SHALL contain `README.md` (what pratt is, the minimal example `example/pratt_first_grammar.cpp[tag=program]` copied with its output, which doc-check compares at every doc build, how to build and test inside the webcpp superproject with `b2 libs/pratt/test libs/pratt/example`, with `toolset=emscripten` and with `toolset=clang-wasip2 testing.launcher=wasmtime` / `clang-wasip3`, and the link to the page), `AGENTS.md` (what is specific to pratt: the two layers, the fixed errors, every recursion through `expression()`, the measured default, the first error in reading order, exactness, allocation, no exceptions in a header, `config.hpp` first, openui-lang named alone) and a changelog appendix `doc/changelog.adoc` whose "Unreleased" lines say what a user sees: emscripten added as a target; `calc::environment` compiling with a stateful allocator that has no default constructor; the `WEBCPP_PRATT_NO_EXCEPTIONS` macro, defined automatically or by the developer; `config.hpp` including `<webcpp/config.hpp>`; `WEBCPP_NO_EXCEPTIONS` disabling pratt's exceptions with every webcpp library's; and the changelog itself.

#### Scenario: The README's example is the program
- **WHEN** doc-check compares the README's code block with `example/pratt_first_grammar.cpp[tag=program]` and its output block with `pratt_first_grammar.expected`
- **THEN** they are equal, or the doc build fails

### Requirement: CI: The superproject's library workflow
`.github/workflows/ci.yml` SHALL be named "CI", run on push to `main`, on pull requests and on manual dispatch with `permissions: contents: read`, a concurrency group per workflow and ref (a run on `main` never cancelled by the next, a pull request's older run replaced by its newer push), and a single job that uses `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: pratt`, which builds the commit inside a checkout of webcpp on every lane pratt declares, with its page, the lint and the test matrix.

#### Scenario: The job
- **WHEN** `.github/workflows/ci.yml` is read
- **THEN** its only job `ci` is named "CI" and uses the reusable workflow with `library: pratt`
