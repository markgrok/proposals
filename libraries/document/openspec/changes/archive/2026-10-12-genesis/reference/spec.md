---
title: "webcpp::document: a JSON document that behaves as JavaScript does"
tags: [webcpp, document, json, javascript, interprocess, multi-index, decimal, streaming, spec]
status: active
created: 2026-10-12
---

# webcpp::document

The owner's decisions of 2026-10-12 (`~/.buzz/PLANS/WEBCPP_DESIGN.md`, the entries from "a new
library, webcpp::json" to "bytes and boolean in the traits; no bigint"). Research:
`~/.buzz/RESEARCH/WEBCPP_INTERPROCESS_WITHOUT_EXCEPTIONS.md`,
`~/.buzz/RESEARCH/JAVASCRIPT_OBJECT_KEY_ORDER.md`,
`~/.buzz/RESEARCH/WEBCPP_SEGMENT_PERSISTENCE.md`. It supersedes
`~/.buzz/PLANS/WEBCPP_XACTOR_DOCUMENT_SPEC.md`.

Rulings marked **(ruling)** were proposed in this specification and presented to the owner; the
owner approved the specification with them on 2026-10-12.

## 1. Purpose

A JSON document whose behaviour is JavaScript's: what `JSON.parse`, property access, assignment,
`delete`, the array methods and `JSON.stringify` do in JavaScript, `webcpp::document` does in C++,
with the same key order and the same text out, so that a JavaScript peer and a C++ program agree
byte for byte. Its tree lives in one relocatable image, built through an allocator, so that a
document can live in a memory segment, be made durable, and travel as binary.

