---
title: "pratt: the owner's draft prompt (reference appendices)"
tags: [webcpp, pratt, draft, reference]
status: active
created: 2026-10-08
---

# pratt: the owner's draft prompt

The owner's draft prompt for webcpp's `pratt` library, kept as given, with
one change: the sentence that named the project the first consumer belongs
to now says only "a view-description language", because no webcpp file may
name that project (`[redacted]` below). The binding design is
`PRATT_DESIGN.md`; where it and this draft differ, the design wins. The
appendices are the verified reference implementation the design cites.

---

# Task: create `pratt`, webcpp's generic Pratt parser library

## Context

You are working in the webcpp superproject at `~/dev/webcpp`.
Read `AGENTS.md` at the root first and obey it in full: layout, naming,
fidelity, C++ rules, Doc Comments, docs, tests, lanes, process, commits.
Where this prompt and `AGENTS.md` disagree, `AGENTS.md` wins. `libs/xactor`
is the model for a library of webcpp's own - use its tree (headers, tests,
examples, doc/) as the template for everything this prompt does not
specify. This is a new library of webcpp's own (not a port), so
`"port-of"` is `null` in `meta/libraries.json`. Follow the new-library
checklist in AGENTS.md chapter 2; the GitHub repository and the submodule
registration are the owner's steps, so start it as a local repository at
`libs/pratt` (`git init`), which the lint and the build already treat as a
library.

This library's first planned real consumer is a parser for **openui-lang**,
a view-description language [redacted] (sample block: `root = Stack()`).
The micro-subset grammar of requirement 7 and the REPL of Appendix D are
the seed for that future parser; design them as such.

## What the library is

A header-only C++20 library that provides the Pratt parsing algorithm
(top-down operator precedence) as a reusable, maximally generic engine,
plus one batteries-included component built on it: a ready-to-use
calculator. Parsing is a runtime activity (the engine consumes any token
stream at runtime - a REPL is a natural client); what is fixed at compile
time is each grammar's shape, through concepts. The person implementing a
concrete parser supplies the whole concrete implementation: token type,
token stream, AST/value type, and the grammar. The library supplies the
algorithm, and it must be structured so that the engine is impossible to
use without going through nud/led categories.

## The reference implementation

Appendices A-D below are a complete, working reference implementation of
the intended architecture. It was compiled with
`clang++ -std=c++20 -Wall -Wextra -Werror` against Boost; every acceptance
check in Appendix C passed, and the REPL of Appendix D was exercised end
to end (its session transcript is reproduced there). Treat it all as the
architectural contract, not as files to paste blindly:

- **Keep as-is (the architecture):** the `token_stream` and `grammar_for`
  concepts; the generic `parser::expression()` loop that knows no concrete
  operator and only calls `nud`/`led`/`lbp`; the category helpers
  (`literal`, `prefix`, `infix_left`, `infix_right`, `postfix`,
  `grouping`) expressed purely in terms of `expression()` recursion; the
  `error` enumerators with their exact fixed values; the calculator's
  semantics, including the binding-power table and the lvalue-tracking
  `node` (name kept on bare variable references so `=`, `++`, `--` know
  their target and undefined variables only error when read); the openui
  grammar's shape, with `(` holding both denotations and the argument
  loop inside its led.
- **Generalize (required deviation):** the reference fixes the
  calculator's number type to `double`. The final library must not - see
  requirement 5. Everything Appendices B and C do with `double` (token
  values, environment, node, arithmetic, the test expectations and their
  printf formatting) is to be rewritten in terms of the `Number` template
  parameter.
- **Adapt to webcpp (the packaging):** split into
  `include/webcpp/pratt/parser.hpp` and
  `include/webcpp/pratt/calculator.hpp`; the openui grammar and REPL into
  the example tree per requirements 7-8; license notices and Doc Comments
  per requirement 9; tests shaped per AGENTS.md chapter 9 with xactor's
  test tree as the model (Appendix C is the acceptance content, not the
  test framework); lint, `.clang-format`, commit conventions per
  chapters 10-11.
- **Improve where the rulebook demands it,** but any behavioral deviation
  from the reference beyond requirement 5 must be called out and
  justified in the final report.

## Hard requirements

1. **nud/led is the architecture, not a convention.** The core expression
   loop must know nothing about any concrete operator (see
   `parser::expression()` in Appendix A). If a user can route operator
   behavior around nud/led, the design is wrong.

2. **Fully generic via templates and concepts.** The engine is constrained
   by the `token_stream` and `grammar_for` concepts of Appendix A; the
   node type is the grammar's, deduced through `Grammar::node_type`, never
   fixed by the library. No inheritance-based customization, no
   `std::function` in the core path, no type erasure. Runtime-extensible
   grammars remain possible by construction (a grammar is any type
   modeling the concept, so one whose nud/led/lbp consult runtime tables
   is legal); do not ship one, but do not preclude it either.

3. **Category helpers as a layer on top**, exactly as in Appendix A: each
   helper is expressed through `expression()` recursion, proving the
   layering. Right associativity is the `bp - 1` handed to the recursive
   call, canonical Pratt.