Why not Boost.JSON (the owner's rationale, measured on Boost 1.92): closer compatibility with
JavaScript. Boost.JSON does not follow JavaScript's key order, its `object::erase` reorders members,
its numbers are doubles (`0.1000000000000000000000000000000001` writes back as `1E-1`), its object
index hashes keys (9.23 s for a crafted 4 MB input), and a `boost::json::value` holds pointers, so it
can neither live in a segment nor travel as binary. webcpp::document uses Boost.JSON's SAX parser,
`basic_parser`, and nothing else of it.

## 2. Decisions

1. A library of its own, `webcpp::document`, at `libs/document`, repository `webcpporg/document`
   (created only with the owner's go). Its namespace alias in the examples is
   `namespace wd = webcpp::document;`.
2. Its API imitates manipulating JSON in JavaScript, in a C++ style (section 6).
3. Object keys follow JavaScript's order by default: array-index keys first, ascending, then every
   other key in insertion order (owner's approval (1)); the traits may choose another order.
4. A repeated key in parsing is accepted as JavaScript accepts it: the last value, in the first
   one's place (owner's approval (2)).
5. The stored types are chosen by traits: `boolean`, `integer`, `number`, `string`, `date`,
   `bytes`, and policies for objects and arrays; by default `bool`, `std::int64_t`, Boost.Decimal's
   `decimal128_t`, UTF-8, milliseconds, base64, JavaScript's key order, dense arrays (section 5).
6. The array methods of JavaScript: `push`, `pop`, `shift`, `unshift`, `splice`, `reverse`,
   `slice`, `concat` (owner, 2026-10-12).
7. Streaming in both directions in the first version: an incremental parser fed in chunks, values
   delivered as they complete through selectors, and `stringify_to` (owner, 2026-10-12).
8. Exception-free in almost everything: every operation that can fail returns `result`; only the
   JavaScript-style sugar that cannot return one needs exceptions, and calling it in a build without
   exceptions is a compile error that names the reason (owner, 2026-10-12).
9. The heap is Boost.Interprocess's `rbtree_best_fit` used directly, with an allocator and strings
   of webcpp's own, because `managed_external_buffer`, `segment_manager` and Interprocess's
   `allocator` throw bare and do not compile without exceptions (owner, 2026-10-12).

## 3. Headers and build

| Header                            | What                                                                                      |
| --------------------------------- | ----------------------------------------------------------------------------------------- |
| `<webcpp/document.hpp>`           | every public header below                                                                 |
| `<webcpp/document/config.hpp>`    | first in every header; includes `<webcpp/config.hpp>`; `WEBCPP_DOCUMENT_NO_EXCEPTIONS`    |
| `<webcpp/document/errors.hpp>`    | `errc`, the category `webcpp.document`, `result`, and with exceptions the exception types |
| `<webcpp/document/traits.hpp>`    | `default_traits`, `js_traits`, the scalar concepts, the policies                          |
| `<webcpp/document/kind.hpp>`      | `kind`, `undefined`, `null`                                                               |
| `<webcpp/document/literal.hpp>`   | `wd::object`, `wd::array`: values written in C++ before they are stored                   |
| `<webcpp/document/number.hpp>`    | number parsing from text and JavaScript's formatting                                      |
| `<webcpp/document/value.hpp>`     | `value` and `const_value`, the handles                                                    |
| `<webcpp/document/document.hpp>`  | `basic_document<Traits, Storage>`, `document`, the storages                               |
| `<webcpp/document/image.hpp>`     | the binary image's header, `open`, `format`, `from_image`                                 |
| `<webcpp/document/parse.hpp>`     | `parse`, `parse_options`, and with exceptions `parse_or_throw`                            |
| `<webcpp/document/parser.hpp>`    | the streaming parser and its selectors                                                    |
| `<webcpp/document/stringify.hpp>` | `stringify`, `stringify_to`                                                               |

`build.jam`: `alias document : : : : <include>include <library>/boost//headers
<library>/webcpp/config//config <library>/webcpp//boost_json ;` (Boost.JSON's parser is not
header-only: it needs `tools/boost_json.cpp`, measured). Boost libraries used: Interprocess
(`offset_ptr`, `rbtree_best_fit`, `null_mutex`), MultiIndex, Decimal, JSON (`basic_parser` only),
System (`result`, `error_code`). Not used: Interprocess's `managed_external_buffer`,
`segment_manager`, `allocator` and `sync/mutex_family.hpp` (bare throws).

Infrastructure this needs: Emscripten builds with exceptions use `-fwasm-exceptions` (the Jamroot
sets it for the WASI toolsets only today), for the exception-using sugar and its tests.

## 4. The model

A document holds one value. A value's `kind`, which is JavaScript's `typeof` with `null`, arrays,
dates and bytes told apart:

| `kind` | JavaScript |
| --- | --- |
| `undefined` | `undefined`: never stored; what a handle reads when nothing is there |
| `null` | `null` |
| `boolean` | `true`, `false` |
| `number` | a number, held as the traits' `integer` or `number` (section 7) |
| `string` | a string |
| `date` | a `Date` |
| `bytes` | a `Uint8Array` |
| `array` | an array |
| `object` | a plain object |

### 4.1 Storage of the tree

One `boost::multi_index_container` of nodes, in a heap inside the document's image:

| Field | Type | What |
| --- | --- | --- |
| `id` | `std::uint64_t` | unique, never reused; the root is 1 |
| `parent` | `std::uint64_t` | the container's id; 0 for the root |
| `group` | `std::uint8_t` | the object policy's rank of the key (6.2); 0 for an array element |
| `ordinal` | `std::uint64_t` | an element's index, or the key's position within its group by the object policy |
| `key` | webcpp string | the member's key; empty for an element |
| `kind`, `repr` | `std::uint8_t` each | the kind; for a number, `integer` or `number` |
| `scalar` | the traits' largest fixed scalar, padded to 8 | a boolean, an integer, a number or a date; for an array its length, for an object its next insertion number |
| `text` | webcpp string | a string's code units or a bytes value's bytes |

A webcpp string is an `offset_ptr<std::byte, std::int64_t, std::uint64_t, 8>` and a
`std::uint64_t` size, allocated exactly through the heap's non-throwing `allocate`, never a
`boost::container::string` (whose allocation may grow beyond the size and throws on failure).

| Index | Key | Unique | Serves |
| --- | --- | --- | --- |
| `by_id` | `id` | yes | a handle's lookup |
| `by_order` | `(parent, group, ordinal)` | yes | an element by index; members in the policy's order; iteration |
| `by_key` | `(parent, key)` | no (the document keeps keys unique) | a member by key |

All ordered, never hashed.

### 4.2 The heap and the allocator

The image's heap is `rbtree_best_fit<no_mutexes, offset_ptr<void, std::int64_t, std::uint64_t, 8>,
16>`, where `no_mutexes` is webcpp's own family of `null_mutex` (not `mutex_family.hpp`). The
container's allocator is webcpp's: it allocates through the algorithm's `allocate_aligned`, zeroes
the block, and returns it; it zeroes a block before it frees it. An allocation that fails is a bug
of the document, never a run-time event, because every operation finds its room first (section 12),
so the allocator reports it through `boost::throw_exception` (which aborts in a build without
exceptions) (owner, 2026-10-12).

### 4.3 Equality

`==` compares values (JavaScript has no deep equality; this one is structural) **(ruling)**: the same
kind; numbers by value (`1`, `1.0` and `1e0` are equal); strings, dates and bytes by value; arrays
element by element; objects with the same keys, each with an equal value. Key order does not
matter.

## 5. Traits

```cpp
struct default_traits {
    using boolean = bool;
    using integer = std::int64_t;
    using number  = boost::decimal::decimal128_t;
    using string  = wd::utf8;                                         // the stored encoding
    using date    = std::chrono::sys_time<std::chrono::milliseconds>;
    using bytes   = wd::base64;                                       // how stringify writes bytes
    using object  = wd::javascript_order;                             // the key order
    using array   = wd::dense;
};

struct js_traits : default_traits {
    using number = double;                                            // JavaScript's own numbers
    using string = wd::utf16;
    using bytes  = wd::javascript_bytes;
};

template <class Traits = default_traits, class Storage = owned>
class basic_document;
using document = basic_document<>;
```

### 5.1 The scalars

`boolean`, `integer`, `number` and `date` are stored in a node's `scalar`; each must satisfy
`wd::stored_scalar` (checked when the traits are used): trivially copyable, of a size of 1, 2, 4,
8 or 16 bytes, of the same layout on every target (no `long`, `std::size_t` or pointer), and given,
by a specialisation of `wd::scalar_codec<T>`, how it is stored (to and from its bytes in the node),
read from JSON text, and written as JavaScript writes it. The defaults:

| Scalar | Default | Stored as | From text | Written as |
| --- | --- | --- | --- | --- |
| `boolean` | `bool` | one byte, 0 or 1 (`bool`'s size is not fixed by the standard) | `true`, `false` | `true`, `false` |
| `integer` | `std::int64_t` | 8 bytes | an integer literal that fits | decimal digits |
| `number` | `decimal128_t` | 16 bytes | any other number literal (section 7) | JavaScript's Number::toString (section 7) |
| `date` | milliseconds since the epoch | `std::int64_t` | a string, only with `revive_dates` or `get<date>()` (section 8) | Date's `toJSON` |

With `number = double` (`js_traits`), numbers round as JavaScript's do, and NaN and the infinities
(which only arithmetic can make: JSON has none) are written `null`, as JavaScript writes them.

### 5.2 Strings and bytes

`string` chooses the stored encoding: `utf8` (default; `length()` counts UTF-16 code units by
decoding, and a lone surrogate in JSON text is refused) or `utf16` (as JavaScript holds strings:
`length()` and indices directly, and lone surrogates kept as JavaScript keeps them). Keys use the
same encoding. `bytes` chooses how `stringify` writes a bytes value: `base64` (default, RFC 4648
with padding), `base64url` (RFC 4648 section 5, no padding), or `javascript_bytes` (what
`JSON.stringify` writes for a Uint8Array: `{"0":1,"1":2}`). Bytes are stored raw in the image and sent
as binary as they are.

### 5.3 Objects and arrays: policies

Arrays and objects are structure (the nodes sharing a parent), not stored C++ types, so the traits
choose their behaviour:

| Policy | Order of an object's keys |
| --- | --- |
| `javascript_order` (default) | array-index keys ascending, then the others in insertion order (section 6) |
| `insertion_order` | insertion order, the text's order, whatever the keys |
| `sorted` | by the keys' code units (for canonical JSON, signatures and hashes) |

`dense` (the only array policy): no holes; writing past the end fills with `null`.

### 5.4 Identity

The traits are part of the image: the header's layout tag hashes each scalar's size and codec name,
the encoding, the bytes policy and the object policy, so a document written with other traits is
refused (`incompatible_document`), never read wrongly.

## 6. The interface

Two layers. The core never throws and is available in every build; the sugar throws, like
JavaScript, and exists only with exceptions.

### 6.1 Parsing and writing

```cpp
wd::result<wd::document> made = wd::parse(text, opt);   // JSON.parse, as a result
wd::document doc = wd::parse_or_throw(text);            // JSON.parse; throws syntax_error (sugar)
std::string out = wd::stringify(doc);                   // JSON.stringify(doc)
std::string pretty = wd::stringify(doc, 2);             // JSON.stringify(doc, null, 2)
wd::stringify_to(doc, sink, 2);                         // the same bytes, in pieces (section 10)
```

`stringify` is also a member: `doc.stringify()`, `doc["user"].stringify(2)`. `stringify` returns a
`std::string`, so its only failure is the standard library's own `std::bad_alloc` (owner,
2026-10-12); a program that must not meet even that uses `stringify_to` with a sink of its own.

### 6.2 Handles

`wd::value` (reads and writes) and `wd::const_value` (reads), returned by `doc.root()`, `doc[...]`
and every navigation. A handle names one place in one document and is cheap to copy:

| A handle names | Made by | Reads as |
| --- | --- | --- |
| a node | navigation to something that exists | the node |
| a missing member or element of a container that exists | `obj["absent"]`, `arr[9]` past the end | `undefined`; `set` creates it |
| nothing | navigation through `undefined`, a scalar, or an erased node | `undefined`; `set` fails with `errc::type` |

A handle holds the document's address and node ids, so it stays valid when the document grows and
its image moves; it reads `undefined` once its node is erased. It is invalid once its document object
is moved from or destroyed, as a container's iterators would be **(ruling)**. A `std::string_view`
from a string is valid until the document's next change.

### 6.3 The core: reading

```cpp
wd::kind k = v.kind();                   // typeof, with null, arrays, dates and bytes told apart
bool u = v.is_undefined();               // also is_null, is_boolean, is_number, is_integer, is_string, is_date, is_bytes, is_array, is_object
wd::result<T> r = v.get<T>();            // T: the traits' scalars, any integer type, decimal types, std::string, std::string_view, bytes
T x = v.value_or(fallback);              // v ?? fallback: the fallback when undefined or null, else get<T>()'s value, else the fallback
bool has = v.contains("key");            // "key" in v (objects); false for any other kind
std::size_t n = v.size();                // an object's key count or an array's length; 0 for any other kind
wd::result<std::size_t> l = v.length();  // a.length, or s.length in UTF-16 code units; errc::type for other kinds
wd::value w = v["key"];  wd::value e = v[3];   // navigation, never fails
wd::value last = v.at(-1);               // Array.prototype.at: a negative index counts from the end
```

`get<T>` fails with `errc::type` for another kind and `errc::range` for a number that is not an
integer or does not fit `T`, or that `T` cannot hold exactly. There is no conversion to or from
`float` or `double` unless the traits' `number` is one.

### 6.4 The core: writing

```cpp
wd::result<void> s = doc["user"]["age"].set(30);     // obj.user.age = 30: creates the member if absent
doc["user"]["age"].set(wd::undefined);               // erases it: see 6.8
doc["user"]["nick"].set(nullptr);                    // null
doc["user"]["tags"].set(wd::array{"a", "b"});
doc["user"]["address"].set(wd::object{{"city", "Recife"}, {"zip", "50000"}});
doc["copy"].set(other["user"]);                      // a deep copy (6.8): never an alias
wd::result<bool> existed = doc["user"].erase("age"); // delete obj.user.age
doc["user"].assign_from(defaults);                   // Object.assign(obj.user, defaults)
```

Settable: the traits' scalars; every integer type; the Boost.Decimal types (finite, else
`errc::range`); `std::string_view`, `const char*`, `std::string` (valid in the encoding, else
`errc::type`); `std::span<const std::byte>` (bytes); a `date`; `nullptr` / `wd::null`;
`wd::undefined`; `wd::object` and `wd::array` literals; a `const_value` or a document (deep copy).
Every core writing function is `[[nodiscard]]`.

### 6.5 The core: arrays, JavaScript's methods

| JavaScript | webcpp::document | Returns |
| --- | --- | --- |
| `a.length = n` | `a.set_length(n)` | `result<void>`: truncates, or appends `null` |
| `a.push(x, y)` | `a.push(x, y)` | `result<std::size_t>`: the new length |
| `a.pop()` | `a.pop()` | `result<document>`: the removed value, or a document holding `undefined` when empty |
| `a.shift()` | `a.shift()` | the same, from the front |
| `a.unshift(x, y)` | `a.unshift(x, y)` | `result<std::size_t>`: the new length |
| `a.splice(start, count, x, y)` | `a.splice(start, count, x, y)` | `result<document>`: the array of the removed elements |
| `a.splice(start)` | `a.splice(start)` | the same, removing to the end |
| `a.reverse()` | `a.reverse()` | `result<void>` |
| `a.slice(begin, end)` | `a.slice(begin, end)` | `result<document>`: the copied elements |
| `a.concat(b, x)` | `a.concat(b, x)` | `result<document>`: this array, then each argument (an array's elements, or the value) |
| `a[i] = x` past the end | `a[i].set(x)` | the elements between become `null` |

Indices follow JavaScript exactly: a negative `start` counts from the end and is clamped at 0; a
`start` past the length is the length; an omitted `count` removes to the end; a negative `count` is 0;
`slice`'s `end` likewise. A method on a handle that is not an array fails with `errc::type`.

### 6.6 The sugar: only with exceptions

```cpp
doc["user"]["age"] = 30;                    // set(30), throwing on failure
std::string_view name = v.as_string();     // get<std::string_view>(), throwing
T n = v.as<T>();                            // get<T>(), throwing
bool b = v.as_bool();
wd::document d = wd::parse_or_throw(text);
```

Each is declared with a named constraint:

```cpp
template <class T>
    requires detail::exceptions_enabled<T>      // false when WEBCPP_DOCUMENT_NO_EXCEPTIONS is defined
T as() const;
```

so in a build without exceptions a call is a compile error that names
`detail::exceptions_enabled`. The array methods and every other core function exist in both builds
and return `result`.

### 6.7 Iteration

```cpp
for (auto [key, val] : obj.entries()) {}  // Object.entries, in the policy's order
for (std::string_view key : obj.keys()) {} // Object.keys
for (auto val : obj.values()) {}           // Object.values
for (auto val : arr) {}                    // for (const val of arr)
```

The ranges read the document as it is when each step is taken; changing the container while
iterating it is undefined, as it is for a standard container.

### 6.8 Where it differs from JavaScript, on purpose

| JavaScript | webcpp::document | Why |
| --- | --- | --- |
| `a.x = b.y` shares one object | copies the subtree | a document is a tree, as JSON is: no shared objects |
| `o.x = undefined` keeps key `x`, which stringify omits | erases `x` | nothing stores `undefined`; stringify is identical, `Object.keys` would differ |
| `[undefined]` stringifies `[null]` | stores `null` | the same text |
| arrays may have holes | `null` fills them | the same text |
| numbers are doubles | exact with the default traits (section 7) | no floating point; `js_traits` restores doubles |
| `JSON.parse` of 40 digits rounds; of `1e999` gives `Infinity` | with the default traits, more than 34 significant digits, or an exponent past `decimal128`'s range (beyond about `1e6144`), is refused (`errc::range`); `1e999` is stored exactly, and a JavaScript peer reads it as `Infinity` (section 7 decides, 2026-10-12); with `js_traits`, as JavaScript: `Infinity`, written `null` | what is stored is what was sent |
| a lone surrogate `"\ud800"` parses | refused by the parser with either encoding; `set` keeps one with `utf16` | Boost.JSON's parser, which document uses, cannot deliver one (the owner, 2026-10-12: documented, not worked around) |
| `-0` | `0` | `JSON.stringify(-0)` writes `0` too |
| a Uint8Array stringifies `{"0":1,...}` | base64 by default | the traits' `bytes` policy; `javascript_bytes` restores it |
| prototypes, getters, symbols, `toJSON`, replacer, reviver, Map, Set, BigInt | none | out of scope (section 15) |

## 7. Numbers

With the default traits a number is stored exactly, in one of two forms chosen by value (owner,
2026-10-12): an integer that fits `integer` is an `integer`; any other value is a `number`,
normalised (no trailing zeros), so `1.0`, `1e0` and `1` are one stored value, as they are one
number in JavaScript, and `18446744073709551615` is an exact `number`. `is_integer()` is JavaScript's
`Number.isInteger`.

Parsing reads a number from its text, never through a double: an integer literal that fits
`integer` is one; any other goes through the number codec (Boost.Decimal's `from_chars` into
`decimal128`), then the form above. More than 34 significant digits, or an exponent outside
`decimal128`'s range, is `errc::range` (6.8).

Writing follows JavaScript's Number::toString (ECMA-262, radix 10) on the stored digits: with `k`
significant digits and the decimal point after position `n`:

| Condition | Written as | Example |
| --- | --- | --- |
| `k <= n <= 21` | the digits, then `n - k` zeros | `1e20` writes `100000000000000000000` |
| `0 < n <= 21` | the digits with a point after `n` | `123.45` |
| `-6 < n <= 0` | `0.`, `-n` zeros, the digits | `0.000001` |
| otherwise | one digit, `.` and the rest if any, `e`, the sign, `n - 1` | `1e+21`, `1e-7`, `1.5e+300` |

So every number `JSON.stringify` produces (the shortest text of a double, at most 17 digits) writes
back byte for byte as JavaScript writes it, and numbers JavaScript cannot hold stay exact.

## 8. Strings, dates and bytes in text

Strings are validated where they enter (Boost.JSON validates the parser's input; `set` validates).
Writing escapes as `JSON.stringify` does: `"` and `\` with a backslash; `\b`, `\f`, `\n`, `\r`,
`\t`; any other character below U+0020 as `\u` and four lowercase hex digits; with `utf16`, a lone
surrogate as `\u` and its four hex digits (as `JSON.stringify` does since ES2019); nothing else, so
`/`, non-ASCII text and U+2028 / U+2029 are written as they are.

A date is written as JavaScript's `Date.prototype.toJSON` writes it (measured in node 26):
`"2026-10-12T03:04:05.678Z"`; a year outside 0 to 9999 as six digits with a sign,
`"+275760-09-13T00:00:00.000Z"`, `"-000001-01-01T00:00:00.000Z"`. Its range is JavaScript's,
plus or minus 8.64e15 ms; `set` outside it fails with `errc::range`. JSON has no dates, so a parse
keeps every string a string (as `JSON.parse` does), unless `parse_options::revive_dates`, which turns
a string in exactly the format above into a date; `get<date>()` of such a string converts too.

Bytes are written by the traits' policy (5.2); a parse keeps strings as strings unless
`parse_options::revive_bytes` (a string valid in the policy's encoding becomes bytes; with
`javascript_bytes`, an object whose keys are exactly `"0"` to `"n-1"` with integer values 0 to 255);
`get<bytes>()` of such a value converts too.

## 9. Key order

An array index is a key that is the canonical decimal spelling of an integer from 0 to
4,294,967,294: `"0"`, `"7"`, `"10"`; not `"01"`, `"-1"`, `"1.0"`, `" 1"`, `"4294967295"`. With
`javascript_order`, an object's keys come out as JavaScript's OrdinaryOwnPropertyKeys gives them:
array indices ascending (group 0, ordinal the index), then the others in the order they were created
(group 1, ordinal the object's next insertion number). Assigning an existing key keeps its place;
erasing a key and creating it again puts it last among the others. Measured in node 26:
`JSON.parse('{"b":1,"2":2,"a":3,"1":4}')` is `{"1":4,"2":2,"b":1,"a":3}`, and the document writes
the same. `insertion_order` puts every key in group 1; `sorted` orders by `by_key` and keeps no
ordinal.

## 10. Streaming

### 10.1 The parser

```cpp
wd::parser p(opt);                                  // wd::basic_parser<Traits, Storage> for others
wd::result<void> w = p.write(chunk);                // any split of the text, down to one byte
wd::result<wd::document> d = p.finish();            // the text has ended
p.reset();                                          // ready for another text
```

`parse_options`: `max_depth` (default 256) **(ruling)**, `max_bytes` (the total of every chunk;
default no limit), `max_document_bytes` (the document's image; default the storage's),
`revive_dates`, `revive_bytes`. Boost.JSON's `basic_parser` is fed each chunk (`write_some` with
`more`), and its handler builds the document node by node, by ids, so the document may grow and move
between chunks. A failed `write` leaves the parser failed: every later call returns the same error,
until `reset`. The parser never throws: its input is the network's.

### 10.2 Selectors

```cpp
p.on("$.items[*]", [](wd::const_value item, wd::path_view where) { /* ... */ });  // delivered, then dropped
p.observe("$.meta", [](wd::const_value meta, wd::path_view where) { /* ... */ });  // delivered, then kept
```

A selector names places in the text with a subset of JSONPath **(ruling)**: `$` the root, then any
sequence of `.name`, `['name']` (any key, with `\'` and `\\` escapes), `[n]` (an index, n >= 0),
`[*]` (every element) and `.*` (every member); a malformed selector is `errc::type` from `on`.
A value whose path matches is delivered when it is complete: a scalar when its token ends, an array
or object when it closes. Then:

- `on`: the value is removed from the document being built (its blocks zeroed and freed), so a
  stream of a million elements needs the memory of one at a time;
- `observe`: the value stays.

A value inside another that also matches is delivered first (it completes first); if it was taken by
`on`, the outer value no longer holds it. A callback receives a `const_value` (it cannot change the
document being built) and the path of what it receives; to keep a value, it clones it. A callback
returns `void` or `wd::result<void>`; a failure it returns, or an exception it throws in a build
with exceptions, leaves the parser failed (the exception is rethrown from `write`). Values no
selector matches are built as usual, and `finish` returns the document without what `on` removed.

### 10.3 Writing in pieces

`stringify_to(value, sink, indent)` writes the same bytes as `stringify`, in pieces of at most a
`chunk` size (default 4096) **(ruling)**, to a sink: any callable taking `std::string_view` and
returning `void` or `wd::result<void>`. A failure the sink returns stops the writing and is
returned; an exception it throws propagates; the document is unchanged either way.

## 11. Storage and the binary image

| Storage | The buffer | When the heap is full |
| --- | --- | --- |
| `owned` (default) | the document's own, 16-aligned | grows: a buffer twice the size (at least what the operation needs), the image copied, the heap grown (`rbtree_best_fit::grow`), the operation carried on; up to `max_bytes` (default 1 GiB) **(ruling)**, then `errc::capacity` |
| `external` | a `std::span<std::byte>` its user owns (a browser's memory) | the operation returns `errc::capacity` and changes nothing |

A third storage, a document inside a shared segment of several documents, is part 4's (concurrency)
and not in this version **(ruling)**.

The image: a 64-byte header, then the heap. Every pointer inside is
`offset_ptr<..., std::int64_t, std::uint64_t, 8>`, every size a `std::uint64_t`, so the image works
at any 16-aligned address and has the same bytes on 64-bit native and wasm32 (measured on clang,
gcc-15, wasip2 and Emscripten with this heap: digest `8d80cfaf86a669cb` on all four).

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 8 | magic `"WCPPDOC\0"` |
| 8 | 4 | format version, 1 |
| 12 | 4 | byte-order mark `0x01020304` as the machine writes it |
| 16 | 4 | reserved, zero (an image whose reserved bytes are not zero is refused) |
| 20 | 4 | layout tag: a hash of the node's, the container's and the heap header's sizes and alignments, and of the traits (5.4) |
| 24 | 8 | image size |
| 32 | 8 | the container's offset |
| 40 | 8 | the next node id |
| 48 | 16 | zero |

**Version rule (ruling, 2026-10-12).** The header does not carry the Boost version. Boost releases
often, and the structures the image holds (Boost.Interprocess's `rbtree_best_fit` block headers,
Boost.MultiIndex's ordered index nodes) rarely change; changing the format is webcpp::document's
own decision. The format version is the library's constant, `webcpp::document::format_version`
(`config.hpp`, value 1): the library raises it when the layout changes and provides a migration from
the previous version. A Boost upgrade does not change it. The format test (`test/format_test.cpp`)
watches Boost: it compares the compact image of a fixed document, under default_traits and
js_traits, with golden images committed under `test/fixtures/format/`, on every toolset, and fails
when a release changes one byte; regenerating the goldens is right only together with raising
`format_version`.

```cpp
std::span<const std::byte> bytes = doc.image();
wd::result<wd::document> copy = wd::from_image(bytes);                   // an owned copy
wd::result<wd::basic_document<T, wd::external>> in = wd::open(buffer);   // in place
wd::result<wd::basic_document<T, wd::external>> fresh = wd::format(buffer); // a new, empty image
```

`open` and `from_image` refuse bytes shorter than the header or than the size it states, another
magic (`not_a_document`), and another format version, byte order, nonzero reserved bytes or layout tag
(`incompatible_document`): a mismatch is refused, never read or converted. Then the heap's
`check_sanity` runs. An image is trusted input **(ruling)**: those checks cannot prove every offset
inside the containers sound, so images travel between a program's own processes and peers it trusts;
untrusted data enters as JSON text.

Free memory is zero **(ruling)**: a buffer is zeroed when made or grown, the allocator zeroes a block
before it frees it, and nodes are built in place (`emplace`), never copied from a temporary. So an
erased value leaves none of its bytes in the image, and the image is a function of the operations
alone. `doc.shrink_to_fit()` gives an owned image's free tail back; `doc.compact()` (16.7) makes it as small as its content, in one canonical layout.

`doc.clone()` and the copy constructor give an owned copy (a copy of the image); moving is cheap.

## 12. Failure without exceptions

Every core operation either completes or changes nothing, a full heap included, without throwing:

1. it computes what it will allocate: each node's block (the container's node size, known per build),
   each string's exact size;
2. it probes the heap for each block with the algorithm's non-throwing `allocate` and frees the probes
   at once; if a probe fails, an owned document grows and probes again, an external one returns
   `errc::capacity`;
3. it allocates for real: strings first, then `emplace`; the algorithm is deterministic, so blocks
   just probed and freed are found again;
4. it rearranges with steps that do not allocate: swapping prepared strings in `modify`,
   renumbering ordinals.

Boost.MultiIndex erases an element whose `modify` throws; nothing in step 4 can throw.

## 13. Errors

`errc`, category `webcpp.document`, values fixed and never reused:

| `errc` | When | Exception, with exceptions only | JavaScript |
| --- | --- | --- | --- |
| `syntax` | malformed JSON text (with Boost.JSON's code as its cause and the byte offset) | `syntax_error` | `SyntaxError` |
| `type` | a value of another kind; a write through nothing; invalid text in the encoding | `type_error` | `TypeError` |
| `range` | a number that does not fit or is not exact; an index out of range where JavaScript throws; a date out of range | `range_error` | `RangeError` |
| `capacity` | a full external buffer; an owned document at `max_bytes`; a parser limit | `capacity_error` | none |
| `not_a_document` | bytes that are not an image | `image_error` | none |
| `incompatible_document` | an image of another version, Boost, layout, byte order or traits | `image_error` | none |

The exception types derive from `wd::error`, from `std::runtime_error`, and exist only with
exceptions.

## 14. Tests

lightweight_test programs on native (clang and gcc-15), wasip2, wasip3 and Emscripten, each built
without exceptions, and the sugar's tests with them:

- **`traits_test`**: the scalar concept (accepted and refused types), each codec, `js_traits`,
  `utf16`, every bytes policy, every object policy, an image refused under other traits.
- **`number_test`**: the two forms and their boundaries (`INT64_MIN`, `INT64_MAX`, `INT64_MAX + 1`,
  `UINT64_MAX`, 34 and 35 digits, the exponent range), `get<T>` for every integer width and decimal
  type, JavaScript's formatting on each row of section 7's table.
- **`value_test`**: navigation and the three handle states; reading every kind; `value_or`; `set` of
  every settable type, literals and deep copies; `erase`; `assign_from`; handles across growth and
  after erasure; equality; dates (format, range, `revive_dates`); bytes (policies, `revive_bytes`).
- **`array_test`**: every method of 6.5 on its JavaScript edge cases (negative and past-the-end
  indices, omitted and negative counts, empty arrays), `set_length`, writing past the end.
- **`order_test`**: section 9's rules: index keys, non-index look-alikes, the boundary
  4,294,967,294, assignment keeping the place, erase and re-create; `insertion_order`, `sorted`.
- **`stringify_test`**: escapes, indentation (`JSON.stringify`'s rules: up to 10 spaces, empty
  containers as `{}` and `[]`), numbers, dates, bytes; `stringify_to` gives the same bytes for every
  chunk size from 1, and a failing sink.
- **`parse_test`**: RFC 8259 and `JSON.parse`'s acceptance and rejection (no comments, no trailing
  commas, the whitespace set), repeated keys, depth and size limits, lone surrogates, number limits.
- **`streaming_test`**: every input of the other tests split at every byte gives the same document
  as one write; selectors: matching, nesting, `on` against `observe`, a failing callback, a
  million-element array with `on` whose image stays below a bound.
- **`image_test`**: an image copied to another address opens and is equal; every header defect is
  refused; the same operations at two addresses give the same bytes; an erased secret leaves no byte;
  `shrink_to_fit`; `external` full: `capacity` and the document unchanged, for every mutating
  operation, with the buffer filled to every size from empty to full; `owned` growth through many
  doublings and `max_bytes`.
- **`sugar_test`**: built with exceptions: each throwing member throws the documented type.
- **`no_exceptions_test`**: compile-fail tests: each sugar member called in a build without
  exceptions does not compile, and the diagnostic names `exceptions_enabled`.
- **The node oracle**: a corpus of JSON texts and of scripts of operations (set, erase, push, splice
  and the rest), each run in node and in C++; node's `JSON.stringify` output is the expected file,
  generated by a script and committed, and the lane fails when the C++ output differs, as xstate's
  oracle does with XState. It runs with `default_traits` (numbers within 17 digits) and with
  `js_traits` (every number). The corpus includes the cases above and generated random documents and
  operation sequences.

## 15. Out of scope

`toJSON`, replacer and reviver; Map, Set, RegExp; BigInt and fixed 256-bit integers (the owner, 2026-10-12:
neither; `decimal128` holds 34-digit integers exactly, and Boost.Multiprecision's `int256_t` is 48
bytes, not trivially copyable, with bytes that differ between compilers and targets, measured); JSON Pointer and JSONPath beyond section
10.2's selectors; schemas (json_schema's); a document inside a shared segment, concurrency and locks
(part 4); the commit and durability (part 2, `~/.buzz/RESEARCH/WEBCPP_SEGMENT_PERSISTENCE.md`);
moving xstate and trystero from Boost.JSON to webcpp::document (their own plans, after this
library).

## 16. Streams of documents

The owner's request (2026-10-12): a stream of bytes on one side, documents delivered on the other as
the bytes are consumed, with examples for the library's main uses: synchronous reading; asynchronous
reading with Boost.Asio, with Boost.Cobalt, and with any executor (xactor's driver); NDJSON; indented
or concatenated JSON that respects no separator (`{"a":1}    {"b":2}`); and JavaScript feeding JSON to
a C++ function compiled with Emscripten that delivers documents.

### 16.1 The parser of a stream

```cpp
enum class framing : std::uint8_t {
    concatenated,  // documents one after another, any JSON whitespace (or none) between them:
                   // indented JSON and `{"a":1}    {"b":2}` alike; no resynchronisation
    lines,         // NDJSON / JSON Lines: one document per line; a raw newline ends a document
    rfc7464,       // JSON text sequences: each document after 0x1E and before a newline
};
enum class on_error : std::uint8_t {
    stop,  // the first error ends the stream (every framing)
    skip,  // lines: the rest of the line is dropped; rfc7464: up to the next 0x1E; not allowed
           // with concatenated, which has no point to resynchronise at
};
struct stream_options : parse_options {
    webcpp::document::framing framing = framing::concatenated;
    webcpp::document::on_error on_error = on_error::stop;
};

template <class Traits = default_traits> class basic_stream_parser {
public:
    explicit basic_stream_parser(stream_options o = {});
    result<std::size_t> write(std::string_view bytes);            // consumes every byte; returns
                                                                   // the documents (and skipped
                                                                   // records) it completed
    result<std::optional<basic_document<Traits>>> next();         // the oldest completed, if any
    template <class F> result<std::size_t> write(std::string_view bytes, F&& on_document);
                                                                   // push style: f(document&&)
                                                                   // for each, in order
    result<std::size_t> finish();                                  // the end of the stream: closes
                                                                   // a trailing top-level number or
                                                                   // a last line without newline
    template <class F> result<std::size_t> finish(F&& on_document);
    void reset();
    std::uint64_t line() const noexcept;       // 1-based line of the last byte consumed
    std::uint64_t offset() const noexcept;     // bytes consumed in the stream
    std::uint64_t documents() const noexcept;  // documents delivered so far
};
using stream_parser = basic_stream_parser<>;
```

- Each document is parsed exactly as `parse` parses one text (key order, numbers, limits per
  document: `max_depth`, `max_document_bytes`), into a new owned document.
- `lines`: a document ends at its line's `\n`; `\r` and other JSON whitespace around it are ignored;
  an empty line is ignored; two documents on one line, or a document not complete at its `\n`, is
  `errc::syntax` for that line. The last line may end without `\n` (closed by `finish`).
- `rfc7464`: a record starts at 0x1E and ends before the next 0x1E or at its `\n`; a document may span
  lines (indented); bytes before the first 0x1E are an error.
- `concatenated`: a document ends where its value closes; whitespace (or nothing, after a container or
  a string) separates documents; a top-level number is closed by the next byte that cannot continue
  it, or by `finish`.
- With `on_error::skip`, a bad record is dropped and reported in order: `next()` (or the push form)
  yields an error whose `line()` names it, then the stream goes on; with `stop` the error is sticky
  until `reset`.
- Without Asio: the parser is sans-IO, so it serves any source (a file, a socket read by hand, an
  actor's messages, JavaScript through Emscripten).

### 16.2 Asio

`<webcpp/document/asio.hpp>`, native only (it is the one header including Boost.Asio; examples and
tests that use it are built natively):

```cpp
template <class AsyncReadStream, class DynamicBuffer, class Traits, class CompletionToken>
auto async_read_document(AsyncReadStream& stream, DynamicBuffer& buffer,
                         basic_stream_parser<Traits>& parser, CompletionToken&& token);
// completes with (boost::system::error_code, basic_document<Traits>): the next document, reading
// from the stream only while the parser has none ready; asio::error::eof at the stream's end after
// the parser's finish delivered what it held; a skipped record completes with its error and the
// stream goes on. Any completion token: a callback, asio::use_awaitable, asio::deferred,
// boost::cobalt::use_op.
```

### 16.3 Examples (the library's main uses)

| Example | What it shows |
| --- | --- |
| `document_stream_sync.cpp` | reading a stream by hand: NDJSON, then indented and concatenated JSON (`{"a":1}    {"b":2}`), then rfc7464 with a bad record skipped |
| `document_stream_asio.cpp` | an Asio TCP client and server on localhost: the server writes NDJSON in odd-sized pieces, the client reads documents with `async_read_document` in a coroutine (`use_awaitable`) and with a callback |
| `document_stream_cobalt.cpp` | the same reading with Boost.Cobalt (`cobalt::use_op`) |
| `document_stream_xactor.cpp` | an xactor actor that receives byte chunks as messages and sends each document to another actor, run by xactor's deterministic driver |
| `document_stream_js.cpp` and its `.mjs` | JavaScript (Node) calls a C++ function compiled with Emscripten, chunk by chunk, with several JSON texts; C++ parses and delivers documents |

### 16.4 Images on the wire, between processes and between threads

The owner's request (2026-10-12): examples of reading, from a socket or any asynchronous source,
documents sent as their binary image by a trusted source; examples with Boost.Interprocess; and
examples of native programs exchanging documents between threads through a queue.

An image frames itself: its 64-byte header states its size (offset 24), so a stream of images is the
images one after another, with nothing between them. A reader reads 64 bytes, checks the header before
reading on (magic, format version, byte order, reserved bytes, layout tag of the reader's traits, a size
within the block it reads into), reads the rest into the same block, and opens the document in place:
no parsing, no copy beyond the read itself, no allocation per document. Images are trusted input
(section 11): this path is for peers the program trusts; untrusted data enters as JSON text.

`<webcpp/document/image_stream.hpp>` (every target, sans-IO):

```cpp
class image_pool;                 // fixed blocks, 16-aligned, allocated once: block_bytes x blocks;
                                  // not thread-safe; a full pool is errc::capacity (backpressure)
template <class Traits = default_traits> class leased_document;
                                  // an external document opened in a pool block, owning its lease:
                                  // move-only; the block returns to the pool when it is destroyed;
                                  // reads and writes as a document does (root(), operator[], image())
template <class Traits = default_traits> class basic_image_reader {
public:
    explicit basic_image_reader(image_pool& pool);
    result<std::size_t> write(std::span<const std::byte> bytes);   // sans-IO: copies into blocks
    result<std::optional<leased_document<Traits>>> next();
    template <class F> result<std::size_t> write(std::span<const std::byte> bytes, F&& on_document);
    result<void> finish();                                         // an image cut short is an error
};
```

`<webcpp/document/asio.hpp>` adds, native only:

```cpp
template <class AsyncReadStream, class Traits, class CompletionToken>
auto async_read_image(AsyncReadStream& stream, image_pool& pool, CompletionToken&& token);
// completes with (error_code, leased_document<Traits>): the header read into a block, checked, the
// rest read straight into the same block (no intermediate buffer), opened in place
template <class AsyncWriteStream, class Traits, class Storage, class CompletionToken>
auto async_write_image(AsyncWriteStream& stream, const basic_document<Traits, Storage>& document,
                       CompletionToken&& token);   // the image as it is, in one gathered write
```

Examples, native only, beside section 16.3's:

| Example | What it shows |
| --- | --- |
| `document_image_asio.cpp` | a TCP server sending documents' images and a client reading them with `async_read_image` into a pool, opened in place, in a coroutine and with a callback; and the sans-IO `image_reader` fed by hand from any source |
| `document_image_interprocess.cpp` | Boost.Interprocess: a producer process builds documents directly inside blocks of a shared memory segment (external storage) and a consumer process opens them in place, with no copy; the handoff through an interprocess message queue of block offsets |
| `document_threads.cpp` | native threads exchanging documents through a queue: moving a `wd::document` (its 40-byte object; the image does not move), and passing images through a ring of pool blocks |

### 16.5 Sealed images

The owner's decision (2026-10-12): an image may carry a checksum. The header's reserved bytes (offset
48) hold it: 8 bytes, zero for an image that is not sealed, else the CRC-32C (Castagnoli, Boost.CRC's
`crc_optimal<32, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true>`, header-only) of the whole image
computed with those 8 bytes zero, stored with a high bit that marks the image sealed.

- `doc.seal()` writes it (a pass over the image); any later change to the document clears it (the
  image is then simply not sealed, never wrongly sealed).
- `open`, `from_image` and the image reader verify a sealed image's checksum (a pass over the image)
  and refuse a mismatch with a new `errc::damaged_document` (value 7, with its exception
  `image_error`); an image that is not sealed opens as before. `open(buffer, verify::none)` skips the
  pass for a program that trusts its memory.
- `async_write_image` seals before it writes.
- What it catches: bytes changed inside an image (a flipped bit on a disk or a wire, a torn write, a
  stray write). What it does not: an old but valid image in reused memory (it carries a valid
  checksum); a program that needs that tells generations apart itself.

### 16.6 Shared documents

The owner's decisions (2026-10-12): a reference count lives in the header of the block that holds an
image, never in the image (the image stays the document's content, the same bytes wherever it lives,
and a count travelling inside it would mean nothing elsewhere).

- `shared_pool`: a pool whose blocks each start with a block header (an address-free, lock-free
  `std::atomic<std::uint64_t>` count, the image's size) followed by the image; the pool's memory is
  the program's: the process heap, or a region of a Boost.Interprocess shared memory segment mapped by
  several processes (the pool then works through offsets from the region, never addresses).
- `shared_document<Traits>`: as `std::shared_ptr`: copying adds a reference (atomically), destroying
  drops one; the last reference zeroes the block and returns it to the pool. It reads and writes as a
  document does: whether several holders may write at once is the program's decision and its
  synchronisation, not the library's (the owner's ruling: the process controls the memory).
- `make_mutable()`: a document this holder alone may change: the same block when it holds the only
  reference, else a copy into a new block (copy on write), leaving the other holders' document as it
  was.
- Limit, documented: a process that dies holding references leaves its blocks counted forever (as
  Boost.Interprocess's shared_ptr does); recovering them (who holds what, per process) is the
  allocators stage's concurrency part.
- Examples: the threads example passes `shared_document`s through its queue (N readers, no copy); the
  Interprocess example's producer builds documents in a shared segment's pool and consumer processes
  hold them by `shared_document`, the last one freeing the block.

### 16.7 Compact images

The owner's request (2026-10-12): an image sent on the wire should carry no waste, and two documents
with equal content should have equal bytes. Today `shrink_to_fit()` only trims the free tail after
the last block in use (section 11): the holes left by erases, by the order in which the parser
allocates, or by growth stay inside, and ids, insertion numbers and the heap's layout record the
history of the edits, not the content.

API (`basic_document<Traits, Storage>`):

```cpp
result<void> compact() noexcept;                                    // owned only
result<basic_document<Traits, external>> compact_into(std::span<std::byte> buffer) const noexcept;
std::uint64_t compact_size() const noexcept;                        // the exact bytes of the copy
```

What a compact image is. The tree is walked once in document order (the traits' own order: for
`sorted` objects by key, otherwise by the index `by_order`: elements by index, array-index keys by
index, then the other members as inserted), depth first, parents before their children, and a copy is
built in a fresh image:

- ids are renumbered from the first, in the order of the walk (the root is 1); `next_id` is the node
  count plus one;
- an insertion number is renumbered from 0 in the order of the walk (under `sorted`, as always, it is
  the node's id); an object's "next insertion number" is the count of its non-index members, and the
  key count and array lengths are the content's;
- for each node, in the walk's order, its key, its text, then the node are allocated in the fresh heap,
  which has no free block until the end: the blocks lie one after another, with no hole and no free
  block, so the heap's own bookkeeping is a function of the content alone;
- the image is sized to exactly the blocks it holds, which is the smallest size the allocator
  accepts: `shrink_to_fit()` of a compact image changes nothing.

Guarantees:

- Determinism: two documents whose content is equal (the same keys in the same JavaScript order, the
  same values of the same representation) compact to byte-identical images, whatever edits made them
  (parsed, built by sets, erases, splices, values replaced by longer and shorter strings, an
  insertion that failed for want of room), on the same target and traits. The image is the same on
  64-bit native and wasm32, as every image is (section 11).
- Idempotence: the compact image of a compact image is the same bytes.
- The content is unchanged: `stringify` writes the same text, `==` holds, key order is kept.
- The image is an image: `open`, `from_image` and the image reader read it, and it seals.
- A seal is not kept: the bytes changed, so the compact document is not sealed (`sealed()` is false);
  sealing after `compact()` is the caller's.

`compact()` (owned storage only) builds the copy in a new buffer of `compact_size()` bytes, then
swaps it in. Strong guarantee: on an error, `errc::capacity` (the new buffer cannot be allocated, or
exceeds `max_bytes`, which it cannot for a document that fits) or `errc::type` (a moved-from
document), the document is unchanged. Like a vector's reallocation it invalidates every handle,
every `std::string_view` read from a value and every iterator. The document's `max_bytes` is kept;
`capacity()` is `compact_size()` afterwards, so the document is full and the next growth moves the
image as usual (`reserve` first when more edits follow).

`compact_into(buffer)` writes the compact copy of any document, owned or external, into the
caller's buffer, which must be 16-aligned and must not overlap this document's image, and returns an
external document over its first `compact_size()` bytes; the rest of the buffer is zeroed. The
source is untouched. `errc::capacity` when the buffer is not 16-aligned or is smaller than
`compact_size()`, in which case the buffer is not written; `errc::type` for a moved-from document or
an overlapping buffer. It is for the documents that cannot reallocate, such as those leased from a
pool or held in a shared block: the program sizes the block with `compact_size()`, or tries a block
and takes a refusal for "too large".

`compact_size()` is exact, not an estimate, and costs one pass over the nodes that allocates and
writes nothing (a fraction of the copy). Each block's size is computed from `rbtree_best_fit`'s own
arithmetic (the bytes of the smallest block, of a payload rounded to the alignment), and the sizes of
the heap's fixed part, of the container's blocks and of a node's are measured once per call in a
scratch heap on the stack, so that the figure follows the Boost release of the build, whose layout the format test pins, and no
constant is repeated. A test compares it with the size of every compact image it builds.

Cost: time linear in the nodes (the walk is O(n log n) in the three indices, as building the tree is);
memory, at the peak, the source image plus the compact one (the copy is built before the swap, which
is what makes the guarantee strong), and a stack of the walk's depth in the process heap.
Not in place: an in-place compaction would move blocks the handles point into and could not keep
the strong guarantee.