4. **A ready-to-use calculator ships with the library**
   (`webcpp/pratt/calculator.hpp`): tokenizer, grammar and one-call
   `evaluate()` included, zero implementation work for the user, and
   itself built on the engine and the helpers of requirements 1-3 (it is
   the showcase, so it must not bypass them). Behavior as in Appendix B:
   numbers and variables, `+ - * / % ^` (right-assoc `^`), parentheses,
   unary `+/-` binding below `^` (so `-2 ^ 2 == -4`), comparisons
   `== != < <= > >=` evaluating to `Number(1)`/`Number(0)`, right-assoc
   `=` writing into the environment, prefix/postfix `++`/`--` with C
   semantics restricted to variables, errors with the exact enumerator.

5. **No binary floating-point error: the number type is a template
   parameter, decimal by default.** The whole `calc` component is
   templated on `Number`, defaulting to `boost::decimal::decimal64_t`
   (Boost.Decimal is part of Boost, so it needs no owner decision and no
   new `build.jam` dependency):
   - `token<Number>`, `tokenize<Number>`, `environment<Number>`
     (`std::map<std::string, Number, std::less<>>`), `node<Number>`,
     `grammar<Number>` and
     `template <class Number = boost::decimal::decimal64_t> result<Number>
     evaluate(std::string_view, environment<Number>&)`.
   - Define and document a concept stating what `Number` must provide
     (construction from `int`, arithmetic operators, comparisons, and
     ADL-reachable `pow` and `fmod`), and constrain the `calc` templates
     with it.
   - `pow` and `fmod` are found by the two-step ADL pattern
     (`using std::pow;` then unqualified `pow(a, b)`), so
     `boost::decimal`'s overloads serve the default and `std::` serves
     builtin types.
   - **Exactness is part of the contract:** with the default decimal
     type, every acceptance value in Appendix C holds exactly, including
     `a = 43 + 12 + 2 * ( 44 - 12 ) ^ 4 == 2097207` and
     `b = a / 2 - 100 == 1048503.5`. If the generic `pow` is not exact
     for integral exponents, `^` must detect an integral right operand
     and take an exact integer-power path; write the test first, it is
     the point of this requirement.
   - The tokenizer builds the value digit by digit in `Number`
     arithmetic (as the reference does in `double`), never through a
     binary `double` intermediate.
   - The tests instantiate the calculator with a second number type
     (`double`) as well, proving the template is real; the acceptance
     values that are exact in binary too must pass there.

6. **webcpp C++ rules apply in full** (AGENTS.md chapter 6): header-only
   C++20, errors as values via `boost::system::result<T>` with the
   `webcpp.pratt` error category and the fixed enumerator values of
   Appendix A, no `throw`/`try`/`catch` reachable from headers, no global
   or static mutable state, determinism, string data as
   `std::string_view`, guards first in every function.

7. **Prove genericity with two independent grammars** (neither is a
   shipped header):
   - **The sexpr grammar of Appendix C** (verified): token type `char`,
     node type `std::string`, engine and helpers unchanged; test-only.
   - **The openui-lang micro-subset of Appendix D** (verified), the seed
     for the future openui-lang parser. It lives in the example tree
     (shared by the REPL of requirement 8 and by the tests). Its shape:
     tokens are identifiers (`[A-Za-z_][A-Za-z0-9_]*`), non-negative
     integers, double-quoted strings without escapes, and the punctuation
     `( ) , : =`; `=` is right-associative infix assignment; `(` has
     **both** denotations - nud is grouping, led is a call with the
     highest binding power, its argument loop living inside that led
     (each argument is `expression(0)`; `,` has lbp 0, so it naturally
     terminates one; `)` closes the call); `:` is a low-binding-power
     infix pair for named properties. The node type is the grammar's own
     rendering of the AST. Acceptance, verified:
     `root = Stack(spacing: 8, Button(label: "Pay"))` parses to
     `(= root (call Stack (: spacing 8) (call Button (: label "Pay"))))`,
     and calls chain (`Stack(1, 2)(3)` is
     `(call (call Stack 1 2) 3)`). Errors reuse the library's
     enumerators. It must stay honest Pratt: no hand-rolled recursive
     descent around the engine.

8. **Two REPL example programs ship with the library**, under the example
   tree (xactor's examples as the model for naming and build wiring):
   - **The calculator REPL**: a read-eval-print loop over
     `calc::evaluate()` with the default decimal number type. One
     `environment` lives for the whole session, so variables persist
     across lines (`a = 2` then `a * 3` works). Each non-empty line is
     evaluated; the result is printed, or `error: ` plus the error's
     category message. EOF or the line `exit` ends the program with
     status 0.
   - **The openui REPL of Appendix D** (verified): the same loop over the
     openui micro-subset grammar, printing each line's AST rendering or
     `error: ` plus the message.
   Interactive I/O (stdin/stdout) is confined to these example programs -
   never to library headers; if the lint's world rule covers examples,
   annotate per the `lint-world:` escape with the reason (a REPL's
   purpose is the terminal).

9. **Documented for MrDocs, webcpp's way** (AGENTS.md chapter 7, xactor's
   doc/ as the model): every public symbol of `webcpp::pratt` - functions,
   classes, enums and enumerators, aliases, concepts, variables, data
   members - carries a `/** ... */` Doc Comment with a one-sentence brief,
   `@param`/`@tparam`/`@return` where they apply, none of the forbidden
   tags, and no hand-written overload anchors. Ship the `doc/` tree:
   `doc/Jamfile` declaring `webcpp.reference pratt ;`, the `pratt.adoc`
   page with `include::{reference}[leveloffset=+1]`, and a `doc/mrdocs.yml`
   with presentation keys only. The reference must build strict -
   `b2 libs/pratt/doc//reference` under MrDocs 2026.9.29 with
   `warn-as-error: true` - and `tools/doc/doc_comments.py` must pass.

10. **Tests before implementation.** Write the failing tests first -
    engine behavior (precedence, associativity, nud/led dispatch, every
    error enumerator), the calculator's full surface including Appendix
    C's acceptance values under the default decimal type and under
    `double`, the integral-exponent exactness of `^`, the sexpr grammar,
    and the openui micro-subset including Appendix D's acceptance cases -
    then bring in the reference implementation, generalized per
    requirement 5, to make them green, following webcpp's test and lane
    conventions from AGENTS.md chapter 9. The REPL examples are covered
    by driving them end to end (lines in, output out, exit status), the
    way webcpp proves examples work; Appendix D's transcript is the
    expected content for the openui one.

11. **Deliverables**, per the AGENTS.md checklist: `libs/pratt` as a local
    git repository with `include/webcpp/pratt/...` headers, tests, the
    two REPL examples, README, `doc/`, `build.jam`, the
    `meta/libraries.json` entry in the superproject (`"port-of": null`,
    one-sentence description, `"cxxstd": "20"`), BSL-1.0 notices, English
    everywhere. Do not create remote repositories, do not push, do not
    `git add -A` in the superproject.

---

## Appendix A - engine (reference, verified; becomes `include/webcpp/pratt/parser.hpp`)

The engine is already number-agnostic; it needs Doc Comments and webcpp
packaging, nothing from requirement 5.

```cpp
// Copyright (c) 2026 WebCpp.org
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef WEBCPP_PRATT_PARSER_HPP
#define WEBCPP_PRATT_PARSER_HPP

#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>

#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

namespace webcpp::pratt {

template <class T>
using result = boost::system::result<T>;

// Fixed enumerator values, never reused (AGENTS.md chapter 6).
enum class error
{
    unexpected_token = 1,
    unexpected_end = 2,
    expected_closing_token = 3,
    trailing_input = 4,
    invalid_assignment_target = 5,
    undefined_variable = 6,
    invalid_character = 7,
};

namespace detail {

struct error_category final : boost::system::error_category
{
    const char* name() const noexcept override { return "webcpp.pratt"; }

    std::string message(int ev) const override
    {
        switch (static_cast<error>(ev))
        {
        case error::unexpected_token: return "unexpected token";
        case error::unexpected_end: return "unexpected end of input";
        case error::expected_closing_token: return "expected closing token";
        case error::trailing_input: return "trailing input after expression";
        case error::invalid_assignment_target: return "invalid assignment target";
        case error::undefined_variable: return "undefined variable";
        case error::invalid_character: return "invalid character";
        }
        return "unknown webcpp.pratt error";
    }
};

} // namespace detail

inline const boost::system::error_category& pratt_category()
{
    static const detail::error_category category;
    return category;
}

inline boost::system::error_code make_error_code(error e)
{
    return {static_cast<int>(e), pratt_category()};
}

// A source of user-defined tokens. peek() must stay valid at end of input
// (an end token, or any token whose lbp is 0 and whose nud errors).
template <class S>
concept token_stream = requires(S s, const S cs) {
    typename S::token_type;
    { cs.peek() } -> std::convertible_to<typename S::token_type>;
    { s.advance() } -> std::same_as<typename S::token_type>;
    { cs.at_end() } -> std::convertible_to<bool>;
};

template <class Grammar, class Stream>
class parser;

// A grammar exposes, for every token, exactly the three Pratt denotations:
// nud (the token at prefix position), led (the token with a left operand)
// and lbp (how tightly it binds as infix/postfix). The node type is the
// grammar's, not the library's.
template <class G, class S>
concept grammar_for =
    token_stream<S> &&
    requires(const G g, parser<G, S>& p, const typename S::token_type& t,
             typename G::node_type n) {
        typename G::node_type;
        { g.lbp(t) } -> std::convertible_to<int>;
        { g.nud(p, t) } -> std::same_as<result<typename G::node_type>>;
        { g.led(p, t, std::move(n)) } -> std::same_as<result<typename G::node_type>>;
    };

template <class Grammar, class Stream>
class parser
{
public:
    using token_type = typename Stream::token_type;
    using node_type = typename Grammar::node_type;

    parser(const Grammar& grammar, Stream stream)
        : grammar_(&grammar), stream_(std::move(stream))
    {
    }

    // The canonical Pratt loop. It knows no concrete operator: the first
    // token's nud parses the prefix position, and while the next token
    // binds tighter than rbp, that token's led extends the expression.
    result<node_type> expression(int rbp)
    {
        token_type token = stream_.advance();
        result<node_type> left = grammar_->nud(*this, token);
        if (left.has_error())
            return left;
        while (grammar_->lbp(stream_.peek()) > rbp)
        {
            token = stream_.advance();
            left = grammar_->led(*this, token, std::move(*left));
            if (left.has_error())
                return left;
        }
        return left;
    }

    result<node_type> parse()
    {
        result<node_type> node = expression(0);
        if (node.has_error())
            return node;
        if (!stream_.at_end())
            return error::trailing_input;
        return node;
    }

    // For nud/led implementations that consume extra tokens (grouping).
    decltype(auto) peek() const { return stream_.peek(); }
    token_type advance() { return stream_.advance(); }

private:
    const Grammar* grammar_;
    Stream stream_;
};

template <class Grammar, token_stream Stream>
    requires grammar_for<Grammar, Stream>
result<typename Grammar::node_type> parse(const Grammar& grammar, Stream stream)
{
    parser<Grammar, Stream> p(grammar, std::move(stream));
    return p.parse();
}

// Category helpers. Each expresses one operator category purely through
// the parser's expression() recursion, so a grammar built from them still
// routes everything through nud/led. Every Make returns result<node>.

// nud of an atom: the token is the whole expression.
template <class Make, class Token>
auto literal(Make make, const Token& token) -> decltype(make(token))
{
    return make(token);
}

// nud of a prefix operator binding with power bp.
template <class Parser, class Make>
result<typename Parser::node_type> prefix(Parser& p, int bp, Make make)
{
    result<typename Parser::node_type> operand = p.expression(bp);
    if (operand.has_error())
        return operand;
    return make(std::move(*operand));
}

// led of a left-associative infix operator.
template <class Parser, class Node, class Make>
result<typename Parser::node_type> infix_left(Parser& p, int bp, Node left, Make make)
{
    result<typename Parser::node_type> right = p.expression(bp);
    if (right.has_error())
        return right;
    return make(std::move(left), std::move(*right));
}

// led of a right-associative infix operator: the recursive call binds one
// step weaker, so an equal operator on the right wins.
template <class Parser, class Node, class Make>
result<typename Parser::node_type> infix_right(Parser& p, int bp, Node left, Make make)
{
    result<typename Parser::node_type> right = p.expression(bp - 1);
    if (right.has_error())
        return right;
    return make(std::move(left), std::move(*right));
}

// led of a postfix operator: no recursion, the left operand is everything.
template <class Node, class Make>
auto postfix(Node left, Make make) -> decltype(make(std::move(left)))
{
    return make(std::move(left));
}

// nud of a grouping open token: a full inner expression, then the token
// is_close accepts.
template <class Parser, class IsClose>
result<typename Parser::node_type> grouping(Parser& p, IsClose is_close)
{
    result<typename Parser::node_type> inner = p.expression(0);
    if (inner.has_error())
        return inner;
    if (!is_close(p.peek()))
        return error::expected_closing_token;
    p.advance();
    return inner;
}

} // namespace webcpp::pratt

namespace boost::system {

template <>
struct is_error_code_enum<webcpp::pratt::error> : std::true_type
{
};

} // namespace boost::system

#endif // WEBCPP_PRATT_PARSER_HPP
```

## Appendix B - calculator (reference, verified with `double`; becomes `include/webcpp/pratt/calculator.hpp` templated per requirement 5)

Everything below that says `double` becomes the `Number` template
parameter with default `boost::decimal::decimal64_t`; the structure,
semantics, binding powers and error behavior stay.

```cpp
// Copyright (c) 2026 WebCpp.org
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef WEBCPP_PRATT_CALCULATOR_HPP
#define WEBCPP_PRATT_CALCULATOR_HPP

#include <webcpp/pratt/parser.hpp>

#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace webcpp::pratt::calc {

enum class kind
{
    number,
    name,
    plus,
    minus,
    star,
    slash,
    percent,
    caret,
    lparen,
    rparen,
    assign,
    eq,
    ne,
    lt,
    le,
    gt,
    ge,
    increment,
    decrement,
    end,
};

struct token
{
    kind k = kind::end;
    double number = 0;
    std::string_view text{};
};

using environment = std::map<std::string, double, std::less<>>;

// Views in the returned tokens point into src; src must outlive them.
inline result<std::vector<token>> tokenize(std::string_view src)
{
    std::vector<token> tokens;
    std::size_t pos = 0;
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
    auto is_name = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    while (pos < src.size())
    {
        char c = src[pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            ++pos;
            continue;
        }
        if (is_digit(c))
        {
            double value = 0;
            while (pos < src.size() && is_digit(src[pos]))
                value = value * 10 + (src[pos++] - '0');
            if (pos + 1 < src.size() && src[pos] == '.' && is_digit(src[pos + 1]))
            {
                ++pos;
                double scale = 0.1;
                while (pos < src.size() && is_digit(src[pos]))
                {
                    value += (src[pos++] - '0') * scale;
                    scale /= 10;
                }
            }
            tokens.push_back({kind::number, value, {}});
            continue;
        }
        if (is_name(c))
        {
            std::size_t start = pos;
            while (pos < src.size() && (is_name(src[pos]) || is_digit(src[pos])))
                ++pos;
            tokens.push_back({kind::name, 0, src.substr(start, pos - start)});
            continue;
        }
        if (pos + 1 < src.size())
        {
            kind two = kind::end;
            std::string_view pair = src.substr(pos, 2);
            if (pair == "++") two = kind::increment;
            else if (pair == "--") two = kind::decrement;
            else if (pair == "==") two = kind::eq;
            else if (pair == "!=") two = kind::ne;
            else if (pair == "<=") two = kind::le;
            else if (pair == ">=") two = kind::ge;
            if (two != kind::end)
            {
                tokens.push_back({two, 0, pair});
                pos += 2;
                continue;
            }
        }
        kind one = kind::end;
        switch (c)
        {
        case '+': one = kind::plus; break;
        case '-': one = kind::minus; break;
        case '*': one = kind::star; break;
        case '/': one = kind::slash; break;
        case '%': one = kind::percent; break;
        case '^': one = kind::caret; break;
        case '(': one = kind::lparen; break;
        case ')': one = kind::rparen; break;
        case '=': one = kind::assign; break;
        case '<': one = kind::lt; break;
        case '>': one = kind::gt; break;
        default: return error::invalid_character;
        }
        tokens.push_back({one, 0, src.substr(pos, 1)});
        ++pos;
    }
    tokens.push_back({kind::end, 0, {}});
    return tokens;
}

class stream
{
public:
    using token_type = token;

    explicit stream(std::vector<token> tokens) : tokens_(std::move(tokens)) {}

    const token& peek() const { return tokens_[pos_]; }

    token advance()
    {
        token t = tokens_[pos_];
        if (t.k != kind::end)
            ++pos_;
        return t;
    }

    bool at_end() const { return tokens_[pos_].k == kind::end; }

private:
    std::vector<token> tokens_;
    std::size_t pos_ = 0;
};

// A parsed subexpression. A bare variable reference keeps its name so that
// '=', '++' and '--' can treat it as an assignment target; whether it also
// has a value depends on the environment (an undefined variable only
// errors when read, so 'a = 5' can define 'a').
struct node
{
    double value = 0;
    bool has_value = true;
    std::string_view name{};
};

class grammar
{
public:
    using node_type = node;

    // Binding powers; higher binds tighter. unary_bp sits below power_bp
    // so that -2 ^ 2 == -(2 ^ 2), the mathematical convention.
    static constexpr int assign_bp = 10;
    static constexpr int compare_bp = 20;
    static constexpr int additive_bp = 30;
    static constexpr int multiplicative_bp = 40;
    static constexpr int unary_bp = 45;
    static constexpr int power_bp = 50;
    static constexpr int postfix_bp = 70;

    explicit grammar(environment& env) : env_(&env) {}

    int lbp(const token& t) const
    {
        switch (t.k)
        {
        case kind::assign: return assign_bp;
        case kind::eq:
        case kind::ne:
        case kind::lt:
        case kind::le:
        case kind::gt:
        case kind::ge: return compare_bp;
        case kind::plus:
        case kind::minus: return additive_bp;
        case kind::star:
        case kind::slash:
        case kind::percent: return multiplicative_bp;
        case kind::caret: return power_bp;
        case kind::increment:
        case kind::decrement: return postfix_bp;
        default: return 0;
        }
    }

    template <class Parser>
    result<node> nud(Parser& p, const token& t) const
    {
        switch (t.k)
        {
        case kind::number:
            return literal(
                [](const token& tok) -> result<node> {
                    return node{tok.number, true, {}};
                },
                t);
        case kind::name:
        {
            auto found = env_->find(t.text);
            if (found == env_->end())
                return node{0, false, t.text};
            return node{found->second, true, t.text};
        }
        case kind::lparen:
            return grouping(p, [](const token& tok) { return tok.k == kind::rparen; });
        case kind::plus:
        case kind::minus:
        {
            double sign = t.k == kind::minus ? -1.0 : 1.0;
            return prefix(p, unary_bp, [sign](node operand) -> result<node> {
                if (!operand.has_value)
                    return error::undefined_variable;
                return node{sign * operand.value, true, {}};
            });
        }
        case kind::increment:
        case kind::decrement:
        {
            double step = t.k == kind::increment ? 1.0 : -1.0;
            return prefix(p, unary_bp, [this, step](node operand) -> result<node> {
                if (operand.name.empty())
                    return error::invalid_assignment_target;
                if (!operand.has_value)
                    return error::undefined_variable;
                double updated = operand.value + step;
                (*env_)[std::string(operand.name)] = updated;
                return node{updated, true, {}};
            });
        }
        case kind::end:
            return error::unexpected_end;
        default:
            return error::unexpected_token;
        }
    }

    template <class Parser>
    result<node> led(Parser& p, const token& t, node left) const
    {
        switch (t.k)
        {
        case kind::assign:
            if (left.name.empty())
                return error::invalid_assignment_target;
            return infix_right(p, assign_bp, std::move(left),
                               [this](node l, node r) -> result<node> {
                                   if (!r.has_value)
                                       return error::undefined_variable;
                                   (*env_)[std::string(l.name)] = r.value;
                                   return node{r.value, true, {}};
                               });
        case kind::plus:
            return infix_left(p, additive_bp, std::move(left),
                              arithmetic([](double a, double b) { return a + b; }));
        case kind::minus:
            return infix_left(p, additive_bp, std::move(left),
                              arithmetic([](double a, double b) { return a - b; }));
        case kind::star:
            return infix_left(p, multiplicative_bp, std::move(left),
                              arithmetic([](double a, double b) { return a * b; }));
        case kind::slash:
            return infix_left(p, multiplicative_bp, std::move(left),
                              arithmetic([](double a, double b) { return a / b; }));
        case kind::percent:
            return infix_left(p, multiplicative_bp, std::move(left),
                              arithmetic([](double a, double b) { return std::fmod(a, b); }));
        case kind::caret:
            return infix_right(p, power_bp, std::move(left),
                               arithmetic([](double a, double b) { return std::pow(a, b); }));
        case kind::eq:
            return infix_left(p, compare_bp, std::move(left),
                              arithmetic([](double a, double b) { return a == b ? 1.0 : 0.0; }));
        case kind::ne:
            return infix_left(p, compare_bp, std::move(left),
                              arithmetic([](double a, double b) { return a != b ? 1.0 : 0.0; }));
        case kind::lt:
            return infix_left(p, compare_bp, std::move(left),
                              arithmetic([](double a, double b) { return a < b ? 1.0 : 0.0; }));
        case kind::le:
            return infix_left(p, compare_bp, std::move(left),
                              arithmetic([](double a, double b) { return a <= b ? 1.0 : 0.0; }));
        case kind::gt:
            return infix_left(p, compare_bp, std::move(left),
                              arithmetic([](double a, double b) { return a > b ? 1.0 : 0.0; }));
        case kind::ge:
            return infix_left(p, compare_bp, std::move(left),
                              arithmetic([](double a, double b) { return a >= b ? 1.0 : 0.0; }));
        case kind::increment:
        case kind::decrement:
        {
            double step = t.k == kind::increment ? 1.0 : -1.0;
            return postfix(std::move(left), [this, step](node l) -> result<node> {
                if (l.name.empty())
                    return error::invalid_assignment_target;
                if (!l.has_value)
                    return error::undefined_variable;
                (*env_)[std::string(l.name)] = l.value + step;
                return node{l.value, true, {}};
            });
        }
        default:
            return error::unexpected_token;
        }
    }

private:
    // Both operands read as values; an unset one is an undefined variable.
    template <class Op>
    auto arithmetic(Op op) const
    {
        return [op](node l, node r) -> result<node> {
            if (!l.has_value)
                return error::undefined_variable;
            if (!r.has_value)
                return error::undefined_variable;
            return node{op(l.value, r.value), true, {}};
        };
    }

    environment* env_;
};

// The one-call entry point: evaluates src over env, writing assignments
// and increments into env.
inline result<double> evaluate(std::string_view src, environment& env)
{
    result<std::vector<token>> tokens = tokenize(src);
    if (tokens.has_error())
        return tokens.error();
    grammar g(env);
    result<node> parsed = parse(g, stream(std::move(*tokens)));
    if (parsed.has_error())
        return parsed.error();
    if (!parsed->has_value)
        return error::undefined_variable;
    return parsed->value;
}

} // namespace webcpp::pratt::calc

#endif // WEBCPP_PRATT_CALCULATOR_HPP
```

## Appendix C - sexpr grammar and acceptance checks (reference for the tests)

The `sexpr` grammar proves genericity (different token and node types,
engine and helpers unchanged) and belongs in the test tree, not in the
shipped headers. The `main` below is the verified acceptance content, run
with the `double` reference; reshape it into webcpp's test conventions
(chapter 9) and into requirement 5's number genericity (the same value
checks under the default decimal type and under `double`, expectations
written in `Number`, formatting adjusted accordingly). It is not the test
framework.

```cpp
// Acceptance checks for the pratt engine and its calculator, plus a second
// grammar (different token and node types) proving the engine is generic.

#include "calculator.hpp"
#include "parser.hpp"

#include <cstdio>
#include <string>
#include <string_view>

namespace pratt = webcpp::pratt;

// Second grammar: tokens are the input's characters, nodes are strings
// holding the parse as an S-expression. Engine and helpers unchanged.
namespace sexpr {

class stream
{
public:
    using token_type = char;

    explicit stream(std::string_view src) : src_(src) {}

    char peek() const { return pos_ < src_.size() ? src_[pos_] : '\0'; }

    char advance()
    {
        char c = peek();
        if (pos_ < src_.size())
            ++pos_;
        return c;
    }

    bool at_end() const { return pos_ >= src_.size(); }

private:
    std::string_view src_;
    std::size_t pos_ = 0;
};

struct grammar
{
    using node_type = std::string;

    int lbp(char t) const
    {
        switch (t)
        {
        case '+':
        case '-': return 10;
        case '*': return 20;
        case '^': return 30;
        default: return 0;
        }
    }

    template <class Parser>
    pratt::result<std::string> nud(Parser& p, char t) const
    {
        if (t >= '0' && t <= '9')
            return pratt::literal(
                [](char c) -> pratt::result<std::string> { return std::string(1, c); }, t);
        if (t == '(')
            return pratt::grouping(p, [](char c) { return c == ')'; });
        if (t == '-')
            return pratt::prefix(p, 40, [](std::string operand) -> pratt::result<std::string> {
                return "(neg " + operand + ")";
            });
        if (t == '\0')
            return pratt::error::unexpected_end;
        return pratt::error::unexpected_token;
    }

    template <class Parser>
    pratt::result<std::string> led(Parser& p, char t, std::string left) const
    {
        auto join = [t](std::string l, std::string r) -> pratt::result<std::string> {
            return "(" + std::string(1, t) + " " + l + " " + r + ")";
        };
        if (t == '^')
            return pratt::infix_right(p, lbp(t), std::move(left), join);
        return pratt::infix_left(p, lbp(t), std::move(left), join);
    }
};

} // namespace sexpr

static_assert(pratt::token_stream<pratt::calc::stream>);
static_assert(pratt::grammar_for<pratt::calc::grammar, pratt::calc::stream>);
static_assert(pratt::token_stream<sexpr::stream>);
static_assert(pratt::grammar_for<sexpr::grammar, sexpr::stream>);

static int failures = 0;

static void check_value(pratt::calc::environment& env, std::string_view src, double expected)
{
    pratt::result<double> r = pratt::calc::evaluate(src, env);
    if (r.has_error() || *r != expected)
    {
        std::printf("FAIL  %.*s  expected %g got %s\n", int(src.size()), src.data(), expected,
                    r.has_error() ? r.error().message().c_str() : std::to_string(*r).c_str());
        ++failures;
        return;
    }
    std::printf("ok    %.*s  =>  %g\n", int(src.size()), src.data(), *r);
}

static void check_error(pratt::calc::environment& env, std::string_view src, pratt::error expected)
{
    pratt::result<double> r = pratt::calc::evaluate(src, env);
    if (!r.has_error() || r.error() != expected)
    {
        std::printf("FAIL  %.*s  expected error %d\n", int(src.size()), src.data(), int(expected));
        ++failures;
        return;
    }
    std::printf("ok    %.*s  =>  error: %s\n", int(src.size()), src.data(),
                r.error().message().c_str());
}

static void check_sexpr(std::string_view src, std::string_view expected)
{
    pratt::result<std::string> r = pratt::parse(sexpr::grammar{}, sexpr::stream(src));
    if (r.has_error() || *r != expected)
    {
        std::printf("FAIL  sexpr %.*s\n", int(src.size()), src.data());
        ++failures;
        return;
    }
    std::printf("ok    sexpr %.*s  =>  %s\n", int(src.size()), src.data(), r->c_str());
}

int main()
{
    pratt::calc::environment env;

    check_value(env, "a = 43 + 12 + 2 * ( 44 - 12 ) ^ 4", 2097207);
    check_value(env, "a", 2097207);
    check_value(env, "2 ^ 3 ^ 2", 512);
    check_value(env, "-2 ^ 2", -4);
    check_value(env, "b = a / 2 - 100", 1048503.5);
    check_value(env, "b > 1000000", 1);
    check_value(env, "b == a / 2 - 100", 1);
    check_value(env, "10 % 3", 1);
    check_value(env, "i = 0", 0);
    check_value(env, "i++", 0);
    check_value(env, "i", 1);
    check_value(env, "++i", 2);
    check_value(env, "--i", 1);
    check_value(env, "i--", 1);
    check_value(env, "i", 0);
    check_value(env, "c = d = 5", 5);
    check_value(env, "c + d", 10);
    check_value(env, "1.5 * 2", 3);

    check_error(env, "x + 1", pratt::error::undefined_variable);
    check_error(env, "5 = 3", pratt::error::invalid_assignment_target);
    check_error(env, "3++", pratt::error::invalid_assignment_target);
    check_error(env, "( 1 + 2", pratt::error::expected_closing_token);
    check_error(env, "1 2", pratt::error::trailing_input);
    check_error(env, "1 +", pratt::error::unexpected_end);
    check_error(env, "1 $ 2", pratt::error::invalid_character);
    check_error(env, "", pratt::error::unexpected_end);

    check_sexpr("1+2*3", "(+ 1 (* 2 3))");
    check_sexpr("2^3^2", "(^ 2 (^ 3 2))");
    check_sexpr("-(1+2)*3", "(* (neg (+ 1 2)) 3)");
    check_sexpr("1-2-3", "(- (- 1 2) 3)");

    if (failures != 0)
    {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}
```

## Appendix D - openui micro-subset REPL (reference, verified; becomes the example of requirement 8, grammar shared with the tests)

Compiled with the same flags and exercised end to end; the session
transcript follows the code. Note `(` holding both Pratt denotations and
the call's argument loop living inside its led - the pattern the real
openui-lang parser will grow from.

```cpp
// REPL over a micro-subset of openui-lang, built on the pratt engine.
// Each line is parsed and echoed back as the S-expression of its AST.
// The subset: identifiers, integers, double-quoted strings, '=' assignment
// (right-assoc), ':' named properties, and 'Component(arg, ...)' calls,
// where '(' holds both Pratt denotations: nud is grouping, led is a call.

#include "parser.hpp"

#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace pratt = webcpp::pratt;

namespace openui {

enum class kind
{
    identifier,
    integer,
    string,
    lparen,
    rparen,
    comma,
    colon,
    assign,
    end,
};

struct token
{
    kind k = kind::end;
    std::string_view text{};
};

// Views in the returned tokens point into src; src must outlive them.
inline pratt::result<std::vector<token>> tokenize(std::string_view src)
{
    std::vector<token> tokens;
    std::size_t pos = 0;
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
    auto is_name = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    while (pos < src.size())
    {
        char c = src[pos];
        if (c == ' ' || c == '\t')
        {
            ++pos;
            continue;
        }
        if (is_name(c))
        {
            std::size_t start = pos;
            while (pos < src.size() && (is_name(src[pos]) || is_digit(src[pos])))
                ++pos;
            tokens.push_back({kind::identifier, src.substr(start, pos - start)});
            continue;
        }
        if (is_digit(c))
        {
            std::size_t start = pos;
            while (pos < src.size() && is_digit(src[pos]))
                ++pos;
            tokens.push_back({kind::integer, src.substr(start, pos - start)});
            continue;
        }
        if (c == '"')
        {
            std::size_t start = pos++;
            while (pos < src.size() && src[pos] != '"')
                ++pos;
            if (pos == src.size())
                return pratt::error::unexpected_end;
            ++pos;
            tokens.push_back({kind::string, src.substr(start, pos - start)});
            continue;
        }
        kind one = kind::end;
        switch (c)
        {
        case '(': one = kind::lparen; break;
        case ')': one = kind::rparen; break;
        case ',': one = kind::comma; break;
        case ':': one = kind::colon; break;
        case '=': one = kind::assign; break;
        default: return pratt::error::invalid_character;
        }
        tokens.push_back({one, src.substr(pos, 1)});
        ++pos;
    }
    tokens.push_back({kind::end, {}});
    return tokens;
}

class stream
{
public:
    using token_type = token;

    explicit stream(std::vector<token> tokens) : tokens_(std::move(tokens)) {}

    const token& peek() const { return tokens_[pos_]; }

    token advance()
    {
        token t = tokens_[pos_];
        if (t.k != kind::end)
            ++pos_;
        return t;
    }

    bool at_end() const { return tokens_[pos_].k == kind::end; }

private:
    std::vector<token> tokens_;
    std::size_t pos_ = 0;
};

struct grammar
{
    using node_type = std::string;

    // Binding powers; the call binds tightest, so 'Stack(x)(y)' chains.
    static constexpr int assign_bp = 10;
    static constexpr int pair_bp = 20;
    static constexpr int call_bp = 90;

    int lbp(const token& t) const
    {
        switch (t.k)
        {
        case kind::assign: return assign_bp;
        case kind::colon: return pair_bp;
        case kind::lparen: return call_bp;
        default: return 0;
        }
    }

    template <class Parser>
    pratt::result<std::string> nud(Parser& p, const token& t) const
    {
        switch (t.k)
        {
        case kind::identifier:
        case kind::integer:
        case kind::string:
            return pratt::literal(
                [](const token& tok) -> pratt::result<std::string> {
                    return std::string(tok.text);
                },
                t);
        case kind::lparen:
            return pratt::grouping(p, [](const token& tok) { return tok.k == kind::rparen; });
        case kind::end:
            return pratt::error::unexpected_end;
        default:
            return pratt::error::unexpected_token;
        }
    }

    template <class Parser>
    pratt::result<std::string> led(Parser& p, const token& t, std::string left) const
    {
        switch (t.k)
        {
        case kind::assign:
            return pratt::infix_right(p, assign_bp, std::move(left),
                                      [](std::string l, std::string r)
                                          -> pratt::result<std::string> {
                                          return "(= " + l + " " + r + ")";
                                      });
        case kind::colon:
            return pratt::infix_left(p, pair_bp, std::move(left),
                                     [](std::string l, std::string r)
                                         -> pratt::result<std::string> {
                                         return "(: " + l + " " + r + ")";
                                     });
        case kind::lparen:
        {
            // The call's argument loop lives inside this led: each argument
            // is a full expression; ',' (lbp 0) ends one, ')' ends the call.
            std::string call = "(call " + std::move(left);
            if (p.peek().k == kind::rparen)
            {
                p.advance();
                return call + ")";
            }
            while (true)
            {
                pratt::result<std::string> argument = p.expression(0);
                if (argument.has_error())
                    return argument;
                call += " " + *argument;
                kind next = p.peek().k;
                if (next == kind::comma)
                {
                    p.advance();
                    continue;
                }
                if (next == kind::rparen)
                {
                    p.advance();
                    return call + ")";
                }
                return pratt::error::expected_closing_token;
            }
        }
        default:
            return pratt::error::unexpected_token;
        }
    }
};

} // namespace openui

static_assert(pratt::token_stream<openui::stream>);
static_assert(pratt::grammar_for<openui::grammar, openui::stream>);

int main()
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line == "exit")
            return 0;
        if (line.empty())
            continue;
        pratt::result<std::vector<openui::token>> tokens = openui::tokenize(line);
        if (tokens.has_error())
        {
            std::printf("error: %s\n", tokens.error().message().c_str());
            continue;
        }
        pratt::result<std::string> tree =
            pratt::parse(openui::grammar{}, openui::stream(std::move(*tokens)));
        if (tree.has_error())
        {
            std::printf("error: %s\n", tree.error().message().c_str());
            continue;
        }
        std::printf("%s\n", tree->c_str());
    }
    return 0;
}
```

Verified session transcript (input line, then output):

```
root = Stack()
(= root (call Stack))

root = Stack(spacing: 8, Button(label: "Pay"), Text("Total"))
(= root (call Stack (: spacing 8) (call Button (: label "Pay")) (call Text "Total")))

view = Row(Button(label: "ok"), gap: 4)
(= view (call Row (call Button (: label "ok")) (: gap 4)))

Stack(1, 2)(3)
(call (call Stack 1 2) 3)

(x)
x

Stack(1
error: expected closing token

= 3
error: unexpected token

root = Stack(label: "unterminated)
error: unexpected end of input

root @ 3
error: invalid character

exit            [exits with status 0]
```
