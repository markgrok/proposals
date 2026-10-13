## Purpose

webcpp::document is a header-only C++20 library of webcpp's own: a JSON document whose behaviour is
JavaScript's (what `JSON.parse`, property access, assignment, `delete`, the array methods and
`JSON.stringify` do, with the same key order and the same text out), exact in decimal, whose tree
lives in one relocatable binary image that can be copied, sent, opened in place and shared, and
whose core never throws. It reads and writes JSON in streams, runs natively, on wasip2, wasip3 and
Emscripten, and is also a JavaScript module and an npm package. These requirements describe 0.1.0
as built (`reference/source/`).

## ADDED Requirements

### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::document`, with internals in `webcpp::document::detail`, public headers under `<webcpp/document/...>` (one per responsibility), macros prefixed `WEBCPP_DOCUMENT_` (test-only macros `WEBCPP_TEST_DOCUMENT_`) and include guards `WEBCPP_DOCUMENT_<PATH>_HPP` (`WEBCPP_DOCUMENT_HPP`, `WEBCPP_DOCUMENT_DETAIL_HEAP_HPP`). It ports nothing (`"port-of": null` in `meta/libraries.json`): its evidence is JavaScript itself, run as an oracle in Node. The examples and the page write the namespace as `namespace wd = webcpp::document;`.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/document//document` and includes `<webcpp/document.hpp>`
- **THEN** it compiles, and only the compiled parts of Boost.JSON and Boost.Charconv (which `build.jam` names) are built for it

#### Scenario: The namespace alias of the examples
- **WHEN** `example/document_quick_start.cpp` writes `namespace wd = webcpp::document;` and calls `wd::parse`
- **THEN** it names `webcpp::document::parse`

### Requirement: Library: Public headers and the convenience header
The library SHALL ship these public headers under `include/webcpp/document/`: `asio.hpp`, `config.hpp`, `containers.hpp`, `document.hpp`, `errors.hpp`, `image.hpp`, `image_stream.hpp`, `kind.hpp`, `literal.hpp`, `number.hpp`, `parse.hpp`, `parser.hpp`, `shared.hpp`, `stream.hpp`, `stringify.hpp`, `traits.hpp` and `value.hpp`, and the convenience header `include/webcpp/document.hpp`, which MUST include `config.hpp` first and then every one of those but `asio.hpp`. `asio.hpp` is the one header that includes Boost.Asio, which builds natively only, so a program that uses it includes it by name and is built natively. Every public header includes `<webcpp/document/config.hpp>` first. The headers under `detail/` (`convert.hpp`, `fwd.hpp`, `handler.hpp`, `heap.hpp`, `selector.hpp`, `tree.hpp`, `utf.hpp`, `value_array.hpp`, `value_sugar.hpp`, `writer.hpp`) are internals, named but not frozen.

#### Scenario: The convenience header leaves Asio out
- **WHEN** a program includes only `<webcpp/document.hpp>` and is built for wasm32-wasip2
- **THEN** it compiles, and no header of Boost.Asio is included

#### Scenario: Asio is included by name
- **WHEN** a native program includes `<webcpp/document/asio.hpp>` and calls `wd::async_read_document`
- **THEN** it compiles; the same include on a WASI or Emscripten toolset is not built, since every program that includes it is declared `native`

### Requirement: Build: The alias and its usage requirements
`build.jam` SHALL declare `project /webcpp/document` and one alias `document` whose usage requirements are `<include>include`, `<library>/boost//headers`, `<library>/webcpp/config//config`, `<library>/webcpp//boost_json` and `<library>/webcpp//boost_charconv`. Boost.JSON's definitions are linked because the library uses its parser (`boost::json::basic_parser`), which is not header-only; Boost.Charconv's because doubles are read and written through its `from_chars` and `to_chars`. A b2 project uses the library through `/webcpp/document//document`.

#### Scenario: One alias brings everything
- **WHEN** a program names `/webcpp/document//document`
- **THEN** document's include path, Boost's headers, webcpp's configuration, and the compiled definitions of Boost.JSON and Boost.Charconv reach its compile and link lines

### Requirement: Build: Boost libraries used and never used
The library SHALL need Boost 1.92 or newer and use these parts of it: Interprocess (`offset_ptr.hpp`, `mem_algo/rbtree_best_fit.hpp` and `sync/null_mutex.hpp` only), MultiIndex, Decimal, System (`result`, `error_code`), JSON (`basic_parser` only), Charconv, CRC and Core's `lightweight_test` in the tests. It MUST NOT include `boost/interprocess/managed_external_buffer.hpp`, `segment_manager.hpp`, `allocators/allocator.hpp` or `sync/mutex_family.hpp`, which throw bare and do not compile without exceptions. Optional parts: Boost.Asio (`asio.hpp`), Boost.Cobalt and Boost.Beast (examples), xactor (one example), Emscripten and Node (the JavaScript module and its lane).

#### Scenario: The heap compiles without exceptions
- **WHEN** a test is built with `exception-handling=off`
- **THEN** it compiles, since the heap uses `offset_ptr`, `rbtree_best_fit` and `null_mutex` through `detail::no_mutexes` and not `mutex_family.hpp`

### Requirement: Build: Targets
The library SHALL build and pass its tests and examples on four targets: native (Clang and GCC; the library carries portability work for Apple Clang, MSVC and Clang before 20), Emscripten (run by Node), wasm32-wasip2 and wasm32-wasip3 (run by wasmtime). `test/Jamfile`, `example/Jamfile` and `bench/Jamfile` declare them with `webcpp.targets native emscripten wasip2 wasip3`; a program that needs Boost.Asio, threads, shared memory, Boost.Cobalt or Boost.Beast is declared `native`, and `document_stream_xactor.cpp` is declared `native wasip2 wasip3`.

#### Scenario: The target lists
- **WHEN** `test/Jamfile` is read
- **THEN** it declares `webcpp.targets native emscripten wasip2 wasip3`, and `stream_asio`, `image_asio`, `shared_interprocess` and the headers-alone test of `asio.hpp` are declared `native`

### Requirement: Library: No hidden state and no forbidden constructs
The library SHALL use no thread, no clock, no file and no global or static mutable state, no hashed container anywhere (keys come from untrusted text), no `float` or `double` outside `js_traits` and its codec, and no `throw` in `include/`: an exception is raised through `boost::throw_exception`, inside `#ifndef WEBCPP_DOCUMENT_NO_EXCEPTIONS`. The repository's lint refuses a `throw` in a public header. The tests that count allocations or threads are the programs' own instruments, in their own files.

#### Scenario: The core compiles without exceptions
- **WHEN** `exception-handling=off` is asked for every test and example but those that declare `<exception-handling>on`
- **THEN** they compile and pass, and `document_no_exceptions.cpp` declares `<exception-handling>off` whatever the build asks

### Requirement: Config: WEBCPP_DOCUMENT_NO_EXCEPTIONS
`config.hpp` SHALL include `<webcpp/config.hpp>` first and define `WEBCPP_DOCUMENT_NO_EXCEPTIONS` exactly when webcpp has no exceptions (`WEBCPP_HAS_EXCEPTIONS` is not defined: the compiler has none, or the developer defined `WEBCPP_NO_EXCEPTIONS` for every webcpp library), and MUST keep the macro when a developer defines it for document alone in a build that has exceptions. The macro removes only what throws (the sugar and the exception types): the core is the same in both builds. Under `__MRDOCS__` the header defines and at once undefines the macro, so that the reference lists the APIs that throw.

#### Scenario: The three ways the macro is defined
- **WHEN** `config_test.cpp` is built as the build asks (`config`), with `WEBCPP_NO_EXCEPTIONS` (`config_all_off`) and with `WEBCPP_DOCUMENT_NO_EXCEPTIONS` on the command line (`config_developer_defined`)
- **THEN** the first has the macro exactly when `WEBCPP_HAS_EXCEPTIONS` is undefined, and the other two have it; each compiles

### Requirement: Config: The constraint of every member that throws
`detail::exceptions_on` SHALL be `false` when `WEBCPP_DOCUMENT_NO_EXCEPTIONS` is defined and `true` otherwise, and the concept `detail::exceptions_enabled<T>` SHALL be `exceptions_on && sizeof(T) != 0`, so that it depends on `T`, is checked where a member is called, and a build without exceptions reports a call by this name.

#### Scenario: The concept follows the macro
- **WHEN** `config_test.cpp` is compiled
- **THEN** `static_assert(!detail::exceptions_enabled<int>)` holds with the macro defined and `static_assert(detail::exceptions_enabled<int>)` without it

### Requirement: Config: WEBCPP_DOCUMENT_NO_DANGLING
`config.hpp` SHALL define `WEBCPP_DOCUMENT_NO_DANGLING` as `[[gnu::no_dangling]]` on GCC 14 and later and as nothing elsewhere, and the handles, the ranges of a container's children and the children iterators SHALL carry it, so that GCC's `-Wdangling-reference` does not take `for (auto key : doc["a"].keys())` for a dangling reference.

#### Scenario: A range of a temporary handle
- **WHEN** a program writes `for (auto key : doc["a"].keys()) {}` and is built with GCC 14 and `-Wdangling-reference`
- **THEN** it compiles without that warning

### Requirement: Config: The format version
`config.hpp` SHALL define `inline constexpr std::uint32_t format_version = 1` in `webcpp::document`: the version of the image's format that the library writes and opens. It is the library's constant, not an option: the library raises it when the layout of an image changes and then provides a migration from the previous version; a new Boost release does not change it.

#### Scenario: The version a header carries
- **WHEN** a document's image is read
- **THEN** the 4 bytes at offset 8 hold `format_version`, which is 1

### Requirement: Errors: The errc values
`errc` SHALL be an `enum class errc : int` whose values start at 1 and are fixed and never reused: `syntax` = 1 (malformed JSON text, JavaScript's `SyntaxError`), `type` = 2 (a value of another kind, a write through nothing, text invalid in the encoding; `TypeError`), `range` = 3 (a number that does not fit or is not exact, an index out of range where JavaScript throws, a date out of range; `RangeError`), `capacity` = 4 (no room left: a full external buffer, an owned document at its limit, a parser's limit), `not_a_document` = 5 (bytes that are not an image), `incompatible_document` = 6 (an image of another format version, nonzero reserved bytes, layout, byte order or traits) and `damaged_document` = 7 (a sealed image whose bytes do not match its seal). `boost::system::is_error_code_enum<errc>` SHALL be `std::true_type`.

#### Scenario: The values are fixed
- **WHEN** `errors_test` casts each enumerator to `int`
- **THEN** `syntax` is 1, `type` 2, `range` 3, `capacity` 4, `not_a_document` 5, `incompatible_document` 6 and `damaged_document` 7

#### Scenario: A result holds an error
- **WHEN** `const wd::result<int> r = wd::errc::range;` is made
- **THEN** `r.has_error()` is true and `r.error() == wd::errc::range`

### Requirement: Errors: The category and its messages
The category of every `errc` SHALL be a constant `error_category()` with the fixed identity `0x646f63756d656e74` (the ASCII of "document"), holding no mutable state, whose `name()` is "webcpp.document" and whose messages are: `syntax` "malformed JSON text", `type` "a value of another kind", `range` "a number or index out of range", `capacity` "no room left", `not_a_document` "not a document's image", `incompatible_document` "an image of another format, layout, byte order or traits", `damaged_document` "a sealed image whose bytes do not match its seal", and "unknown webcpp.document error" for any other value. `make_error_code(errc)` SHALL make the `error_code` of an `errc`, and an `errc` converts to an `error_code` and compares with one.

#### Scenario: The messages
- **WHEN** `errors_test` asks `boost::system::error_code(wd::errc::capacity).message()` and `wd::error_category().message(8)`
- **THEN** they are "no room left" and "unknown webcpp.document error", and `wd::error_category().message(0)` is the unknown message too

### Requirement: Errors: A message copied into a caller's buffer
The category's `message(value, buffer, size)` SHALL be `noexcept`, copy the message cut to fit `size` bytes with its terminating zero, never write past `size`, write nothing for a null buffer or a size of 0, and return the whole message whatever the buffer.

#### Scenario: A buffer of three bytes
- **WHEN** `category.message(static_cast<int>(wd::errc::capacity), buffer, 3)` is called on a buffer filled with `x`
- **THEN** it returns "no room left", the buffer holds "no" and its terminating zero, and `buffer[3]` is still `x`

### Requirement: Errors: Every fallible operation returns a result
`result<T>` SHALL be an alias of `boost::system::result<T>` (`void` for an operation that returns none). The core never throws: every operation of it that can fail returns one, with an `errc`, or with the error a sink or a selector's callback of the user's returned, passed on unchanged in its own category. The standard library's `std::bad_alloc` on the process heap (the text `stringify` returns, the key a handle of a missing member holds, the parser's frames) is the one failure that is not a value.

#### Scenario: A failure is a value
- **WHEN** `wd::parse("[1,")` fails
- **THEN** the result holds an error with `errc::syntax`, and the program goes on

### Requirement: Errors: The exception types
With exceptions only (`WEBCPP_DOCUMENT_NO_EXCEPTIONS` not defined), the library SHALL define `error` (derived from `std::runtime_error`, holding the `error_code`, with `code()`), `syntax_error` (with `offset()`, the byte offset of the failure in the text), `type_error`, `range_error`, `capacity_error` and `image_error`, each derived from `error`; and `detail::raise(code, offset)` SHALL throw, through `boost::throw_exception`, `syntax_error` for `syntax`, `type_error` for `type`, `range_error` for `range`, `capacity_error` for `capacity`, `image_error` for `not_a_document`, `incompatible_document` and `damaged_document`, and `error` for a code of another category.

#### Scenario: Raise throws the type of its code
- **WHEN** `errors_raise` calls `detail::raise` with each `errc` and offset 17
- **THEN** `syntax` throws `syntax_error` with offset 17 and message "malformed JSON text", `type` throws `type_error`, `range` `range_error`, `capacity` `capacity_error`, and `not_a_document`, `incompatible_document` and `damaged_document` throw `image_error`

### Requirement: Kinds: kind, undefined and null
`kind` SHALL be an `enum class kind : std::uint8_t` with fixed values: `undefined` = 0 (never stored; what a handle reads when nothing is there), `null` = 1, `boolean` = 2, `number` = 3, `string` = 4, `date` = 5 (a `Date`), `bytes` = 6 (a `Uint8Array`), `array` = 7 and `object` = 8. `kind.hpp` SHALL also define `undefined_t` and the constant `undefined` (which `set` takes to erase a member and which stores `null` in an element), and `null_t` and the constant `null` (which `set` stores, as `nullptr` does).

#### Scenario: The values are fixed
- **WHEN** `errors_test` casts each `kind` to `int`
- **THEN** they are 0 to 8 in the order listed

### Requirement: Traits: default_traits and js_traits
`default_traits` SHALL have `boolean = bool`, `integer = std::int64_t`, `number = boost::decimal::decimal128_t`, `string = utf8`, `date = date_ms` (`std::chrono::sys_time<std::chrono::milliseconds>`), `bytes = base64`, `object = javascript_order` and `array = dense`; `js_traits` SHALL derive from it and change `number` to `double`, `string` to `utf16` and `bytes` to `javascript_bytes`. A program's traits derive from `default_traits` and change what they need.

#### Scenario: A traits of the program's own
- **WHEN** `struct insertion_traits : wd::default_traits { using object = wd::insertion_order; };` is used as `wd::basic_document<insertion_traits>`
- **THEN** it satisfies `document_traits` and its objects keep their keys in insertion order

### Requirement: Traits: The document_traits concept
The concept `document_traits<T>` SHALL require `T::boolean` to be `bool`, `T::integer` to be `std::int64_t`, `T::date` to be `date_ms`, `T::number` to be a `stored_scalar` with a `number_codec`, `T::string` to be `utf8` or `utf16`, `T::bytes` to be `base64`, `base64url` or `javascript_bytes`, `T::object` to be `javascript_order`, `insertion_order` or `sorted`, and `T::array` to be `dense`; `basic_document` MUST `static_assert` it, so that a document of traits that do not satisfy it does not compile.

#### Scenario: Traits that do not satisfy
- **WHEN** a program names `wd::basic_document<T>` with a `T` whose `boolean` is `int`
- **THEN** it fails to compile with "the traits do not satisfy document_traits"

### Requirement: Traits: Scalar codecs
A `stored_scalar` SHALL be a trivially copyable type with a `scalar_codec<T>` that has `name`, `size` (1, 2, 4, 8 or 16 bytes) and `store`/`load` between the value and its bytes in a node, written least significant byte first so that the bytes are the same on every target. The library SHALL provide codecs for `bool` ("bool", one byte, 0 or 1), `std::int64_t` ("int64", 8), `boost::decimal::decimal128_t` ("decimal128", 16: its IEEE 754 BID bits, the halves on either byte order), `double` ("binary64", 8) and `date_ms` ("date-ms", 8: its milliseconds as an `int64_t`).

#### Scenario: A codec round-trips
- **WHEN** `traits_test` stores and loads a value with each codec
- **THEN** the value is read back exactly and `size` is as listed; a type without a `scalar_codec` is no `stored_scalar`

### Requirement: Traits: Number codecs
`number_codec<T>` SHALL give, for the traits' `number` type: `stores_non_finite`, `parse(json_number)`, `write(v)` (a `number_text`), `integral(v)` (an `int64_t` when it is one), `from_int64`, `finite`, `digits` and `from_digits`. The codec of `decimal128_t` is exact and refuses NaN and infinities (`stores_non_finite` is false); it reads through `scan_number`, so more than 34 significant digits or an exponent outside the type's range is `errc::range`. The codec of `double` rounds to the nearest double as `JSON.parse` does (`1e400` is an infinity, a value too small is a zero, text outside JSON's grammar is `errc::syntax`), writes the shortest digits that read back, and writes NaN and the infinities as `null`.

#### Scenario: The double codec
- **WHEN** the codec of `double` parses `1e400` and writes the result
- **THEN** the value is an infinity and `write` gives the text `null`

### Requirement: Traits: Identity is part of the image
`traits_tag<Traits>()` SHALL be an FNV-1a hash (32 bits) of each scalar codec's name and size (`boolean`, `integer`, `number`, `date`) and then of the names of the `string`, `bytes`, `object` and `array` policies, each followed by a zero byte; `detail::layout_tag<Traits>()` SHALL hash that tag with the sizes and alignments of the node, the container and the heap, and the image's header carries it, so that two traits that store or order differently have different tags and an image of other traits is refused with `errc::incompatible_document`, never read wrongly.

#### Scenario: Another traits' image
- **WHEN** an image written under `default_traits` is opened as `wd::open<wd::js_traits>(buffer)`
- **THEN** the result is `errc::incompatible_document` with the message "an image of another format, layout, byte order or traits"

### Requirement: Traits: Encodings and policy names
The library SHALL define the encodings `utf8` (name "utf8") and `utf16` ("utf16"); the bytes policies `base64` ("base64", RFC 4648 with padding), `base64url` ("base64url", RFC 4648 section 5, without padding) and `javascript_bytes` ("javascript", what `JSON.stringify` writes for a `Uint8Array`, `{"0":1,"1":2}`); the object policies `javascript_order` ("javascript"), `insertion_order` ("insertion") and `sorted` ("sorted"); and the array policy `dense` ("dense"), the only one. Each has a static `name`, used in the layout tag.

#### Scenario: The names
- **WHEN** `traits_test` reads `wd::utf16::name`, `wd::base64url::name` and `wd::sorted::name`
- **THEN** they are "utf16", "base64url" and "sorted"

### Requirement: Numbers: decimal_digits and number_text
`decimal_digits` SHALL describe a finite decimal as JavaScript's Number::toString sees it: its value is `0.d1 d2 ... dk` times ten to the power `point`, with `negative`, up to 34 `digits` (`'0'` to `'9'`), `count` (`k`) and `point` (`n`); zero has `count` 0, `point` 0 and `negative` false; any other value has between 1 and 34 digits, the first and the last of them not `'0'`, so that every number has exactly one `decimal_digits` (`1.5`, `1.50` and `15e-1` are all `{false, "15", 2, 1}`). `number_text` SHALL hold at most 64 characters by value (`chars`, `size`, `view()`), so that writing a number or a date allocates nothing.

#### Scenario: One form for a number
- **WHEN** `scan_number` reads `1.50`, `1.5` and `15e-1`
- **THEN** each gives `negative` false, digits "15", `count` 2 and `point` 1

### Requirement: Numbers: scan_number reads a number's text exactly
`scan_number(json_number)` SHALL read the text of a JSON number, `-? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?`, into its digits, never through a double, dropping leading and trailing zeros, and be `noexcept`. Text outside the grammar (`""`, `-`, `+1`, `01`, `-01`, `1.`, `.5`, `1e`, `1e+`, `1.e5`, `0x10`, `1 `, ` 1`, `1e5.0`, `NaN`, `Infinity`, `--1`, `1-`) SHALL be `errc::syntax`; a 35th significant digit, or a nonzero number whose exponent or point leaves `std::int32_t`, SHALL be `errc::range`; and zero SHALL be zero whatever its exponent.

#### Scenario: The edges of the text
- **WHEN** `number_test` scans and writes `-0`, `0e0`, `1E+2`, `1.50`, `1234567890123456789012345678901234`, `12345678901234567890123456789012345` and `1e2147483648`
- **THEN** the writes are `0`, `0`, `100`, `1.5` and `1.234567890123456789012345678901234e+33`, and the last two are `errc::range`

#### Scenario: Zero whatever its exponent
- **WHEN** `0e2147483648` and `-0.000e-99999999999` are scanned
- **THEN** both give zero, with no digits and no sign

#### Scenario: The exponent and the point together
- **WHEN** `1e2147483646`, `10e2147483646` and `0.01e-2147483648` are scanned
- **THEN** the first writes `1e+2147483646` and the other two are `errc::range`

### Requirement: Numbers: write_javascript is Number::toString
`write_javascript(d)` SHALL write a number as ECMA-262's Number::toString, radix 10, with `k` significant digits and the decimal point after `n` of them: when `k <= n <= 21` the digits then `n - k` zeros; when `0 < n <= 21` the digits with a point after `n` of them; when `-6 < n <= 0` `0.`, then `-n` zeros, then the digits; otherwise one digit, a point and the rest if any, `e`, the sign and `n - 1`. Zero is `0` and a negative number is led by `-`.

#### Scenario: Each row of the table
- **WHEN** `number_test` writes digits `("1", 21)`, `("1", 22)`, `("12345", 3)`, `("1", -5)`, `("1", -6)`, `("1234", -6)` and `("17976931348623157", 309)`
- **THEN** the texts are `100000000000000000000`, `1e+21`, `123.45`, `0.000001`, `1e-7`, `1.234e-7` and `1.7976931348623157e+308`

#### Scenario: Exact where a double rounds
- **WHEN** digits `("123456789012345678", 21)` and `("1234567890123456789012345678901234", 6144)` are written
- **THEN** the texts are `123456789012345678000` and `1.234567890123456789012345678901234e+6143`

### Requirement: Numbers: Conversions between digits, integers and decimal types
The library SHALL offer `to_int64`, `to_uint64` (an integer from `INT64_MIN` to `INT64_MAX`, or from 0 to `UINT64_MAX`, else nothing; a fraction, or more than 20 digits, is nothing), `from_int64`, `from_uint64` (digits without trailing zeros: `-1000` is one digit with the point after four), `to_decimal<D>` and `from_decimal<D>` for the concept `decimal_type` (`decimal32_t`, `decimal64_t`, `decimal128_t` and their `fast` variants), and `to_decimal128` and `from_decimal128`. `to_decimal` SHALL be exact: `errc::range` when the type cannot hold the number (more digits than its precision, an exponent outside its range, a value rounded into a subnormal, flushed to zero or carried to infinity), and `from_decimal` gives the same digits for every member of a cohort.

#### Scenario: Integer edges
- **WHEN** `to_int64` reads `9223372036854775807`, `9223372036854775808`, `-9223372036854775808`, `-9223372036854775809`, `1.0`, `1.5` and `1e20`
- **THEN** the results are `INT64_MAX`, nothing, `INT64_MIN`, nothing, 1, nothing and nothing

#### Scenario: Unsigned edges
- **WHEN** `to_uint64` reads `18446744073709551615`, `18446744073709551616`, `-1`, `-0` and `1.8e1`
- **THEN** the results are `UINT64_MAX`, nothing, nothing, 0 and 18

### Requirement: Numbers: Stored by value, in one of two forms
A number SHALL be stored in one of two forms chosen by its value, never by its spelling: an integer that fits the traits' `integer` (`std::int64_t`) is an `integer`; any other number is the traits' `number`, normalised, without trailing zeros. So `1`, `1.0` and `1e0` are one stored value (as they are one number in JavaScript), `1.50` and `1.5` store the same bytes, `18446744073709551615` is an exact `number`, and `-0` is `0`. With `js_traits` a number is first rounded to a double and then stored as an `integer` when that double is integral and fits `std::int64_t`, so it writes back as Node writes the double (`4611686018427387904` is written `4611686018427388000`, `123456789012345678` is `123456789012345680`).

#### Scenario: The same number whichever way it came
- **WHEN** `1`, `1.0`, `1e0` and `1.5e1` are stored under `default_traits`
- **THEN** each `is_integer()` is true and reads as the `std::int64_t` 1, 1, 1 and 15; `1.50` is a number, not an integer, whose `get<std::int64_t>()` is `errc::range` and `get<decimal128_t>()` is `1.5`

#### Scenario: Under js_traits
- **WHEN** `4611686018427387904` is parsed under `js_traits` and written
- **THEN** the text is `4611686018427388000`

### Requirement: Numbers: What each traits refuses
With the default traits a number SHALL be refused with `errc::range` when it has more than 34 significant digits (`12345678901234567890123456789012345`, 35 digits) or an exponent past `decimal128_t`'s range (`1e999999999999`, and `1e6145`), and every number inside it SHALL be stored exactly, `1e999` included (written `1e+999`, which a JavaScript peer reads as `Infinity`); `set` of a NaN or an infinity of a decimal type is `errc::range` and stores nothing. With `js_traits` no number is refused: it rounds as `JSON.parse` does, a number past a double's range is an infinity that `stringify` writes `null` (as `JSON.stringify` does), `12345678901234567890123456789012345` is written `1.234567890123457e+34`, and a double's NaN and infinities, which arithmetic makes, are stored and written `null`.

#### Scenario: Forty digits
- **WHEN** a 40-digit number is parsed under `default_traits` and under `js_traits`
- **THEN** the first is `errc::range` with the message "a number or index out of range" and the second is written `1.2345678901234568e+39`

#### Scenario: 1e999
- **WHEN** `1e999` is parsed and written under each traits
- **THEN** `default_traits` write `1e+999` and `js_traits` write `null`

### Requirement: Numbers: Reading a number as a type
`get<T>()` SHALL read a number as any integer type (character types and `bool` aside), a Boost.Decimal type, or the traits' `number`, when `T` holds it exactly, and give `errc::range` otherwise: a number that is not an integer, or does not fit, is never truncated or rounded into `T`; `errc::type` for another kind. A double is read or set only when the traits' `number` is one, and there is no conversion to or from `float` ever. `is_integer()` SHALL be `Number.isInteger`: true for an `integer`, and for a `number` with no fractional part such as `1e30`.

#### Scenario: Reading widths
- **WHEN** `300` is read as `std::int16_t`, `std::int8_t`, `std::int64_t` and `18446744073709551615` as `std::uint64_t` and `std::int64_t`
- **THEN** the results are 300, `errc::range`, 300, `18446744073709551615` and `errc::range`

#### Scenario: A number that is an integer but does not fit integer
- **WHEN** the decimal `1e30` is set and read
- **THEN** `is_integer()` is true and `get<std::int64_t>()` is `errc::range`

### Requirement: Numbers: Doubles through Boost.Charconv
The codec of `double` SHALL read and write through Boost.Charconv's `from_chars` and `to_chars` (not the standard library's, whose floating-point overloads Xcode 16's libc++ deletes), the same code path on every target; writing uses the shortest representation that reads back (`to_chars` in scientific format without a precision) and reads a value out of range as `JSON.parse` does, an infinity for a large one and zero (signed) for a small one, keeping a subnormal that some standard libraries report out of range.

#### Scenario: The shortest digits
- **WHEN** `number_codec<double>::write(0.1)` and the parse of `123456789012345678901234567890` are written
- **THEN** the texts are `0.1` and `1.2345678901234568e+29`

### Requirement: Strings: Encodings, validation and length
The traits' `string` SHALL be the stored encoding of strings and keys: `utf8` (the default) or `utf16` (the code units JavaScript holds). Either way `length()` counts UTF-16 code units as JavaScript does (UTF-8 is decoded to count them: `aé€😀` has length 5); text is validated where it enters: the parser refuses invalid UTF-8, and `set` refuses text invalid in its encoding with `errc::type` and changes nothing (invalid UTF-8, a surrogate written in UTF-8, a lone surrogate in UTF-16 under `utf8`). `length()` of any other kind than an array or a string is `errc::type`.

#### Scenario: Lengths
- **WHEN** `a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80` is set under each traits
- **THEN** `length()` is 5 under both, and `set` of `"\xff"` under `default_traits` is `errc::type` and leaves the length at 5

### Requirement: Strings: Reading a string as a type
A view SHALL read a string as it is stored and cannot transcode: `get<std::string_view>()` reads UTF-8 strings only and `get<std::u16string_view>()` UTF-16 ones (the other is `errc::type`), while `get<std::string>()` and `get<std::u16string>()` transcode from either; keys come out as the encoding's view (`std::string_view` under `utf8`, `std::u16string_view` under `utf16`). The sugar's `as_string()` reads UTF-8 strings only.

#### Scenario: Both encodings
- **WHEN** the string `é😀` is stored under `default_traits` and under `js_traits`
- **THEN** under `default_traits` `get<std::string_view>()` reads it and `get<std::u16string_view>()` is `errc::type`; under `js_traits` `get<std::u16string_view>()` reads 3 code units, `get<std::string>()` reads `é😀`, and `get<std::string_view>()` is `errc::type`

### Requirement: Strings: Escaping as JSON.stringify escapes
`stringify` SHALL write a string and a key escaped as `JSON.stringify` escapes them: `"` and `\` with a backslash; `\b`, `\f`, `\n`, `\r` and `\t`; any other character below U+0020 as `\u` and four lowercase hexadecimal digits; nothing else, so `/`, U+007F, text beyond ASCII and U+2028 and U+2029 are written as they are. Both encodings write the same text.

#### Scenario: Every control character
- **WHEN** a string of U+0000 to U+001F and U+007F is stringified
- **THEN** the text is the string between quotes in which U+0000 to U+001F are `\u0000` to `\u001f` (lowercase hexadecimal) but U+0008, U+0009, U+000A, U+000C and U+000D, which are `\b`, `\t`, `\n`, `\f` and `\r`, and U+007F is the byte itself, the same under each traits

### Requirement: Strings: Lone surrogates
A lone surrogate (a UTF-16 code unit of a pair whose other half is missing) SHALL be refused by `set` under `utf8` (`errc::type`), kept by `set` of a `std::u16string_view` under `utf16`, and written by `stringify` as `\u` and its four lowercase hexadecimal digits as `JSON.stringify` has since ES2019; the parser refuses one with either encoding as `errc::syntax`, because Boost.JSON's parser cannot deliver one (it refuses it, or with `allow_invalid_utf16` replaces it with U+FFFD, which would change the text silently), so the limit is documented and not worked around.

#### Scenario: A surrogate under utf16
- **WHEN** `u"\xD800"` is set under `js_traits` and stringified
- **THEN** `set` succeeds and the text is `"\ud800"`; a valid pair `D83D DE00` is written as the character, and `"\udc00\ud800"` as `"\udc00\ud800"`

#### Scenario: A surrogate in text
- **WHEN** `"\ud800"` (as JSON text) is parsed under either traits
- **THEN** the result is `errc::syntax`

### Requirement: Dates: A date is a Date
A date SHALL be a `date_ms` (milliseconds since 1970-01-01T00:00:00Z) with JavaScript's range of plus or minus 8.64e15 ms: `set` outside it is `errc::range`, and writing follows `Date.prototype.toJSON`: `"2026-10-12T03:04:05.678Z"`, with a year outside 0 to 9999 as six digits and a sign (`"+275760-09-13T00:00:00.000Z"`, `"-000001-01-01T00:00:00.000Z"`), floor division for negative times (`-1` ms is `1969-12-31T23:59:59.999Z`), and the Gregorian calendar of any year a 64-bit count reaches.

#### Scenario: toJSON's text
- **WHEN** `traits_test` writes the dates 0, -1, 1791774245678, 8640000000000000, -8640000000000000, -62198755200000, 253402300800000, 253402300799999 and 951782400000
- **THEN** the texts are `1970-01-01T00:00:00.000Z`, `1969-12-31T23:59:59.999Z`, `2026-10-12T03:04:05.678Z`, `+275760-09-13T00:00:00.000Z`, `-271821-04-20T00:00:00.000Z`, `-000001-01-01T00:00:00.000Z`, `+010000-01-01T00:00:00.000Z`, `9999-12-31T23:59:59.999Z` and `2000-02-29T00:00:00.000Z`

#### Scenario: One millisecond past the range
- **WHEN** a date of `8.64e15 + 1` ms is set
- **THEN** the result is `errc::range`

### Requirement: Dates: Reading a date from text
A string in exactly the format of `toJSON` SHALL read as a date: `get<date_ms>()` of such a string converts it, revived or not, and a parse with `parse_options::revive_dates` turns it into a date node; any other string stays a string and `get<date_ms>()` of it is `errc::type`. The text is refused for a missing field or other separator (`2026-10-12`, `2026-10-12T03:04:05Z`, `2026-10-12T03:04:05.678+00:00`), a field out of range (month 13, `2026-02-30`), `+275760-09-13T00:00:00.001Z`, a year of five digits (`10000-01-01T00:00:00.000Z`), and a six-digit year that the other form would write (`-000000`, or a `+` year up to 9999). JSON has no dates, so a parse keeps every string a string unless the option asks.

#### Scenario: Revived or not
- **WHEN** `["2026-10-12T03:04:05.678Z","2026-10-12"]` is parsed with `revive_dates`, and without
- **THEN** with it element 0 `is_date()` and element 1 `is_string()`, and the stringified text is unchanged; without it both are strings

### Requirement: Bytes: Stored raw, written by the traits' policy
Bytes (a `std::span<const std::byte>` for `set`; `std::vector<std::byte>` or a span for `get`) SHALL be stored raw in the image and sent as binary as they are, with `kind::bytes`; `stringify` writes them by the traits' policy: `base64` (RFC 4648 with padding: `Af8=` for `01 FF`), `base64url` (RFC 4648 section 5, no padding: `Af8`) or `javascript_bytes` (`{"0":1,"1":255}`, an empty value `{}`), and an empty value is `""` under the base64 policies. A string valid in the policy's text reads as bytes through `get<std::vector<std::byte>>()` (not as a view), and a parse with `parse_options::revive_bytes` turns such a string into bytes; under `javascript_bytes` an object whose keys are exactly `"0"` to `"n-1"` holding integers from 0 to 255 does, and an object with a gap, a value of 256 or a string does not.

#### Scenario: The three policies
- **WHEN** the bytes `DE AD BE EF` are stringified under `default_traits`, a `base64url` traits and `js_traits`
- **THEN** the texts are `"3q2+7w=="`, `"3q2-7w"` and `{"0":222,"1":173,"2":190,"3":239}`

#### Scenario: Revived bytes
- **WHEN** `["Zm9v","Zm9v!"]` is parsed with `revive_bytes`
- **THEN** element 0 `is_bytes()` and reads `f`, `o`, `o`, and element 1 is a string

### Requirement: Bytes: Base64 decoding
Reading base64 text SHALL require a length that is a multiple of four with at most two `=` of padding (for `base64url`, no padding and a length of 4n + 1 is refused), characters of the alphabet only, and zero in the bits of the last character that make no whole byte; any other text is `errc::syntax` from the decoder, so it is no bytes. The encoder SHALL write RFC 4648 section 10's vectors (`f` is `Zg==`, `fo` `Zm8=`, `foo` `Zm9v`, `foob` `Zm9vYg==`, `fooba` `Zm9vYmE=`, `foobar` `Zm9vYmFy`) and `\xfb\xff` as `+/8=` or, in the URL alphabet, `-_8`.

#### Scenario: Text that is no base64
- **WHEN** `Zg=`, `Z`, `Zm9v!`, `Zg==Zg==` and `Zm=v` are decoded as base64
- **THEN** each is refused

### Requirement: Order: Array-index keys
An array index SHALL be a key that is the canonical decimal spelling of an integer from 0 to 4,294,967,294: `"0"`, `"7"` and `"10"` are, and `"01"`, `"-0"`, `"-1"`, `"1.0"`, `" 1"`, `"1e2"`, `""`, `"4294967295"` and a number of more than 10 digits are not.

#### Scenario: Indices and look-alikes
- **WHEN** `traits_test` asks `detail::array_index` of `0`, `7`, `10`, `4294967294`, then of `4294967295`, `01`, `-0`, `-1`, `1.0`, ` 1`, `1e2`, `` (empty), `a`, `00` and `99999999999999999999`
- **THEN** the first four are the indices 0, 7, 10 and 4294967294 and the rest are not indices

### Requirement: Order: JavaScript's key order
With `javascript_order` (the default) an object's keys SHALL come out as ECMA-262's OrdinaryOwnPropertyKeys gives them: array indices ascending (group 0, ordinal the index), then the other keys in the order they were created (group 1, ordinal the object's next insertion number). Assigning an existing key keeps its place; erasing a key and creating it again puts it where a new key goes (last among the others, or back among the indices, ascending). The order is kept by `set`, `erase`, `assign_from`, a copy, a literal and the parser.

#### Scenario: Measured in Node
- **WHEN** `{"b":1,"2":2,"a":3,"1":4}` is parsed and written
- **THEN** the text is `{"1":4,"2":2,"b":1,"a":3}`

#### Scenario: Every look-alike
- **WHEN** the keys `x`, `01`, `4294967295`, `-0`, `1e2`, the empty key, `4294967294`, `0` and a key holding a zero byte are set in that order
- **THEN** the keys come out `0`, `4294967294`, `x`, `01`, `4294967295`, `-0`, `1e2`, the empty key, the key with the zero byte, each found exactly by `contains` and none of `1`, `00`, `+0`, `100`, `k`, `1E2` found

#### Scenario: Erased and created again
- **WHEN** `x`, `0`, `4294967295`, the zero-byte key, `4294967294` and the empty key are each erased and set again
- **THEN** a non-index key goes last among the others and an index key goes back among the indices; `set(wd::undefined)` of a key erases it and a later `set` creates it last

### Requirement: Order: insertion_order and sorted
`insertion_order` SHALL put every key in the order it was created, whatever the key (the text's order for a parsed object), and `sorted` SHALL order keys by their UTF-16 code units, with either encoding, as JavaScript's `sort()` and RFC 8785 order them, keeping no ordinal (the node's ordinal is its id); with `sorted` a key erased and created again takes its sorted place.

#### Scenario: One text under each policy
- **WHEN** `{"b":1,"2":2,"a":3,"1":4,"01":5}` is parsed under `javascript_order`, `insertion_order` and `sorted`
- **THEN** the texts are `{"1":4,"2":2,"b":1,"a":3,"01":5}`, `{"b":1,"2":2,"a":3,"1":4,"01":5}` and `{"01":5,"1":4,"2":2,"a":3,"b":1}`

#### Scenario: Sorted by code units
- **WHEN** nine keys including the empty key and a zero-byte key are set under `sorted`
- **THEN** they come out the empty key, the zero-byte key, `-0`, `0`, `01`, `1e2`, `4294967294`, `4294967295`, `x`

### Requirement: Order: A repeated key
A key written twice in parsed text, a literal or an `assign_from` SHALL keep the last value in the first one's place, as `JSON.parse` does.

#### Scenario: The last value in the first place
- **WHEN** `{"a":1,"b":2,"a":3}` and `{"a":[1],"a":{"x":1}}` are parsed
- **THEN** the texts are `{"a":3,"b":2}` and `{"a":{"x":1}}`

### Requirement: Order: Ordered indices, never hashed
An object's members SHALL be kept in ordered indices and never in a hashed one, since keys come from untrusted text: no input makes a lookup slower than its logarithm. A crafted input that took Boost.JSON 9.23 s for 4 MB therefore costs one tree insertion per key.

#### Scenario: 100,000 keys
- **WHEN** an object of 100,000 distinct keys `k0` to `k99999` is parsed
- **THEN** `size()` is 100,000 and every key is found with its value

### Requirement: Heap: Boost.Interprocess's rbtree_best_fit, used directly
The image's heap SHALL be `boost::interprocess::rbtree_best_fit<no_mutexes, offset_ptr<void, std::int64_t, std::uint64_t, 8>, 16>` used directly, where `detail::no_mutexes` is document's own family of `null_mutex`; every pointer inside an image is `detail::pointer<T>`, an `offset_ptr<T, std::int64_t, std::uint64_t, 8>`, and every size a `std::uint64_t`, so the image works at any 16-aligned address and has the same bytes on 64-bit native and wasm32. The first block the heap hands out, where the tree's container lives, is at `detail::first_block_offset` from the image's start, the same in every image.

#### Scenario: The same bytes everywhere
- **WHEN** the same operations build a document on clang, gcc, wasip2 and Emscripten
- **THEN** the images are byte for byte equal, and `example/document_image.expected` prints the same SHA-256 digest prefix (`ed85084394e7bf7f...`) on every target

### Requirement: Heap: The allocator zeroes
`detail::allocator<T>` SHALL allocate through the heap's `allocate_aligned`, zero each block before it hands it out and before it frees it, and report an allocation that fails through `boost::throw_exception(std::bad_alloc())` (such a failure is a bug of the library, never a run-time event, since every operation finds its room first); its `pointer` is an offset pointer, its `size_type` a `std::uint64_t`, and two allocators compare equal when they allocate in the same heap.

#### Scenario: A block is handed out zeroed
- **WHEN** `heap_test` allocates and frees blocks through the allocator
- **THEN** each block is all zero when handed out, and zero again after it is freed

### Requirement: Heap: Strings of exact size
`detail::byte_string` SHALL be an offset pointer and a `std::uint64_t` size, allocated at exactly the bytes' size through the heap's non-throwing `allocate` and never a `boost::container::string` (whose allocation may grow beyond the size and throws on failure); it reads as bytes, as UTF-8 `chars()` and as UTF-16 `units()`. `make_string` SHALL return `errc::capacity` when there is no room, an empty string allocates nothing, and `free_string` zeroes the bytes, frees them and leaves the string empty.

#### Scenario: A secret leaves no byte
- **WHEN** a 64-byte secret is stored as a value, in an array, as a key and in a copy, and those values are erased or replaced
- **THEN** no eight consecutive bytes of the secret are found anywhere in the image, and the rest of the document reads as before

### Requirement: Heap: Regions, owned and external
`detail::region` SHALL be the bytes of an image: owned, which allocates, zeroes and grows, or external, a `std::span<std::byte>` its user owns, which never grows. An owned region needs at least the bytes that hold a header and an empty heap (a document made by `create`, or parsed, starts at 4,096 bytes, or `max_bytes` rounded down to 16 if smaller), grows to twice its size or to what the operation needs, if more, rounded up to 16, at most its `max_bytes` (1 GiB, `default_max_bytes`, unless told otherwise; `errc::capacity` past it), copying the image, zeroing the rest and growing the heap; `shrink_to` moves the first bytes into a smaller buffer. A buffer that is not 16-aligned is `errc::capacity` to `external_empty`.

#### Scenario: An owned region stops at its limit
- **WHEN** an owned document with `max_bytes` 64 KiB is filled until a change does not fit
- **THEN** the change returns `errc::capacity`, the document's content is as it was, and the region never exceeds 64 KiB

### Requirement: Tree: Nodes, indices and ids
A document's tree SHALL be one `boost::multi_index::multi_index_container` of `detail::node<Traits>` in the image's heap with three ordered indices and none hashed: `by_id` (unique), `by_order` on `(parent, group, ordinal)` (unique; elements by index, members in the policy's order, iteration) and `by_key` on `(parent, key)` (non-unique; the document keeps keys unique; a member by key, ordered by the keys' UTF-16 code units whatever the encoding). A node holds `id` (unique, never reused; the root's is `root_id` = 1), `parent` (0 for the root), `group` (0 for an element and for an array-index key under `javascript_order`, else 1), `ordinal`, `key`, `kind`, `repr` (for a number, the traits' `integer` or `number`), a 16-byte `scalar` (a boolean, an integer, a number or a date; for an array its length; for an object its next insertion number and its key count) and `text` (a string's code units or a bytes value's bytes). A node takes 192 bytes of the image whatever it holds, and a string's or a key's bytes a block of their own besides.

#### Scenario: Memory per value
- **WHEN** `"abcdefgh"` and `{"k":0}` are stored
- **THEN** they take 240 and 432 bytes of image, the same on every target

### Requirement: Tree: Every operation completes or changes nothing
Every operation that changes the tree SHALL either complete or change nothing and return `errc::capacity`, without throwing and a full heap included: it computes what it will allocate (each node's block, bounded by `node_probe_bytes = sizeof(node) + 128`, since Boost.MultiIndex does not name its internal node type; each string's exact size), probes the heap for each block with the non-throwing `allocate` and frees the probes at once; if a probe fails an owned document grows and probes again and an external one returns `errc::capacity`; it then allocates for real (strings first, then `emplace`) and rearranges with steps that do not allocate (swapping prepared strings in `modify`, renumbering ordinals), since Boost.MultiIndex erases an element whose `modify` throws. A source is read before anything changes, so a value may be copied into its own subtree.

#### Scenario: A full external buffer in every size
- **WHEN** `value_test` copies a value into external buffers of every size from the smallest that formats to the first that holds the copy
- **THEN** each copy either completes or fails with `errc::capacity` leaving the document's value and image header as they were

### Requirement: Tree: Free memory is zero and the image is a function of the operations
A buffer SHALL be zeroed when made or grown, the allocator zeroes a block before it frees it, nodes are built in place (`emplace`), never copied from a temporary, and the index's key extractors and comparators carry a zeroed byte (an empty class's byte holds no value, and MSVC copies a temporary's without writing it first), so that an erased value leaves none of its bytes in the image and the image is a function of the operations alone: the same operations give the same image, byte for byte, at any address and on every target. After an operation that failed for want of room the document's content is as it was but bytes of the heap's free space may differ (the heap keeps its bookkeeping there), so documents are compared with `==`, not by raw image, after a failure.

#### Scenario: The same operations at two addresses
- **WHEN** `image_test` runs the same calls, with growth, erasure and nesting, on a document of each storage at different addresses
- **THEN** the images are equal byte for byte

### Requirement: Images: The 64-byte header
An image SHALL be a 64-byte header, in the byte order of the machine that wrote it, followed by the heap at offset 64: at offset 0 the magic `WCPPDOC` and a zero byte (`image_magic`, 8 bytes); 8 the format version (4 bytes, `format_version`); 12 the byte-order mark `0x01020304` as the writer's machine writes it (`byte_order_mark`, 4); 16 four reserved bytes, zero; 20 the layout tag (4); 24 the image's size (8); 32 the offset of the tree's container (8, the heap's first block, the same in every image); 40 the id the next node takes (8; ids are never reused); 48 the seal (8; zero for an image that is not sealed); 56 eight bytes of zero. `image_header` is that struct, `static_assert`ed to 64 bytes with the seal at offset 48; the four reserved bytes sit where a Boost version once sat, and the other fields do not move.

#### Scenario: The offsets
- **WHEN** `heap_test` compiles
- **THEN** `offsetof(image_header, reserved_4)` is 16, `offsetof(image_header, layout)` is 20 and `offsetof(image_header, seal)` is 48

### Requirement: Images: check_image and what opening refuses
`check_image(bytes, layout, check)` SHALL read nothing past the header before the size is known and refuse, in this order: a sealed image (unless `verify::none`) whose bytes are fewer than its size or do not match its seal, as `errc::damaged_document`, before anything else its header states is trusted; bytes shorter than the header (64 bytes), another magic, a size below 64 or beyond the bytes, as `errc::not_a_document`; another format version or byte order, as `errc::incompatible_document` (before the size, which an image of another version states in another way); nonzero reserved bytes or another layout, as `errc::incompatible_document`; and a seal that is neither zero nor marked, as `errc::damaged_document`. `open` and `from_image` then run the heap's own sanity check, and a container offset other than the heap's first block, or a pointer to the root that leaves the image, is `errc::not_a_document`, refused before anything is read through it.

#### Scenario: Each refusal
- **WHEN** `example/document_image_errors.cpp` opens an image of other traits, an image of format version 2, text longer than a header with no magic, zeroed memory, a misaligned buffer, an image cut short, a header that states a size larger than the bytes, and a header that names a container outside the heap
- **THEN** the codes are `incompatible_document`, `incompatible_document`, `not_a_document`, `not_a_document`, `not_a_document`, `not_a_document`, `not_a_document` and `not_a_document`

#### Scenario: The checks of the header
- **WHEN** `heap_test` edits a valid header: the magic's first byte, a size of image size plus 1, a size of 63, version 2, byte order `0x04030201`, `reserved_4` of 1 and of `0x80000000`, version `format_version + 1`
- **THEN** the first three are `not_a_document` and the rest `incompatible_document`; the first 63 bytes alone are `not_a_document`, and a valid header with another expected layout is `incompatible_document`

### Requirement: Images: open, format, from_image, image and clone
`format<Traits>(buffer)` SHALL make a new empty document (holding `null`) in a 16-aligned buffer (`errc::capacity` when it is too small or not aligned); `open<Traits>(buffer, check = verify::seal)` SHALL open the image a buffer holds, in place, as an `external` document (`errc::not_a_document` for a buffer that is not 16-aligned); `from_image<Traits>(bytes, max_bytes, check)` SHALL copy an image, from any address, into a new owned document (`errc::capacity` when it is larger than `max_bytes`); `doc.image()` SHALL be a `std::span<const std::byte>` of the image valid until the document's next change; and `doc.clone()` SHALL copy a document into a new owned document, keeping a seal (not checked, the image being its own), as `structuredClone` would. A buffer larger than the image its header states is opened over the first `size` bytes.

#### Scenario: Copied to another address
- **WHEN** a document's image is copied into a buffer at another 16-aligned address and opened there, in place and with `from_image`
- **THEN** both are equal to the original and `from_image`'s grows independently up to its `max_bytes`

### Requirement: Images: An image is trusted input
The checks of opening SHALL be documented as unable to prove sound every offset inside the tree, so an image travels only between a program's own processes and peers it trusts; untrusted data enters as JSON text, which the parser checks entirely. A seal is a checksum against accidental damage and is not authentication: the page repeats "never open an image from a source you do not trust" wherever images are discussed.

#### Scenario: The page says it
- **WHEN** `doc/images.adoc`, `doc/wire.adoc`, `doc/gains.adoc` and the README are read
- **THEN** each says that a seal is not authentication and that an image from an untrusted source must not be opened

### Requirement: Images: The format version is the library's own, and a test watches Boost
The header SHALL NOT carry the Boost version; the format version is the library's constant, raised only when the layout changes and then with a migration from the previous version. `test/format_test.cpp` SHALL compare the compact image of a fixed document under `default_traits` and under `js_traits` with the golden images `test/fixtures/format/default_traits.image` and `js_traits.image` on every toolset, require each golden to open and write the expected text, and fail when a Boost release changes one byte of the layout Boost.Interprocess's `rbtree_best_fit` and Boost.MultiIndex's ordered indices give; `format_test --write <directory>` writes the goldens again, which is right only together with raising `format_version`.

#### Scenario: A Boost release changes the layout
- **WHEN** a Boost release changes a byte of the compact image of the fixed document
- **THEN** `format_test` fails on every toolset, and the golden images are regenerated only with a raised `format_version`

### Requirement: Images: Owned and external storage
`basic_document<Traits, Storage>`'s `Storage` SHALL be `owned` (the default: the document's own 16-aligned buffer, which grows twice the size, or to what the operation needs if more, up to the `max_bytes` of `create` or `from_image`, 1 GiB unless told otherwise, and past it returns `errc::capacity`) or `external` (a `std::span<std::byte>` its user owns, 16-aligned, such as a browser's memory, which never grows: a full one returns `errc::capacity` and changes nothing). `owned` documents move cheaply (a moved-from document holds nothing and its handles are invalid) and are copied only by `clone()`, or with exceptions by the copy constructor.

#### Scenario: An external document says full
- **WHEN** `document_image.cpp` pushes into a document formatted in a small buffer until it is full
- **THEN** the push after the last that fits is `errc::capacity` ("no room left"), the document is unchanged, and the elements pushed so far are all there

### Requirement: Images: reserve, capacity and shrink_to_fit
`doc.capacity()` SHALL be the image's size (0 for a moved-from document); `doc.reserve(bytes)` SHALL, like `std::vector::reserve`, make an owned image hold at least `bytes` in one step (to `bytes` or twice its size, the larger, at most its `max_bytes`), invalidating every `std::string_view` read from a value but no handle or iterator and clearing a seal, changing nothing within the capacity, and returning `errc::capacity` (changing nothing) for a request past `max_bytes` or past an external image's size and `errc::type` for a moved-from document; `doc.shrink_to_fit()` (owned only) SHALL give back the free space at the end of the image to the end of its last block in use and only that: blocks never move, so a document that grew and let most of its values go may stay nearly as large, and a deep copy into a new document, or `compact()`, packs the content.

#### Scenario: Reserving once
- **WHEN** ten thousand members are inserted into two objects, one without and one with a reservation of 256 bytes a member
- **THEN** the unreserved one grew the image during the insertions, the reserved one did not (`reserve grew the image: true`, `after the reserve, 10000 insertions grew it: false`)

### Requirement: Seal: sealing an image
`doc.seal()` SHALL write into the header, at offset 48, the CRC-32C (Castagnoli: Boost.CRC's `crc_optimal<32, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true>`) of the whole image with the seal's own eight bytes taken as zero, in the low 32 bits, with the high bit set as the marker (`sealed_marker = 1 << 63`); `doc.sealed()` SHALL say whether the image carries a seal (false for a moved-from document, not checking it); any later change to the document, `reserve` and `compact` included, clears it, so an image is never wrongly sealed; sealing twice gives the same seal; a moved-from document's `seal()` is `errc::type`. The check of the CRC is `0xE3069283` for the standard string `123456789`. A seal costs a pass over the image (about 600 MB/s, 1.7 ms per MiB measured), and so does the check at opening.

#### Scenario: Sealed, then changed
- **WHEN** a sealed document is opened, a member set, and the document sealed again
- **THEN** after the set `sealed()` is false and the image opens unsealed; after the second `seal()` it opens sealed and reads the new member

#### Scenario: A change that fails keeps the seal
- **WHEN** a sealed external document is asked to `set` a string of 8,192 bytes that does not fit
- **THEN** the result is `errc::capacity`, the document is still sealed and its image bytes are unchanged

### Requirement: Seal: Verification at opening
`open`, `from_image`, `basic_image_reader` and `shared_document::from_image`/`adopt` SHALL check a sealed image's bytes against its seal first of all and refuse a mismatch with `errc::damaged_document`; an image that is not sealed opens as before; `verify::none` (against the default `verify::seal`) skips the pass for a program that trusts its memory, so a damaged string opens and reads as damaged; `async_write_image` seals before it writes. Every byte of a sealed image changed, one at a time, and every single bit of its header, SHALL be refused as damaged by `open` and `from_image`, and a sealed image cut short is damaged (a torn write) while an unsealed one cut short is `not_a_document`.

#### Scenario: Every byte
- **WHEN** `seal_test` changes each byte of a sealed image in turn, in a buffer of exactly its size
- **THEN** `open` and `from_image` each refuse all of them, the count equal to the image's size

#### Scenario: verify::none
- **WHEN** one byte of a sealed image's string is changed and the image opened with `wd::verify::none`
- **THEN** it opens and the string reads as changed (`{"user":"ada",...}`), where the default check says `damaged_document`

### Requirement: Compact: compact, compact_size and compact_into
`doc.compact()` (owned only) SHALL rebuild the image as the compact image of its content, built before it replaces the old one, so on failure (`errc::capacity` when the new buffer cannot be allocated, `errc::type` for a moved-from document) nothing changes; like a vector's reallocation it invalidates every handle, every `std::string_view` and every iterator, clears a seal, keeps the document's `max_bytes`, and leaves the image's size at `compact_size()`, which is also its `capacity()`. `doc.compact_size()` SHALL be the exact size of the compact image (0 for a moved-from document), read from the nodes without allocating or writing (each block's size comes from `rbtree_best_fit`'s own arithmetic and a scratch heap on the stack, so the figure follows the Boost release of the build). `doc.compact_into(buffer)` SHALL write the compact copy of any document, owned or external, into a 16-aligned buffer of at least `compact_size()` bytes that does not overlap the image, return an external document over its first `compact_size()` bytes, zero the rest of the buffer and leave the source untouched; `errc::capacity` for a buffer that is smaller or not 16-aligned (writing nothing), `errc::type` for a moved-from document or an overlapping buffer.

#### Scenario: Smaller than shrink_to_fit
- **WHEN** a small document of 60 members has 54 erased, and a large one has 90% of its members erased
- **THEN** the small one is 15,312 bytes after `shrink_to_fit` and 2,352 after `compact`, and the large one 2,080,304 and 209,424

#### Scenario: A buffer exactly enough
- **WHEN** `compact_into` is given a buffer of exactly `compact_size()` bytes, one byte short, and a larger one
- **THEN** the first holds the copy, full, with the bytes owned documents compact to; the second is `errc::capacity` and writes nothing; in the third the image is first and the rest is zero

### Requirement: Compact: Equal content gives equal bytes
The compact image SHALL be a function of the content alone: the tree is walked once in document order (the traits' own order, for `sorted` objects by key, otherwise elements by index, array-index keys by index, then the other members as inserted), depth first, parents before children, with ids renumbered from 1 in the order of the walk (`next_id` is the node count plus one), insertion numbers renumbered from 0 (an object's next insertion number is the count of its non-index members), and for each node its key, its text and then the node allocated in a fresh heap with no free block until the end, the image sized to exactly the blocks it holds. Two documents of equal content (the same keys in the same JavaScript order, the same values of the same representation) SHALL compact to byte-identical images whatever edits made them (parsed, built by sets, erases and splices, values replaced by longer and shorter strings, an insertion that failed for want of room) on the same target and traits; a compact image compacts to itself and `shrink_to_fit` of it changes nothing; `stringify`, `==` and key order are kept; and the image opens, seals and verifies.

#### Scenario: Different histories, the same bytes
- **WHEN** `compact_test` makes one content by parsing, by sets and erases, by replacing values with longer and shorter strings, by splicing, and in a buffer too small for one edit
- **THEN** the compact images are byte-identical, the images before compacting differ, and compacting again changes nothing

### Requirement: Document: Making, moving and copying
`basic_document<Traits, Storage>` SHALL be made by `create(max_bytes = default_max_bytes)` (owned only; a new document holding `null`; `errc::capacity` when even an empty one does not fit `max_bytes`), by `format`, `open` or `from_image`, or by `parse`; `document` SHALL be `basic_document<>` (default traits, owned). A document is moved cheaply (a moved-from document holds nothing, reads as `undefined`, and every handle of either is invalid), is not copy-assignable, and is copied by `clone()` (a new owned document, `errc::type` for a moved-from one, `errc::capacity` when it does not fit); with exceptions the default constructor makes a document holding `null` and the copy constructor clones it. `root()`, `operator[](std::string_view)` and `operator[](std::uint64_t)` give handles, `==` compares the roots structurally, and `stringify(indent)` writes the root.

#### Scenario: A new document
- **WHEN** `wd::document::create()` is made
- **THEN** `root().kind()` is `kind::null`, `root().size()` is 0 and `doc["x"]` and `doc[0]` are `undefined`

#### Scenario: A moved-from document
- **WHEN** a document is moved from
- **THEN** `stringify(from)` is empty, `clone()` is `errc::type`, `seal()` is `errc::type`, `image()` is empty and `capacity()` is 0, while the new document holds the old value

### Requirement: Handles: Three states, and navigation never fails
`value` (reads and writes) and `const_value` (reads), and their `basic_` templates, SHALL name one place in one document and be cheap to copy: a node (navigation to a value that exists), a missing member or element of a container that exists (`obj["absent"]`, `arr[9]`; it reads `undefined` and `set` creates it, filling the elements between with `null`), or nothing (navigation through `undefined`, a scalar or an erased node, or a moved-from document; it reads `undefined` and `set` fails with `errc::type`). `operator[]` by key and by index and `at` SHALL never fail. A handle holds the document's address and node ids, not a pointer into the tree, and resolves its place again at every call, so it reads what is there now, stays valid when the document grows and its image moves, reads `undefined` once its node is erased, and is invalid once its document is moved from or destroyed.

#### Scenario: Navigation through nothing
- **WHEN** `doc["a"]["b"][9]`, `doc["a"]["b"][1]["x"]`, `doc["zz"]["y"]["w"]` and `doc["a"][0]` are read on `{"a":{"b":[1,2]}}`
- **THEN** each `is_undefined()`, and `doc["zz"].get<std::int64_t>()` is `errc::type`

#### Scenario: A handle survives growth
- **WHEN** a handle `doc["a"]` is kept and enough is inserted to double the owned image several times
- **THEN** the handle reads and writes through the moved image; only a `std::string_view` taken before the change is stale

#### Scenario: Setting through nothing
- **WHEN** `doc["missing"]["x"].set(1)` and `doc["user"]["age"]["x"].set(1)` are called
- **THEN** both are `errc::type` and the document equals its clone from before

### Requirement: Handles: kind and the predicates
A handle SHALL give `kind()` (`kind::undefined` when nothing is there), `is_undefined()`, `is_null()`, `is_boolean()`, `is_number()`, `is_integer()`, `is_string()`, `is_date()`, `is_bytes()`, `is_array()` and `is_object()`; `is_integer()` is JavaScript's `Number.isInteger`.

#### Scenario: Predicates
- **WHEN** `{"n":3,"s":"x"}` is read
- **THEN** `doc["n"].is_integer()` is true, `doc["s"].is_string()` is true and `doc["n"].is_string()` is false

### Requirement: Handles: get, value_or, contains, size, length and at
`get<T>()` SHALL read the value as `T` and return a `result<T>`, for `T` in: `bool`, any integer type, a Boost.Decimal type, the traits' `number`, `std::string`, `std::string_view`, `std::u16string`, `std::u16string_view`, `date_ms`, `std::vector<std::byte>` and `std::span<const std::byte>` (a view is valid until the document's next change); `errc::type` when nothing is there or for another kind, `errc::range` for a number `T` cannot hold exactly. `value_or(fallback)` SHALL be JavaScript's `??`: the value `get<T>()` gives, `T` being the fallback's type, or the fallback when the value is `undefined` or `null` or cannot be read as `T`. `contains(key)` is `key in v` for an object (false for any other kind), `size()` an object's key count or an array's length (0 otherwise), `length()` an array's length or a string's in UTF-16 code units (`errc::type` for anything else), and `at(i)` is `Array.prototype.at` (a negative index counts from the end; a handle that names nothing when out of range).

#### Scenario: Nullish coalescing
- **WHEN** `{"age":30,"none":null,"name":"Ana"}` is read with `value_or(18)` of `absent`, `none`, `age`, `name`, and `value_or(std::string("x"))` of `name`
- **THEN** the results are 18, 18, 30, 18 and "Ana", and `doc["a"]["b"].value_or(std::int64_t{7})` is 7

#### Scenario: JavaScript's own table
- **WHEN** `example/document_javascript.cpp` reads `user.name`, `user.nick`, `doc.n.x.y`, `user.tags[0]`, `user.tags.at(-1)`, `'tags' in user`, `user.name.length` and `Number.isInteger(doc.n)`
- **THEN** it prints `"Ada"`, `undefined`, `undefined`, `"math"`, `"math"`, `true`, `3` and `true`, the lines Node prints for the same JavaScript

### Requirement: Handles: set stores a value
`set(x)` SHALL store `x` at the handle's place (a missing member is created, an element past the end fills the elements between with `null`, an existing value is replaced) and return `result<void>`, `[[nodiscard]]`. Settable: `bool`; any integer type (character types and `bool` aside); the Boost.Decimal types (finite, else `errc::range`); the traits' `number`; a floating-point type of 8 bytes only into traits whose `number` is binary; `std::string_view`, `const char*`, `std::string` and `std::u16string_view` (valid in the encoding, else `errc::type`); `std::span<const std::byte>` and `std::vector<std::byte>` (bytes); a `date_ms` (within JavaScript's range, else `errc::range`); `nullptr` or `wd::null`; the literals `wd::object` and `wd::array` and `wd::literal`; a handle or a document of the same traits and any storage (a deep copy); and `wd::undefined`. A character is not a number: `set('a')` does not compile, nor does `set` of a `float`, nor of a `double` unless the traits' number is one. A failed `set` changes nothing.

#### Scenario: Creating and replacing
- **WHEN** `doc["user"]["age"].set(30)` then `.set(31)` is called on `{"user":{}}`
- **THEN** the member is created with 30, then replaced with 31, and `doc["user"].size()` stays 1

#### Scenario: An element past the end
- **WHEN** `[1,2]` has `doc["arr"][4].set(5)` called
- **THEN** the array is `[1,2,null,null,5]`, its length 5, and elements 2 and 3 are `null`

### Requirement: Handles: erase, set(undefined) and assign_from
`erase(key)` SHALL be `delete` and return `result<bool>` (whether the key was there; `errc::type` for any other kind than an object, including a handle that names nothing); `set(wd::undefined)` of a member SHALL erase it (JavaScript keeps a member whose value is `undefined`, and writes nothing for it; a document never stores `undefined`), of an element store `null`, and of a root make the document hold `undefined` and write nothing; erasing what is not there succeeds. `assign_from(source)` SHALL be `Object.assign`: each member of the source is set in its order, as one operation that completes or changes nothing; an array's elements are its members `"0"` to `"n-1"`; `null` and `undefined` add nothing; a scalar source, or a target that is not an object, is `errc::type`; the keys are staged first, so a source inside the target is read before anything changes.

#### Scenario: Erasing keeps the order
- **WHEN** `a` is erased from `{"a":1,"b":2,"c":3,"d":4}`
- **THEN** the text is `{"b":2,"c":3,"d":4}` (Boost.JSON's `erase` gives `{"d":4,"b":2,"c":3}`)

#### Scenario: Object.assign
- **WHEN** `{"age":30,"b":0,"c":3}` has `age` erased and is assigned `{"a":1,"b":2}`, then the array `["x",[true]]`
- **THEN** the keys are `b`, `c`, `a`, then `0`, `1`, `b`, `c`, `a`; assigning `null` or a missing member adds nothing, and assigning `source["a"]` (a scalar) is `errc::type`

### Requirement: Handles: Copies, never aliases
`set` of a handle or a document SHALL store a deep copy of its value, never an alias (JavaScript's `a.x = b.y` makes one object of two; a document is a tree, as JSON is), reading the source before anything changes, so a value may be copied into its own subtree and `a.splice(0, 1, a)` inserts a copy of the array as it was; between documents the copies need the same traits and any storage; `clone()` copies a whole document. A change to the source afterwards does not reach the copy.

#### Scenario: A copy is its own
- **WHEN** `doc["copy"].set(other["user"])` is called and then `other["user"]["name"].set("Bia")` and a member is added to `other["user"]`
- **THEN** `doc["copy"]["name"]` is still "Ana" and the new member is not in the copy

### Requirement: Handles: Structural equality
`==` of two handles or two documents SHALL compare structurally, which JavaScript does not: the same kind; numbers by value (`1`, `1.0` and `1e0` are equal); strings, dates and bytes by value; arrays element by element; objects with the same keys, each holding an equal value, in any order. Two values that read `undefined` are equal; two documents compare their roots.

#### Scenario: Order does not matter
- **WHEN** `{"b":2,"c":3,"a":1}` and `{"a":1,"b":2,"c":3}` are compared
- **THEN** they are equal

### Requirement: Handles: Iteration
`entries()`, `keys()` and `values()` SHALL be `Object.entries`, `Object.keys` and `Object.values`, in the order of the traits' object policy (empty for any other kind than an object), a key being a `std::string_view` with UTF-8 strings and a `std::u16string_view` with UTF-16 ones, and a `for` over an array's handle SHALL visit its elements. The ranges read the document as it is when each step is taken, so changing the container while iterating it is undefined, as for a standard container.

#### Scenario: Entries
- **WHEN** `for (auto [key, v] : user.entries())` runs on the object of `example/document_javascript.cpp`
- **THEN** it prints `name: "Lovelace"`, `tags: ["math","poetry"]`, `home: {...}`, `lang: "en"`, and `for (x : user["tags"])` prints `tag "math"` and `tag "poetry"`

### Requirement: Handles: Every writing function completes or changes nothing
Every writing function (`set`, `erase`, `assign_from`, the array methods, the container views' insertions) SHALL return a `result` and be `[[nodiscard]]`, and SHALL either complete or change nothing: it finds the room it needs in the image before it allocates, so a full image, a value of the wrong kind or a number out of range leave the document as it was. A handle is not a regular value: with exceptions `x = y` writes where `x` points and never makes it point elsewhere, so it must not be swapped or sorted (`std::swap(x, y)` assigns through both).

#### Scenario: Not swappable
- **WHEN** `sugar_test` calls `std::swap` on two handles of two members
- **THEN** both places end up holding the second value and each handle still names its own place

### Requirement: Literals: object, array and literal
`wd::object` and `wd::array` SHALL be values written in C++ as JavaScript writes `{...}` and `[...]`, plain C++ values that a document converts by its traits when it stores them, owning what they hold: `wd::object{{"city", "Recife"}, {"zip", "50000"}}` (members in the order written, a repeated key keeping the last value in the first one's place, `object_member(std::string key, literal value)`), `wd::array{1, "x", nullptr}`, with `members()` and `elements()` views. `wd::literal` SHALL convert implicitly from `bool`, any integer (character types aside; held as `int64_t` or `uint64_t`), the decimal types (a NaN or infinity is held but refused at storing), an 8-byte floating-point value (stored only by binary `number` traits), `const char*` and `std::u16string_view`/`const char16_t*` (a null pointer is `null`), `std::string_view`, `std::string`, `std::u16string`, `date_ms`, bytes (`std::span<const std::byte>`, `std::vector<std::byte>`), `nullptr`, `null`, an `object` and an `array`, with `visit(f)`. A literal is refused whole when one of its values is.

#### Scenario: A nested literal
- **WHEN** `wd::object{{"a", wd::object{{"b", wd::array{1, wd::array{2, wd::object()}}}}}}` is stringified
- **THEN** the text is `{"a":{"b":[1,[2,{}]]}}`

#### Scenario: CWG 1467
- **WHEN** `wd::array{wd::array{1}}` is written on a compiler that applies CWG 1467 to every class (MSVC, Clang before 20) and on one that does not
- **THEN** it is `[1]` on the first and `[[1]]` on the second, so the tests write `wd::array{wd::literal(wd::array{1})}`, and the Doc Comment of `array` states the rule

### Requirement: Arrays: JavaScript's methods
A handle of an array SHALL have `push(x, y...)` (`result<std::uint64_t>`: the new length), `pop()` and `shift()` (`result<document>`: a new owned document holding the removed element, or holding `undefined` when the array was empty), `unshift(x, y...)` (the new length), `splice(start, count, x...)` and `splice(start)` (`result<document>`: the array of the removed elements, deep copies), `reverse()` (`result<void>`), `slice(start, end)` (a new array of copies; also on a `const_value`), `concat(x...)` (a new array: this array's elements, then each argument's: an array, a document or handle holding an array, or a literal holding one, contributes its elements, any other value itself), `at(i)` (a handle), and `set_length(n)` (`a.length = n`: truncates, or appends `null`). The values a method takes are what `set` takes, each copied into the array; `push()` and `unshift()` with no value return the length unchanged.

#### Scenario: The methods of the page
- **WHEN** `example/document_arrays.cpp` applies `push(6, 7)`, `pop()`, `shift()`, `unshift(0)`, `splice(1, 2, "x", "y", "z")`, `reverse()`, `slice(-2)`, `concat([8, 9], 10)` and `at(-1)` to `[1,2,3,4,5]`
- **THEN** the lines are `a.push(6, 7) -> 7, a = [1,2,3,4,5,6,7]`, `a.pop() -> 7, a = [1,2,3,4,5,6]`, `a.shift() -> 1, a = [2,3,4,5,6]`, `a.unshift(0) -> 6, a = [0,2,3,4,5,6]`, `a.splice(1, 2, 'x', 'y', 'z') -> [2,3], a = [0,"x","y","z",4,5,6]`, `a.reverse() -> undefined, a = [6,5,4,"z","y","x",0]`, `a.slice(-2) -> ["x",0]`, `a.concat([8, 9], 10) -> [6,5,4,"z","y","x",0,8,9,10]`, `a.at(-1) -> 0`

### Requirement: Arrays: Indices follow ECMA-262
`detail::relative_index(index, length)` SHALL give an index as ECMA-262's relative index does: a negative index counts from the end and is clamped at 0, an index past the length is the length. `splice`'s `start` follows it, an omitted `count` removes to the end, a negative `count` is 0, a `count` past the end removes to the end, and `slice`'s `end` follows `start`'s rules; the limits `INT64_MIN` and `INT64_MAX` clamp.

#### Scenario: Splice at the limits
- **WHEN** `[0,1,2,3,4]` is spliced with `(INT64_MIN, 1)`, `(INT64_MAX, INT64_MAX, 5)`, `(3, INT64_MAX)` and `(1, INT64_MIN, {"k":true})`
- **THEN** the removed arrays are `[0]`, `[]`, `[3,4]` and `[]` and the arrays after are `[1,2,3,4]`, `[0,1,2,3,4,5]`, `[0,1,2]` and `[0,{"k":true},1,2,3,4]`

#### Scenario: relative_index
- **WHEN** `relative_index` is asked `(-1, 5)`, `(-6, 5)`, `(9, 5)`, `(-1, 0)`
- **THEN** the answers are 4, 0, 5 and 0

### Requirement: Arrays: Dense, with holes as null
Arrays SHALL be dense (`wd::dense`, the traits' only array policy): a longer `length`, or a write past the end, fills the elements between with `null`, which writes the same text JavaScript writes for a hole; so `pop` or `shift` of an element that was a hole returns `null`, where JavaScript returns `undefined`; `pop` and `shift` of an empty array return a document that holds `undefined`; `[undefined]` stores `null`.

#### Scenario: Holes
- **WHEN** `[1]` has `set_length(3)` called, then `a[5].set(6)`, then `pop()` twice
- **THEN** the arrays are `[1,null,null]` and `[1,null,null,null,null,6]`, the first pop returns `6` (leaving `[1,null,null,null,null]`) and the second returns `null` (leaving `[1,null,null,null]`), where JavaScript's second returns `undefined`

#### Scenario: An empty array
- **WHEN** `pop()` is called on `[]`
- **THEN** the returned document holds `undefined` and the array is still `[]`

### Requirement: Arrays: Failures
A method on a handle that is not an array SHALL fail with `errc::type`; a method that inserts many values (`push`, `unshift`, `splice`, `concat`) SHALL find the room for all of them first, so in a full external buffer it returns `errc::capacity` and the array is as it was; the removed or copied elements are returned in a new owned document so that they outlive the array.

#### Scenario: Splice into every size
- **WHEN** `array_test` splices into external buffers filled to every size from empty to full
- **THEN** each call completes or returns `errc::capacity` leaving the array as it was

#### Scenario: Not an array
- **WHEN** `pop`, `shift`, `unshift`, `reverse` and `set_length` are called on an object
- **THEN** each is `errc::type`

### Requirement: Containers: An object as a sorted map
`as_object()` SHALL give `result<basic_object_ref<Handle>>` (`errc::type` for any other kind), a view that owns nothing (two words, cheap to copy, whose constness is its own: `const_object_ref` is the view that only reads) with the types of a map (`key_type` = `std::string_view`, `mapped_type`, `value_type` = pair of the key view and the handle, `iterator`, `reverse_iterator`, `size_type` = `std::uint64_t`), `size`, `empty`, `find(key)`, `contains`, `count`, `insert`, `insert_or_assign` and `try_emplace` (each returning `result<std::pair<iterator, bool>>`: a new key takes the place the traits' policy gives it, an existing key keeps its place, `insert` and `try_emplace` leave its value and `insert_or_assign` assigns), `erase(pos)`, `erase(first, last)`, `erase(key)` (`result<size_type>`: 1 or 0) and `clear`, and `begin`/`end`/`rbegin`/`rend` in the traits' key order. A key that is not valid UTF-8, or a source that reads `undefined`, is `errc::type`.

#### Scenario: The map operations
- **WHEN** `example/document_containers.cpp` inserts `name`, inserts it again, assigns it, emplaces `born`, finds and erases it
- **THEN** it prints `insert name: true`, `insert name again: false, name = Ada`, `insert_or_assign name: false, name = Augusta Ada`, `try_emplace born: true`, `find born: 1815`, `contains born: true, find nope is end: true`, `erase born: 1, contains born: false`

### Requirement: Containers: An array as a vector
`as_array()` SHALL give `result<basic_array_ref<Handle>>` (`errc::type` for any other kind; `const_array_ref` only reads) with `size`, `empty`, `front`, `back` (a handle naming nothing when empty), `operator[]` and `at(index)` (`result<Handle>`: `errc::range` past the end), `push_back`, `emplace_back` (the new handle), `pop_back`, `insert(pos, v)` (`result<iterator>`: later elements move one index up), `erase(pos)` and `erase(first, last)` (the iterator after the erased ones), `resize(n)` (truncates or appends `null`), `clear`, and iteration both ways; an index costs O(log n) because the tree finds an element by its index, and the iterators are bidirectional, not random access.

#### Scenario: A vector's operations
- **WHEN** the page's program applies `push_back(4)`, `emplace_back(5)`, `insert(begin + 1, 9)`, `erase(begin + 1)`, `resize(3)`, `at(10)` and `resize(5)` to `[1,2,3]`
- **THEN** it prints `[1,2,3,4]`, `[1,2,3,4,5]`, `[1,9,2,3,4,5]`, `[1,2,3,4,5]`, `[1,2,3]`, `at(10) is errc::range: true` and `[1,2,3,null,null]`

### Requirement: Containers: by_key and range queries
`as_object()`'s `by_key()` SHALL give `basic_object_by_key<Handle>`: the same members in key order, by UTF-16 code units whatever the encoding (the object's own order under `sorted`), with `size`, `empty`, `begin`, `end`, `rbegin`, `rend`, `find`, `lower_bound`, `upper_bound` and `equal_range`; each step and each bound is a search of a tree, O(log n), and nothing is sorted. A prefix query is the range from the first key at or after the prefix up to the first key at or after the prefix with its last character raised by one; a key that is not valid UTF-8 gives the end.

#### Scenario: A prefix query
- **WHEN** an object with the keys `admin`, `guest`, `owner`, `user`, `user_1` to `user_9` is queried
- **THEN** the JavaScript order lists `user_3 admin user_1 guest user_9 user user_2 owner user_7 user_5 user_4 user_8 user_6`, the `by_key` order lists `admin guest owner user user_1 ... user_9`, the prefix `user_` has 9 keys from `user_1` to `user_9`, `equal_range("user_5")` is one member and `lower_bound("p")` is `user`

### Requirement: Containers: Ranges and validity
`basic_object_ref`, `basic_object_by_key` and `basic_array_ref` SHALL be `std::ranges::bidirectional_range`s, views, sized and borrowed ranges (`enable_view` and `enable_borrowed_range` are specialised), so `<algorithm>` and `<ranges>` read them; their iterators hold ids, which no change reuses: an insertion and any growth of the image keep every iterator and handle valid and invalidate only the `std::string_view`s read from values; an erase invalidates the iterators to the erased member or element and its subtree (the later elements' iterators follow their elements); a replaced value invalidates the iterators to its old subtree; a moved-from or destroyed document invalidates all. A view that reads has no member that writes.

#### Scenario: Algorithms over the views
- **WHEN** the page's program filters the prices above 5 of an array and doubles them
- **THEN** it prints `prices above 5: 3` and `prices above 5, doubled: 16 24 40`, and finds the first out of stock as `pear`

#### Scenario: Iterators across growth
- **WHEN** iterators are held across 20,000 insertions that grow the image
- **THEN** they are all still valid and read their members

### Requirement: Stringify: stringify writes JSON.stringify's bytes
`stringify(v, indent = 0)` SHALL be `JSON.stringify(v, null, indent)` and write its exact bytes, returning a `std::string`: keys in the policy's order, strings escaped as it escapes them, numbers as JavaScript writes them, a date as `Date.prototype.toJSON` writes it, bytes by the traits' policy. It is a function of a handle (`basic_const_value`, which a `value` converts to) and of a document, and a member of both (`doc.stringify(2)`, `doc["user"].stringify()`; a subtree is indented from its own level). The indent is the number of spaces of each level, clamped to 10 (`12` and `UINT32_MAX` write what `10` writes) and 0 writes compactly; an empty array or object is `[]` or `{}` at any indent. Its only failure is the standard library's `std::bad_alloc`.

#### Scenario: Compact and indented
- **WHEN** `stringify_test` writes an object holding arrays, objects, a string of every escape, `null` and `true` with indent 0 and 2
- **THEN** the compact text begins `{"b":[1,{"c":[]},{}],"a":"x\"\\\b\f\n\r\t\u0001\u001f/` and the indented text is `{`, a newline and `  "b": [` with two spaces per level; indent 1 begins `{\n "b": [\n  ` and indent 10 begins `{\n          "b"`

#### Scenario: Nesting and key order
- **WHEN** `{"1":1,"0":0,"a":[[]],"b":{}}` is written with indent 3
- **THEN** the text is `{\n   "0": 0,\n   "1": 1,\n   "a": [\n      []\n   ],\n   "b": {}\n}`

### Requirement: Stringify: Numbers, dates and bytes in text
`stringify` SHALL write numbers by `write_javascript` of the stored digits (`1.5`, `1e+21`, `-9223372036854775808`, `18446744073709551615`, `-0` as `0`, `0.0000001` as `1e-7`); under `js_traits` an integer is written as its double is (`-9223372036854775808` as `-9223372036854776000`, `42` as `42`) and NaN and infinities as `null`; a date as a string `"2026-10-12T03:04:05.678Z"`; and bytes by the policy (`"Af8="`, `"Af8"`, `{"0":1,"1":255}`; an empty value `""` or `{}`), also past any buffer the writer keeps (lengths 299, 300, 301 and 1000 against the encoder).

#### Scenario: Numbers under js_traits
- **WHEN** `4611686018427387904`, `123456789012345678` and the array `[NaN, Infinity]` are written under `js_traits`
- **THEN** the texts are `4611686018427388000`, `123456789012345680` and `[null,null]`

### Requirement: Stringify: An undefined value writes nothing
A value that is `undefined` (a handle of nothing or of a missing member, a document that holds `undefined`, a moved-from document, `wd::const_value()`) SHALL write no text from `stringify` (an empty string), as `JSON.stringify(undefined)` returns none, and no piece from `stringify_to`.

#### Scenario: No text
- **WHEN** `stringify` is given `other["missing"]`, `other["a"]["b"]`, `wd::const_value()` and a document set to `wd::undefined`
- **THEN** each text is empty

### Requirement: Stringify: stringify_to writes the same bytes in pieces
`stringify_to(v, sink, indent = 0, chunk = default_chunk)` SHALL write the same bytes as `stringify` to a sink: any callable taking `std::string_view` and returning `void` or `result<void>` (the concept `detail::sink`), in pieces of at most `chunk` bytes (`default_chunk` = 4096; a chunk of 0 is taken as 1), every piece but the last full and none empty, and return `result<void>`. A failure the sink returns stops the writing and is returned (even on the last piece, which only the final flush hands over); an exception it throws propagates; the document is unchanged either way. The sink is used in place, never copied. A document of the same call takes the document's overload.

#### Scenario: Every chunk size
- **WHEN** `stringify_test` writes the sample with chunk sizes 1 to 64 at indents 0 and 2
- **THEN** each piece is at most `chunk` bytes, every piece but the last is exactly `chunk`, and the pieces joined are `stringify`'s text

#### Scenario: A failing sink
- **WHEN** a sink returns `errc::capacity` on its third call (chunk 4)
- **THEN** `stringify_to` returns `errc::capacity` after exactly 3 calls, `stringify` reads as before and the image is byte for byte unchanged

#### Scenario: The page's sinks
- **WHEN** `example/document_streaming.cpp` writes `{"name":"Ada","tags":["math"],"n":1500}` in pieces of 8 bytes with a bar after each, then with a sink that keeps the first piece and fails
- **THEN** it prints `{"name":|"Ada","t|ags":["m|ath"],"n|":1500}|` and `{"name": then no room left`

### Requirement: Stringify: The writer does not recurse
The JSON writer shared by `stringify` and `stringify_to` SHALL walk a subtree depth-first with a stack of its own that holds node ids, never nodes and never recursion, so that the depth of a document is bounded by memory and not by the thread's stack, and each step finds its node again so that nothing it holds is stale when the image moves.

#### Scenario: 100,000 levels
- **WHEN** `stringify_test` writes a document nested 100,000 levels deep
- **THEN** it completes (Emscripten's memory may grow for it)

### Requirement: Parse: parse, parse_into and parse_or_throw
`parse<Traits = default_traits>(text, options = {})` SHALL be `JSON.parse(text)`: it reads JSON text into a new owned document and returns `result<basic_document<Traits, owned>>`, `noexcept`; `parse_into<Traits>(buffer, text, options)` SHALL read into a document in a 16-aligned buffer the program owns (`errc::capacity` when it is full, for every buffer size from 1 KB, equal to the owned parse otherwise); and `parse_or_throw<Traits>(text, options)` (with exceptions only) SHALL throw `syntax_error` with the byte offset, `range_error` or `capacity_error`. It accepts what RFC 8259 and `JSON.parse` accept and nothing else: no comments, no trailing commas, whitespace of space, tab, line feed and carriage return only; a lone surrogate is refused; a key written twice keeps the last value in the first one's place.

#### Scenario: Rows JSON.parse accepts
- **WHEN** `parse_test` parses ` [1 , 2] `, `{"b":1,"2":2,"a":3,"1":4}`, `{"a":1,"b":2,"a":3}`, `-0`, `1.0` and ` \t\n\r1 \t\n\r`
- **THEN** the written texts are `[1,2]`, `{"1":4,"2":2,"b":1,"a":3}`, `{"a":3,"b":2}`, `0`, `1` and `1`, under both traits

#### Scenario: Texts JSON.parse refuses
- **WHEN** `""`, `   `, `[1,]`, `{"a":1,}`, `// c\n1`, `/* c */ 1`, `[1] 2`, `01`, `+1`, `.5`, `1.`, `NaN`, `'a'`, `"\ud800"`, `"\x41"`, a string with a raw newline, the byte 0xFF, an overlong UTF-8 encoding, a form feed and `[1` are parsed
- **THEN** each is `errc::syntax`, under both traits

### Requirement: Parse: parse_options and their defaults
`parse_options` SHALL hold `max_depth` (the deepest nesting; 256; `errc::capacity` one deeper), `max_bytes` (the most bytes of text, every chunk counted; no limit by default; `errc::capacity` past it), `max_document_bytes` (the largest image an owned document grows to; 64 MiB, `std::uint64_t{64} << 20`; `errc::capacity` past it), `revive_dates` and `revive_bytes` (both false). Each limit is exact. The default for the image is low because each value takes at least 192 bytes of image and a short value's image is a hundred times its text or more: 2 MB of `[0,0,...]` makes an image of 268 MB (201 MB after `shrink_to_fit`), 700 KB of it reaches 64 MiB in 0.4 s with 137 MB resident, where a limit of 1 GiB would take 11 MB of text, 9 s and 2.2 GB. A program that parses larger documents raises it, with `max_bytes`; a document the program builds itself with `create` keeps a limit of 1 GiB.

#### Scenario: Depth, bytes and the image
- **WHEN** `[[[[1]]]]` and `[[[[[1]]]]]` are parsed with `max_depth` 4, `[1,2,3,45]` and `[1,2,3,456]` with `max_bytes` 10, a nesting of 256 and one of 257 with the defaults, and 1,000 strings of 10 bytes with `max_document_bytes` 8192
- **THEN** the first of each pair parses and the second (and the 257-deep nesting and the large array) is `errc::capacity`

#### Scenario: The default image limit
- **WHEN** `[0,0,...]` of 400,000 zeros (800 KB of text, an image of 77 MB) is parsed with the defaults and with `max_document_bytes` of 128 MiB
- **THEN** the first is `errc::capacity` and the second has `size()` 400,000

#### Scenario: A 64 MB string
- **WHEN** a string of 64 MiB is parsed with `max_document_bytes` of 256 MiB and of 32 MiB
- **THEN** the first reads it whole and the second is `errc::capacity`

### Requirement: Parse: The parser in chunks
`basic_parser<Traits, Storage>` (`parser` is the default) SHALL take the text in chunks of any size, split anywhere, down to one byte, inside a string, a number, an escape or a character's bytes: `write(chunk)` returns `result<void>`, `finish()` ends the text and returns the document (without the values `on` took), `reset()` makes it ready for another text, its selectors kept, and a parser is neither copied nor moved. It feeds Boost.JSON's `basic_parser` each chunk (`write_some` with `more`, numbers left as text, the depth limit set), and the handler builds the document node by node by ids, so the document may grow and move between chunks. Every text of the corpus split at every byte, and written one byte at a time, gives the document a whole parse gives. A parser of `external` storage takes a 16-aligned buffer and formats it when the text begins.

#### Scenario: Split anywhere
- **WHEN** the text `{"name":"Ada","tags":["math"],"n":1.5e3}` is written as `{"name":"Ad`, `a","tags":["ma`, `th"],"n":1.` and `5e3}`
- **THEN** `finish()` returns a document that writes `{"name":"Ada","tags":["math"],"n":1500}`

### Requirement: Parse: Errors, offsets and a failed parser
A syntax error SHALL be `errc::syntax`, `offset()` the count of bytes consumed (after a syntax error the offset where it was found, the same however the text was split), and `cause()` the code of Boost.JSON's parser behind it; Boost.JSON's `too_deep` is `errc::capacity` and `exponent_overflow` is `errc::range`; a number the traits refuse is `errc::range`; a limit passed is `errc::capacity`; `max_bytes` counts every chunk written, not each alone. Anything but whitespace after the text is `errc::syntax` whose `cause()` is `extra_data` at the offset where the second text begins, whether it comes in the same chunk or a later one (so `1 2` and `[1] ` then `[2]` are refused, and no selector sees the second text). A parser that failed stays failed (every later `write` and `finish` returns the same error, an exception a callback threw included) until `reset`, and a finished parser takes no more text, which is `errc::syntax`, until `reset`.

#### Scenario: Offsets and causes
- **WHEN** `[1,]` is written, then `[1,2` and `finish`, then `1 2`
- **THEN** the errors are `errc::syntax` at offsets 3, 4 and 2, the last with `cause() == boost::json::error::extra_data`

#### Scenario: The page's errors
- **WHEN** `example/document_streaming.cpp` writes `[1,2`, `,]` and `3]` to one parser, then `[1]`, `finish()` and `[2]` to a reset one, then `[[1],` and `[2]]` to a parser with `max_depth` 2 and `max_bytes` 8, and parses `[[[1]]]` with those options
- **THEN** it prints `write [1,2: ok`, `write ,]: malformed JSON text at 5`, `cause: syntax error`, `write 3]: malformed JSON text at 5` (a failed parser stays failed), `write [1]: ok`, `finish: [1]`, `write [2]: malformed JSON text at 3` (a finished parser takes no more text), `write [[1],: ok`, `write [2]]: no room left at 5` (nine bytes in all) and `[[[1]]]: no room left`

### Requirement: Parse: Revivers
With `revive_dates` a string in exactly `Date.prototype.toJSON`'s format SHALL become a date node, and with `revive_bytes` a string valid in the traits' bytes policy SHALL become bytes (under `javascript_bytes` an object of the shape `JSON.stringify` gives a `Uint8Array`: keys exactly `"0"` to `"n-1"` holding integers from 0 to 255); anything else stays as it was, and without the options every string is a string and every object an object.

#### Scenario: Under javascript_bytes
- **WHEN** `{"u":{"0":102,"1":111,"2":111},"gap":{"0":1,"2":2},"big":{"0":256},"s":"Zm9v"}` is parsed under `js_traits` with `revive_bytes`
- **THEN** `u` is bytes, `gap` and `big` are objects, `s` is a string, and the text written back is the input

### Requirement: Selectors: The grammar
A selector SHALL be a subset of JSONPath: `$`, the root, then any sequence of `.name` (a name starts with a letter, `_`, `$` or a byte of 0x80 or more, and goes on with those and digits), `['name']` (any key, with `\'` and `\\` as its only escapes), `[n]` (an index in decimal without a leading zero, up to `UINT64_MAX`), `[*]` (every element of an array) and `.*` (every member of an object). Text outside the grammar is refused with `errc::type`, returned by `on` or `observe`: no `$` first, spaces, `$..a`, `$[-1]`, `$[00]`, `$[01]`, `$[+1]`, `$[1.5]`, `$["a"]`, an unterminated quote, an escape other than `\'` and `\\`, an index of more than 64 bits.

#### Scenario: Accepted and refused
- **WHEN** `selector_grammar_test` parses `$`, `$.a`, `$['a b']`, `$['it\'s']`, `$[0]`, `$[*]`, `$.*`, `$.items[*].id`, `$['a\\b']`, `$.é`, `$[18446744073709551615]`, and `a`, `$.`, `$[`, `$[-1]`, `$['a`, `$..a`, `$['a\b']`, `$[00]`, `$[18446744073709551616]`, `$ `, `$.1a`, `$.a-b`
- **THEN** the first group is accepted and the second refused, and `$` has no steps

### Requirement: Selectors: Matching
A selector SHALL match a path of exactly as many steps, each matched by its step: a key step matches a member of that key (never an element), an index step an element of that index (never a member), `[*]` any element and `.*` any member. A key may hold any byte, a zero byte included.

#### Scenario: Paths
- **WHEN** `$.a.b` is matched against the path of members `a`, `b`, and against `a` alone and `a`, `b`, `c`
- **THEN** only the first matches; `$[1]` does not match a member named `1`

### Requirement: Selectors: on and observe deliver values as they complete
`parser.on(selector, f)` SHALL deliver each value whose path matches once it is complete and then remove it from the document being built (its blocks zeroed and freed); `parser.observe(selector, f)` SHALL deliver it and keep it. The callback is any callable taking a `const_value` and a `path_view` and returning `void` or `result<void>` (a function by name or address too); it cannot change the document. A scalar is delivered when its token ends, an array or object when it closes; a value inside another that also matches is delivered first (it completes first) and, if `on` took it, the outer value no longer holds it; values no selector matches are built as usual and `finish` returns the document without what `on` removed. A path's indices are those of the text, which `on` does not renumber. `on("$")` takes the root and `finish` then returns a document that holds `undefined`; a number at the top of the text completes only when the text ends, so it is delivered by `finish`.

#### Scenario: items streamed, meta kept
- **WHEN** `{"meta":{"page":1},"items":[{"id":"a","qty":2},{"id":"b","qty":3}],"done":true}` is written to a parser with `on("$.items[*]")`, which prints and sums `qty`, and `observe("$.meta")`, which prints
- **THEN** it prints `$['meta']: {"page":1}`, `$['items'][0]: {"id":"a","qty":2}`, `$['items'][1]: {"id":"b","qty":3}`, `total 5` and `finish: {"meta":{"page":1},"items":[],"done":true}`

#### Scenario: Nested first
- **WHEN** `{"rows":[[1,2,3],[4,5,6]]}` is parsed with `on("$.rows[*][1]")` and `observe("$.rows[*]")`, both printing the path and the value
- **THEN** it prints `$['rows'][0][1]: 2`, `$['rows'][0]: [1,3]`, `$['rows'][1][1]: 5`, `$['rows'][1]: [4,6]` and `finish: {"rows":[[1,3],[4,6]]}`

#### Scenario: The root and a top-level number
- **WHEN** `on("$")` is set on `[1,2]` and on `12`
- **THEN** it prints `$: [1,2]` and `finish: undefined`; for `12` written in pieces, `12 written` and `$: 12` then `finish: undefined`

### Requirement: Selectors: A failing callback fails the parser
A failure a callback returns SHALL fail the parser: `write` returns it unchanged (an error of another category too), and so does every later call, until `reset`. In a build with exceptions an exception a callback throws SHALL fail the parser the same way: `write` or `finish` rethrows it (also when thrown at the end of the text, where the root completes), and every later call throws it again, until `reset`.

#### Scenario: A callback that refuses
- **WHEN** a callback of `$[*]` returns `errc::type` for a value that is not a number and `[1,"two",3]` is written
- **THEN** `write` returns `a value of another kind`, and so does `finish`, as the page prints (`write: a value of another kind`, `finish: a value of another kind`)

### Requirement: Selectors: Bounded memory
A value `on` takes SHALL be erased from the document once delivered, its blocks zeroed and freed, so the next one takes the same room and a stream of any length needs the memory of one element at a time; a callback that must keep a value copies it into a document of its own with `set` or `clone`. The deliveries are the same however the text is split, selectors are kept across `reset`, and `path_view::to_string()` writes a path as a selector of its place (`$['items'][0]`, a key's `'` and `\` escaped and every other byte as it is, control characters included, so it is not RFC 9535's normalized path), which parses back and matches that place.

#### Scenario: A million elements
- **WHEN** an array of 1,000,000 elements `{"i":n,"s":"abcdefghijklmnopqrstuvwxyz"}` (250,000 on wasm targets, where a debug build takes minutes) is written in 4 KB chunks, each taken by `on("$[*]")`
- **THEN** every element is delivered in order with its index `n` as the path's, `finish` returns a document that writes `[]`, and its image is at most 65,536 bytes; the page's program with 100,000 `{"id":n}` prints `100000 delivered, [] left, image under 64 KiB: true`

### Requirement: Selectors: path_view
`path_view` SHALL be the path in the text of a value a selector delivers, valid during the call: `size()` (0 for the root), `is_index(i)`, `key(i)` (UTF-8) and `index(i)` for each step, and `to_string()`.

#### Scenario: A path with awkward characters
- **WHEN** the path of a key holding a newline, a quote, a backslash and U+0001 is written with `to_string()`
- **THEN** the quote and the backslash are escaped, the rest is written as it is, and the text parses back as a selector of that key

### Requirement: Streams: framing, on_error and stream_options
`framing` SHALL be `concatenated` (documents one after another, any JSON whitespace between them or none: indented JSON, `{"a":1}    {"b":2}`, `{"a":1}{"b":2}`, `[1][2]`, `"x""y"`; a top-level number ends at the next byte that cannot continue it, so `1 2` is two documents and `12` one; the default), `lines` (NDJSON, JSON Lines: one document per line) or `rfc7464` (JSON text sequences, RFC 7464: each document in a record that the byte 0x1E opens); `on_error` SHALL be `stop` (the first error ends the stream; the default) or `skip` (a bad document is dropped and its error delivered in its place); `stream_options` SHALL derive from `parse_options` and add `framing` and `on_error`, so each document is held to the limits a parse holds one text to (`max_depth`, `max_document_bytes`, and `max_bytes`, which counts one document's bytes from its first, not the stream's).

#### Scenario: Concatenated
- **WHEN** `{"a":1}    {"b":2}`, `{"a":1}{"b":2}`, `[1][2]`, `"x""y"`, `1 2`, `12 34\n5`, `1[2]3` and `true false null"s"[]{}-0.5` are streamed
- **THEN** the documents are `{"a":1}` `{"b":2}`; `{"a":1}` `{"b":2}`; `[1]` `[2]`; `"x"` `"y"`; `1` `2`; `12` `34` `5`; `1` `[2]` `3`; and `true` `false` `null` `"s"` `[]` `{}` `-0.5`

### Requirement: Streams: NDJSON
With `framing::lines` a document SHALL end at its line's `\n`, which JSON never holds inside a string; a `\r` and any other JSON whitespace around the document on its line, and empty lines or lines of whitespace, are ignored; the last line may end without its `\n` and `finish()` closes it. Two documents on one line, or a document not complete at its `\n`, is `errc::syntax` for that line, so indented JSON is not NDJSON: `{` alone is an error, and so is each line after it until a line holds a whole document again. A document is delivered at its line's `\n`, once the line is known to hold it alone.

#### Scenario: Lines
- **WHEN** `{"a":1}\r\n[2]\r\n`, `\n\n{"a":1}\n\n\r\n \n[2]\n`, `1\n2` and `"a\nb"` (escaped) are streamed with `lines`
- **THEN** the documents are `{"a":1}` `[2]`; `{"a":1}` `[2]`; `1` `2`; and `"a\nb"`

#### Scenario: Bad lines skipped
- **WHEN** `{"a":1}\n{bad}\n\n[2]\n1 2\n"x"` is streamed with `lines` and `skip`
- **THEN** the outcomes are `{"a":1}`, a syntax error at line 2 offset 9, `[2]`, a syntax error at line 5 offset 21, and `"x"`

### Requirement: Streams: JSON text sequences
With `framing::rfc7464` a record SHALL start at 0x1E and end before the next 0x1E or at the stream's end; its document may span lines (indented) and is delivered as soon as its value closes, a top-level number closing at the `\n`, at the next 0x1E or at `finish()`; a record holds nothing but whitespace after its document; an empty record is ignored; whitespace before the first record is ignored and any other byte before it is `errc::syntax`; a bad record is skipped up to the next 0x1E with `skip`.

#### Scenario: Records
- **WHEN** `\x1e{"a":1}\n\x1e[2]\n`, `\x1e12\x1e3\n` and `\x1e\x1e\x1e[1]\n\x1e  \n` are streamed
- **THEN** the documents are `{"a":1}` `[2]`; `12` `3`; and `[1]`

#### Scenario: A byte before the first record
- **WHEN** `x\x1e[1]\n` is streamed with `stop` and with `skip`
- **THEN** `stop` yields a syntax error at 1:0 and `skip` yields it and then `[1]`

### Requirement: Streams: Errors, their places and their policy
A bad document SHALL be `errc::syntax` for malformed text, `errc::range` for a number the traits refuse and `errc::capacity` past a limit, as in a parse. With `on_error::stop` the documents before it are delivered, then every call returns its error until `reset()`; with `on_error::skip` (not allowed with `framing::concatenated`, which has no place to go on from) it is dropped and its error delivered in its place, in the stream's order, and the stream goes on at the next line or the next 0x1E. After an error is delivered, `error_line()` (from 1), `error_offset()` (bytes from the stream's start) and `error_cause()` (Boost.JSON's code behind it: `extra_data` for a second document on a line or in a record) say where it was found. `skip` with `concatenated` is refused: every call of the parser returns `boost::system::errc::invalid_argument`.

#### Scenario: Stop is sticky
- **WHEN** `[1]\n[2]\n[3,}\n[4]\n` is written to a `lines` parser with `stop`
- **THEN** `write` returns `errc::syntax` with `error_line()` 3, `error_offset()` 11 and `offset()` 11; `next()` yields two documents and then the error every time; `write` and `finish` return it too; after `reset()` `write("[5]\n")` queues a document

#### Scenario: Skip with concatenated
- **WHEN** a parser is made with `concatenated` and `skip`
- **THEN** its calls return `boost::system::errc::invalid_argument`

### Requirement: Streams: write, next, the push form, finish and reset
`basic_stream_parser<Traits>` (`stream_parser` is the default) SHALL be sans-IO, take bytes in chunks of any size, down to one byte, and parse each document as `parse` parses one text into a new owned document; `write(bytes)` SHALL queue each document it completes and return how many documents and dropped documents' errors it completed (`result<std::size_t>`); `next()` SHALL take the oldest as `result<std::optional<document>>` (nothing when none is complete, or an error as the policy says); `write(bytes, f)` and `finish(f)` (the push form) SHALL hand each document to `f(result<document>&&)`, returning `void` or `result<void>` whose failure ends the stream with that error, after any that `next()` has not taken, in order; `finish()` SHALL close a top-level number or a last line without its `\n`, and a document the end cuts short is `errc::syntax`; after it the parser takes no more bytes (`errc::syntax`) until `reset()`, an empty `write` and another `finish()` completing nothing; `finished()` says so. `line()` (the line of the last byte consumed, from 1), `offset()` and `documents()` (delivered) SHALL be accessors. A parser is neither copied nor moved. An exception the push form's callback throws propagates, with the bytes up to the document it was given consumed.

#### Scenario: Delivered as soon as complete
- **WHEN** `{"a":` is written, then `1}[`, `]12`, `3`, ` `, `4` and `finish()`
- **THEN** the writes complete 0, 1, 1, 0, 1 and 0 documents, `finish()` completes 1, `next()` yields `{"a":1}`, `[]`, `123` and `4`, then none, and `documents()` is 4

#### Scenario: Lines and offsets
- **WHEN** a `lines` parser is written `[1]`, `\n`, `\n[2]\n3`
- **THEN** `line()` is 1, 1 and 4, `offset()` is 10, `documents()` is 0 until `next()` takes one, and `error_line()` is 0

### Requirement: Streams: Any split gives the same outcomes
However a stream is split, in two at every byte or one byte at a time, `next()` and the push form SHALL yield what one write yields, for every row of the framings and policies and for neighbouring pairs of the parse corpus (40 texts: JSON.parse's accepted rows, multi-byte characters raw and escaped, escapes, numbers, literals, keys of every length from 0 to 40, keys that look like array indices, nesting, repeated keys), and the stream of the corpus yields its documents in order in each framing.

#### Scenario: One byte at a time
- **WHEN** `stream_test` writes the corpus as a `concatenated`, a `lines` (texts without a raw newline, with CRLF) and an `rfc7464` stream one byte at a time
- **THEN** `next()` and the push form yield the documents of one write, each as `stringify` writes it

### Requirement: Asio: async_read_document
`<webcpp/document/asio.hpp>` SHALL offer `async_read_document(stream, buffer, parser, token)`: a composed operation of Boost.Asio over an `AsyncReadStream` that takes the parser's next document when it holds one and otherwise reads from the stream into the buffer and writes what it read into the parser, until a document is complete, taking any completion token (a callback, `asio::use_awaitable`, `asio::deferred`, `boost::cobalt::use_op`) with the signature `void(boost::system::error_code, basic_document<Traits>)`. It completes with the next document; with the error of a document dropped under `skip` (the next call goes on); with the error that stopped the stream under `stop`, every time; with `asio::error::eof` once the stream ended and the parser's `finish` delivered what it held; or with the error of a read; on an error the document is empty. Bytes the buffer already holds are the parser's before any read. One operation at a time reads a stream, its buffer and its parser, which outlive it.

#### Scenario: NDJSON then concatenated JSON over a socket
- **WHEN** `example/document_stream_asio.cpp` has a server write NDJSON then concatenated JSON in pieces of 1, 3, 7, 2 and 13 bytes and a client read in a coroutine and with a callback
- **THEN** it prints `{"id":1,"tags":["a","b"]}`, `{"id":2,"ok":false}`, `[3,null]`, `"last"`, `eof`, then `{"a":1}`, `{"b":[true,null]}`, `[1]`, `[2]`, `{"c":"x"}`, `42`, `eof` (the bare number arriving only when `eof` makes the reader finish the parser)

### Requirement: Asio: Both of Asio's dynamic-buffer models
`async_read_document` SHALL take a buffer of Boost.Asio's first model, `DynamicBuffer_v1` (`asio::streambuf`, Beast's `multi_buffer`, whatever the pieces of its `data`; by reference, which the caller keeps alive until completion; the overload is absent when `BOOST_ASIO_NO_DYNAMIC_BUFFER_V1` is defined), and one of the second, `DynamicBuffer_v2` (what `asio::dynamic_buffer` makes of a string or a vector; by value, as Boost.Asio's own reads take it, the caller keeping alive the storage it names; a buffer of both models is one of the second), reading a buffer of several pieces without joining them, with a read size of what the buffer has room for, at least 512 and at most 65,536, within its `max_size` (`asio::error::no_buffer_space` when none).

#### Scenario: After an HTTP header
- **WHEN** `example/document_stream_beast.cpp` reads a response header with Beast into a `multi_buffer` and then the NDJSON body with `async_read_document` from the same buffer
- **THEN** it prints `status 200 OK`, `content type application/x-ndjson`, `{"id":1,"name":"ada"}`, `{"id":2,"tags":["a","b"]}`, `[3,null]`, `"last"` and `eof`, no byte lost between the header and the body

### Requirement: Asio: Completions are never inside the initiating function
A completion of any Asio operation of the library SHALL never be called inside the function that starts it: one that needs no read is dispatched through `async_immediate` to the handler's immediate executor, or posted to the stream's; and a stream that ended stays ended without another read: once the parser `finished()` the operation completes with `eof` at once (Linux's epoll reactor does not retry a read that returned no bytes, so a second read of an ended socket never completed).

#### Scenario: A reader does not start inside its callback
- **WHEN** `document_stream_asio.cpp` starts the next read from a callback
- **THEN** it posts the read rather than calling it, and the program prints the same lines on every run, its `io_context` drained until it has nothing left, with no clock

### Requirement: Asio: Cobalt and the actor system
`async_read_document` SHALL take `boost::cobalt::use_op` as it takes any token, so that a Cobalt task reads a stream of documents (`cobalt::as_result` turns the completion into a `result` whose error is the stream's, `eof` included); and a `stream_parser` SHALL live inside an actor because it is sans-IO (`example/document_stream_xactor.cpp`: two actors on xactor's FIFO driver, bytes as messages, the push form handing each document to a callback that sends it to the consumer, the document moved in the message with its image where it was).

#### Scenario: Cobalt
- **WHEN** `example/document_stream_cobalt.cpp` has one task write lines in pieces and another read them
- **THEN** it prints `1 {"name":"Ada","tags":["math"],"n":1500}`, `2 [1,2.5,null,true,"x"]`, `3 "plain text"`, `4 {"z":1,"a":2}`, `5 {"last":true}` and `eof after 5 documents`

#### Scenario: xactor
- **WHEN** the parser actor receives chunks cut inside a string, a number and a line
- **THEN** the consumer prints `{"name":"Ada","born":1815}`, `{"name":"Grace","langs":["COBOL"]}`, `line 3 dropped: malformed JSON text`, `{"name":"Alan","n":1500}` and `{"name":"Edsger"}`

### Requirement: Pools: image_pool and pool_block
`image_pool::create(block_bytes, blocks)` SHALL allocate once, in one allocation, `blocks` zeroed blocks of `block_bytes` bytes each, 16-aligned, and return `result<image_pool>`: `errc::range` for a block size that is not a multiple of 16, is below 64 (the image's header) or for no block (`create(100, 2)`, `create(48, 2)` and `create(4096, 0)` are `errc::range`), `errc::capacity` when the allocation fails or its size overflows. `acquire()` SHALL lease a free block (`result<pool_block>`; `errc::capacity` when every block is leased or for a moved-from pool) whose bytes are all zero; a `pool_block` is a lease moved and never copied, and when it is destroyed or assigned the block goes back to the pool with the bytes its holder may have written zeroed. `block_bytes()`, `blocks()` and `available()` describe the pool (0 for a moved-from pool). A pool is moved (its leases stay valid), never copied, must outlive its leases and readers, and is not thread-safe: a block is leased and given back on one thread at a time.

#### Scenario: A pool of two blocks
- **WHEN** `image_pool::create(4096, 2)` is made and two blocks are acquired
- **THEN** `block_bytes()` is 4096, `blocks()` 2, `available()` falls to 0, each block is 4096 bytes at a 16-aligned address, the two differ, and a third `acquire()` is `errc::capacity`; dropping a lease makes `available()` 1

### Requirement: Pools: leased_document
`leased_document<Traits>` SHALL be an `external` document opened in place in a block of an `image_pool` that owns its lease: `open(pool_block)` (the errors of `open` on an external buffer, the block then given back) and `format(pool_block)` (a new empty document that may grow to the whole block); it reads and writes as a document does (`get()`, `root()`, `operator[]` by key and index, `image()`, `seal()`, `sealed()`), a change that needs more room than the block has returns `errc::capacity`, it is moved and never copied (a moved-from lease is `empty()`, its image empty), the document goes before its block does, and when it is destroyed the block goes back to the pool zeroed. A default `leased_document` is the empty lease that an operation which failed completes with.

#### Scenario: A lease holds a block
- **WHEN** a block is acquired, a document built in it with `format`, written to a stream and read back with `leased_document::open`
- **THEN** `stringify(read.get())` is `{"in":"a block"}`, and `available()` is back at its start once the leases are gone

#### Scenario: Zeroed bytes are not opened
- **WHEN** `leased_document<>::open` is given a block of zeros
- **THEN** it is `errc::not_a_document` and the block is given back

### Requirement: Pools: basic_image_reader
`basic_image_reader<Traits>` (`image_reader` for the default traits) SHALL be a sans-IO reader of a stream of images, one after another with nothing between them (an image states its size in its header), into the blocks of an `image_pool`: it takes a stream's first 64 bytes into a block, checks them as a header before it takes a byte more (the magic, format version, byte order, reserved bytes, the layout tag of `Traits`, a size within a block), takes the rest of the image into the same block, opens it there (a sealed image checked against its seal, the heap checked) and queues the block; `write(bytes)` copies the stream's bytes and returns how many it took (all of them, or fewer when every block is leased); `write(bytes, f)` hands each document to `f(leased_document&&)` in order, those not yet taken first; `next()` takes the oldest document (`result<std::optional<leased_document>>`); `prepare()` gives the room the next bytes go into, never empty, never crossing an image's end (`errc::capacity` when a new image needs a block and every block is leased), and `commit(n)` takes `n` bytes written there (0 or 1 documents completed; `errc::range` for more bytes than the room holds); `finish()` ends the stream (an image cut short is `errc::not_a_document`, and a block `prepare` took for an image that never came is given back); `reset()` makes it new, giving back every block; `offset()` and `documents()` count bytes taken and images completed. A reader and a pool may be moved while blocks are leased.

#### Scenario: Several images in one write, split anywhere
- **WHEN** `image_stream_test` writes the images of several documents of different sizes in one `write`, then in two halves at every byte, then one byte at a time, then through `prepare` and `commit` a few bytes at a time
- **THEN** the documents come out equal to their sources, byte for byte in image and in value, `offset()` equals the stream's size and `documents()` the count

#### Scenario: Backpressure
- **WHEN** a pool of two blocks receives three images in one `write` while two documents are held
- **THEN** `write` takes only the first two images' bytes, `prepare()` is `errc::capacity`, giving one back makes `available()` 1, and the rest of the stream is written then

### Requirement: Pools: What a reader refuses
A reader SHALL stop with the error it found until `reset()` (a stream that has lost its framing cannot find the next header), delivering the documents completed before it: `errc::not_a_document` for bytes that are not an image's or an image whose heap is not sound (and for a stream cut short at `finish()`), `errc::incompatible_document` for an image of another format version, byte order, nonzero reserved bytes or traits, `errc::damaged_document` for a sealed image whose bytes do not match its seal, `errc::capacity` for an image larger than a block (after its 64-byte header: `offset()` 64 and the pool back to full); then `write`, `next`, `prepare` and `finish` return the same error, and after `reset()` a good stream reads again.

#### Scenario: A refused header
- **WHEN** the second of three images has its format version raised by one
- **THEN** `write` returns `errc::incompatible_document`, the first image is delivered by `next()`, then `next()`, `write`, `prepare` and `finish` return the error, `available()` is the pool's blocks minus the one held, and after `reset()` (`offset()` 0, `documents()` 0) the good stream is read

### Requirement: Pools: A block holds nothing of the last document, and reading allocates nothing
A block given back SHALL be zeroed in the bytes its holder may have written, so the next holder finds none of the last one's (a reused block read with a smaller image holds nothing of a 'SECRET-' string the first one wrote); and a stream read in every form (write, push form, `prepare`/`commit`), its documents taken and given back, SHALL allocate nothing from the process heap (the pool is allocated once, before; the test replaces the global `operator new` and counts every call, aligned forms included).

#### Scenario: No allocation
- **WHEN** `image_stream_test` reads six streams in every form and the global allocations are counted
- **THEN** the count of calls of `operator new` made is 0

### Requirement: Asio: async_read_image
`async_read_image(stream, pool, token)` SHALL be a composed operation of Boost.Asio, with the signature `void(boost::system::error_code, leased_document<Traits>)` (`Traits` defaulting to `default_traits`), that reads the next image's header into a block of the pool, checks it before reading another byte, reads the rest straight into the same block (no intermediate buffer) and opens the document there, reading exactly the bytes the image needs so that the stream is left at the end of the image it completed. On an error the lease is empty (`leased_document::empty()`): `errc::capacity` when every block is leased (the stream is not read; the end of the stream is reported this way too while the pool is full, since the reader needs a block to learn that it ended), `errc::not_a_document`, `errc::incompatible_document` or `errc::damaged_document` for an image the reader refuses, `asio::error::eof` when the stream ends between two images, an image cut short `errc::not_a_document`, or the error of a read.

#### Scenario: Three images over a socket
- **WHEN** `example/document_image_asio.cpp` receives three sealed documents with a pool of two blocks, in a coroutine and with a callback
- **THEN** each is printed with `sealed, blocks available: 1`, and at `eof` `blocks available: 2`; `{"name":"Ada","born":1815,"langs":["Analytical"]}`, `{"name":"Grace","langs":["COBOL"]}`, `{"name":"Alan","n":1500}`

### Requirement: Asio: async_write_image and async_write_images
`async_write_image(stream, document, token)` SHALL seal the document (taken by non-const reference, since the seal changes it) and write its image as one buffer, completing with `void(boost::system::error_code)`: the error of the seal (which writes nothing) or of the write; the caller keeps the document alive until completion. `async_write_images(stream, documents, token)` SHALL seal every document of a range (`detail::image_range`: a forward, sized range whose elements are lvalues with `seal()` returning `result<void>` and `image()` convertible to `std::span<const std::byte>`, such as documents, leased documents and shared documents; the range is taken by lvalue reference, a temporary is refused) and write all the images in one gather write, `writev` continuing over partial writes, with no copy of their bytes, completing with `void(boost::system::error_code, std::size_t)`: the sum of the images' sizes; the first document that fails to seal completes the operation with its error having written nothing, before any buffer is made; an empty range completes with 0 and writes nothing; on a write error the count is the bytes that reached the stream, which may end inside an image (the whole documents sent are the leading ones whose sizes, added in order, do not exceed it); the documents stay unchanged until completion.

#### Scenario: Three images in one write
- **WHEN** the page's program sends three documents with `async_write_images`
- **THEN** it prints `written: 4176 bytes`, and the receiver prints the three documents sealed with `blocks available: 1` and `eof, blocks available: 2`

#### Scenario: A flipped byte in transit
- **WHEN** `image_asio_test` flips a byte of a sealed image on its way
- **THEN** `async_read_image` completes with `errc::damaged_document` and an empty lease

### Requirement: Sharing: shared_pool
`shared_pool` SHALL be a view (copied freely; the pool is the region's and every view of it, in any process, is the same pool) of a region of memory the program gives, the process heap or a shared-memory segment mapped by several processes each at an address of its own: a 64-byte header (`WCPPPOOL`, version 1, the byte-order mark, the block size, the block count, a tagged lock-free stack top, and the count of free blocks) followed by blocks, each a 64-byte header (a `std::atomic<std::uint64_t>` count of references, the next free block, the image's size) and the room of one image. `region_bytes(block_bytes, blocks)` SHALL size a region (`errc::range` for a block size that is not a multiple of 16 or is below 64, no block, or more than 4,294,967,294 blocks; `errc::capacity` on overflow); `format(region, block_bytes)` SHALL format a 16-aligned region as a new pool of as many blocks as it holds, every one free and zero (`errc::range` for a region that is not 16-aligned or a size outside the bounds, `errc::capacity` for a region too small for one block); `attach(region)` SHALL join a pool another view formatted (`errc::not_a_document` for a region that is not 16-aligned, holds no pool or is shorter than its pool, `errc::incompatible_document` for another layout version or byte order); `block_bytes()`, `blocks()`, `available()` and `region()` describe it; a default view is of no pool and has no block. The pool works through offsets from the region's start and never addresses; leasing a block and giving one back are lock-free and safe from any number of threads and processes at once; nothing is allocated; a free block is all zero and a block given back is zeroed. The static assertion `std::atomic<std::uint64_t>::is_always_lock_free` MUST hold.

#### Scenario: Formatted, attached, refused
- **WHEN** `shared_test` formats memory that holds garbage, attaches again over the same memory, and tries a region that is not 16-aligned, one too small, blocks of a size the pool refuses, memory that holds no pool and a region shorter than the pool it holds
- **THEN** garbage is cleared, the second view is the same pool, and each of the others is refused as listed

### Requirement: Sharing: shared_document
`shared_document<Traits>` SHALL be `std::shared_ptr` for a document in a block of a `shared_pool`: `create(pool)` (a new empty document, its only holder; `errc::capacity` when every block is leased), `from_image(pool, bytes, check)` (copies an image into a free block, checking a sealed image first; `errc::capacity` for an image larger than a block; the image then has the whole block's room, which clears its seal unless it already filled one), copying adds a reference (atomically), destroying drops one, and the last reference zeroes the block and gives it back; every holder reads and writes the same image in place (a write through one is every holder's), `use_count()` says how many hold it in every process, and `get()`, `root()`, `operator[]`, `image()`, `seal()` and `sealed()` read and write as a document does. The count lives in the block's header, never in the image, so the image stays the document's content, the same bytes wherever it lives. Whether several holders may write at once is the program's decision and synchronisation, not the library's.

#### Scenario: Four readers share one document
- **WHEN** `example/document_threads.cpp` stores a document in a pool of 7 blocks of 2048 bytes and four threads each copy it
- **THEN** it prints `pool: 7 blocks of 2048 bytes`, `stored: use_count 1, 6 of 7 blocks free`, `with 4 readers: use_count 5`, `after the readers: use_count 1, 6 of 7 blocks free`, four lines `reader n: Ada was born in 1815`, and after the last copy is dropped `last copy dropped: 7 of 7 blocks free`

### Requirement: Sharing: Copy on write
`make_mutable()` SHALL make the document this holder's alone to change: the same block when it holds the only reference, else a copy of the image (its bytes and its seal) in a new block, this holder's only, while the other holders keep the document as it was and the old reference is dropped; `errc::capacity` when a copy is needed and every block is leased (the holder keeps its reference and its document unchanged), `errc::type` for a holder of no document.

#### Scenario: The writer's copy
- **WHEN** a writer holds one of four references and calls `make_mutable()` then sets `name` to `Lovelace`
- **THEN** the page prints `writer: Lovelace was born in 1815` and `after the writer: Ada was born in 1815, use_count 1, 6 of 7 blocks free` for the document `main` holds

### Requirement: Sharing: release, adopt and drop across processes
`release()` SHALL give up a holder's reference without dropping it and return the image's offset from the region's start (0 for a holder of no document; the holder holds nothing after it); `shared_document::adopt(pool, offset, check)` SHALL take that reference over, in this process or in another that maps the same region at any address (`errc::range` for an offset that is no block's image or a block no one holds; the errors of `open`, which leave the reference as it was); `shared_pool::drop(offset)` SHALL give a reference back by offset without opening the image (so a broken header cannot make a block leak; `errc::range` for an offset that names no block or a block no one holds, changing nothing); `offset()` is the image's offset, the same in every process. A process that dies holding references leaves their blocks counted for good, as Boost.Interprocess's own `shared_ptr` does; recovering them is the program's.

#### Scenario: Two processes
- **WHEN** `example/document_image_interprocess.cpp` formats a pool of eight blocks of 4096 bytes in shared memory, builds four documents (one damaged after it was sealed) and sends their offsets through a message queue to a second run of the program
- **THEN** it prints `available before the consumer: 4`, the three good documents each with `use count 1`, `refused: a sealed image whose bytes do not match its seal` for the damaged one (given back with `pool.drop(offset)`), and `available after the consumer: 8`

### Requirement: Sharing: The pool and its documents allocate nothing
Leasing, copying, moving, releasing, adopting, `make_mutable`, sealing and dropping SHALL allocate nothing from the process heap (the test replaces the global `operator new` and counts), and copies and drops on any number of threads at once, leases and gives-back by more threads than blocks (no block is ever held twice and each comes back zeroed), and copies on write by several threads, SHALL be safe.

#### Scenario: Threads
- **WHEN** `shared_test` runs threads that copy and drop references to one document, lease and give back blocks writing their own value, and copy on write, all at once
- **THEN** the counts are exact, no block is leased twice and every block returns zeroed

### Requirement: Sugar: The members that throw
With exceptions only, the library SHALL provide, each as its core function followed by `detail::raise` on failure and each carrying the constraint `detail::exceptions_enabled`: the handle's `operator=(const T&)` (for a settable `T` or `undefined_t`) and `operator=(const basic_value&)` (a deep copy), `as<T>()` (for a readable `T`), `as_string()` (reads UTF-8 strings only: under `js_traits` a program uses `as<std::u16string_view>()` or `as<std::string>()`), `as_bool()`, `parse_or_throw`, and the owned document's default constructor (holding `null`) and copy constructor (an equal and independent copy; `capacity_error`/`type_error` for a moved-from source). Each throws the exception of the code the core function returns for the same call and changes nothing: `errc::type` throws `type_error`, `errc::range` `range_error`, `errc::capacity` `capacity_error`, and `errc::syntax` `syntax_error` with its offset; an external document that is full throws `capacity_error` where `set` returns `errc::capacity`.

#### Scenario: The page's sugar
- **WHEN** `example/document_javascript.cpp` parses `{"n":1,"user":{"name":"Ada"}}` with `parse_or_throw`, assigns `doc["n"] = 2` and `doc["user"]["nick"] = "Countess"`, reads `as<std::int64_t>()` and `as_string()`, assigns the handle `place = doc["user"]["name"]`, assigns `wd::undefined` to `nick`, writes `doc["n"]["x"]["y"] = 1`, reads `doc["user"].as<bool>()`, parses `[1,]` with `parse_or_throw`, and copies the document into `twin`, setting `twin["n"] = 3`
- **THEN** it prints `2 Ada`, `doc.n -> "Ada"`, `doc.user -> {"name":"Ada"}`, `TypeError: a value of another kind` twice, `SyntaxError: malformed JSON text at 3` and `null {"n":"Ada","user":{"name":"Ada"}} {"n":3,"user":{"name":"Ada"}}`

#### Scenario: Assignment never rebinds
- **WHEN** `doc["c"] = doc["b"]` is executed on temporaries, and a handle is assigned itself
- **THEN** the value of `b` is stored at `c`, no handle is rebound, and `o.b = o.b` writes its own value back

### Requirement: Sugar: Refused without exceptions
In a build without exceptions the sugar SHALL stay declared but not satisfiable: each member carries `detail::exceptions_enabled`, which no such build satisfies, so a call does not compile and the compiler's diagnostic names the constraint (for `as<std::int32_t>()`, Clang writes "no matching member function for call to 'as'" with the note "because 'int' does not satisfy 'exceptions_enabled'"). `test/no_exceptions_test.cpp`, built with `<exception-handling>off` as a compile-diagnostic test (`webcpp.compile-diagnostic`, `clang -verify`), holds the diagnostics of the default and copy constructors, both assignments, `as`, `as_string`, `as_bool` and `parse_or_throw`. A generic program asks whether the sugar is there with the concept.

#### Scenario: Each member refused
- **WHEN** `no_exceptions_test.cpp` calls each member that throws
- **THEN** every call stops with the expected error and the note that names `exceptions_enabled`

### Requirement: JS: The module and its wrapper
`js/webcpp-document.mjs` SHALL be an ES module that imports `webcpp-document-core.js` (the Emscripten build of `js/binding.cpp`, built by `js/Jamfile` as `webcpp-document-core` with `--bind`, `-sMODULARIZE=1`, `-sEXPORT_ES6=1`, `-sENVIRONMENT=web,worker,node`, `-sINVOKE_RUN=0`, `--no-entry`, `-sALLOW_MEMORY_GROWTH=1`, `-sMAXIMUM_MEMORY=4GB` and `-sFILESYSTEM=0`) and awaits it when it loads, exporting `Document`, `StreamParser` and `diagnostics` (`heapBytesInUse()`, the bytes of the wasm heap the allocator has handed out and not taken back). `js/binding.cpp` SHALL register `NativeDocument`, `NativeStream`, `parseText`, `openBytes` and `heapBytesInUse` with embind; every operation returns an outcome (a code, a message and a value) and nothing throws into JavaScript, nor does the binding use the library's sugar, so the module also builds and passes its tests with `exception-handling=off` (where Boost's handler is linked in). It runs in Node, in a page and in a Worker.

#### Scenario: The module loads
- **WHEN** a Node program imports `Document` from `webcpp-document.mjs`
- **THEN** the import completes after the WebAssembly is instantiated (top-level `await`), and `Document.parse('{"a":1}').get(['a'])` is 1

### Requirement: JS: Document is addressed by paths
`Document` SHALL be made only by `Document.parse(text)` (`JSON.parse`, throwing the `SyntaxError` `JSON.parse` throws) or `Document.open(bytes)`; its constructor throws `TypeError` for anything else. A place is a path: an array of string keys and non-negative safe-integer indices from the root, `[]` being the root; a path that is not an array, or has another step, is `TypeError`. `get(path)` returns a scalar as its JavaScript value and a container as a copy, and `undefined` where the path leads to nothing; `set(path, value)` writes what `JSON.stringify` writes for the value (`undefined` erases a member; `-0` is `0`; a function, a symbol or a BigInt is `TypeError`); `erase(path)` says whether the key was there; `push(path, ...values)` returns the new length (an `undefined` element is `null`); `splice(path, start, deleteCount, ...items)` takes `Array.prototype.splice`'s arguments (negative positions and clamped counts; without a count, every element from `start`) and returns the removed elements; `length(path)`; `pop`, `shift`, `unshift` (values entered last first so they stand in front in order), `reverse` (gives nothing back), `setLength(path, n)` (`RangeError` for a value that is no array length), `assign(path, source)` (`Object.assign`: `null` and `undefined` add nothing, a scalar or a target that is not an object is `TypeError`); `toJSON()` is `get([])` so `JSON.stringify(doc)` works; `stringify(indent = 0)` is `JSON.stringify(doc, null, indent)` clamped to 0 to 10 and `undefined` for a document that holds `undefined`.

#### Scenario: get, set and erase
- **WHEN** `api.test.mjs` parses `{"user":{"name":"Ada","age":36,"langs":["C"]},"n":null,"t":false}`
- **THEN** `get(['user','name'])` is `'Ada'`, `get(['user','age'])` 36, `get(['user','langs'])` deep-equals `['C']`, `get(['n'])` null, `get(['t'])` false, `get(['missing','deeper'])` and `undefined`, `get(['user','langs',0])` `'C'`; a set of `{ a: [1, 'two', null] }`, `3.14159`, `1e21` and `-0` stringifies as `JSON.stringify` does of the same object

#### Scenario: Array methods
- **WHEN** `{"langs":["C","Rust","Go"]}` has `push(['langs'], 'Note G')`, `splice(['langs'], 0, 1)` and `splice(['langs'], -1, 1, 'JS', 'Lisp')`
- **THEN** the results are 4, `['C']` and `['Note G']`, and the document is `{"langs":["Rust","Go","JS","Lisp"]}`

### Requirement: JS: Errors are JavaScript's
An operation that fails SHALL throw the error JavaScript would throw for it, from the `errc` value: `SyntaxError` for `syntax`, `RangeError` for `range` and `capacity` (a number out of range, no room left in a buffer, a reservation past the limit, a bad size), and `TypeError` for `type`, `not_a_document`, `incompatible_document` and `damaged_document` (a value of another kind, a write through something that is not a container, a value that is not JSON, a path that is not an array of keys and indices, bytes that are not an image, a sealed image that is damaged, a document that was freed); an operation that throws changes nothing.

#### Scenario: What throws TypeError
- **WHEN** `set(['missing','x'], 1)`, `push(['obj'], 1)`, `erase(['arr', 0])`, `splice(['num'], 0, 1)`, `getDecimal(['str'])`, `keysInRange(['arr'], 'a', 'b')`, `set(['f'], () => 1)`, `get(['arr', 1.5])`, `get(['arr', -1])` and `get('arr')` are called
- **THEN** each throws `TypeError`, and `Document.parse` of `{`, `[1,]`, `{"a":1,}`, the empty text, `nul`, `{"a" 1}` and `01` throws `SyntaxError`

### Requirement: JS: Exact numbers and ordered keys
`getDecimal(path)` SHALL give the exact decimal of a number as text (`0.1234567890123456789012345678901234` for the price, `1.5` for `1.50`), where `get` gives the `Number` that rounds it; `stringify` SHALL write the digits the document holds (`12345678901234567890` where `JSON.stringify(JSON.parse(text))` writes `12345678901234567000`, and `1e+400` where JavaScript writes `null`), the exactness caveat the README states; and `keysInRange(path, low, high)` SHALL give the keys of an object from `low` up to and not including `high`, in key order by UTF-16 code units, `TypeError` for a non-object and for bounds that are not strings; a prefix query is the range from the prefix to the prefix followed by U+FFFF. Keys come out in JavaScript's order (`Object.keys(doc.get([]))` of `{"b":1,"2":2,"a":3,"1":4,...}` is `['1','2','b','a',...]`).

#### Scenario: A price of thirty-four digits
- **WHEN** `{"price":0.1234567890123456789012345678901234,"small":1.50}` is parsed
- **THEN** `getDecimal(['price'])` is that text, `get(['price'])` is its `Number`, `getDecimal(['small'])` is `'1.5'` and `get(['small'])` 1.5

#### Scenario: A key range
- **WHEN** `{"users":{"user_2":{},"user_10":{},"admin":{},"user_1":{},"user_":{},"user`":{}}}` is asked `keysInRange(['users'], 'user_', 'user_￿')` and `('x', 'y')`
- **THEN** the answers are `['user_','user_1','user_10','user_2']` and `[]`

### Requirement: JS: Images, memory and seals
`byteLength` SHALL be the image's size and `capacity` the bytes it can hold before a change must move it; `image()` a `Uint8Array` that views the bytes where they are in WebAssembly memory with no copy (the same `buffer` on each call), valid until the memory grows or the document changes (`slice()` keeps it); `Document.open(bytes)` makes a document of an image, in place when the bytes are a 16-aligned view of WebAssembly memory, else from a copy into the module's memory (a memory copy and no parse; the document is its own and the bytes may change or go), refusing bytes that are not an image with `TypeError`; `seal()` writes the CRC into the image, which `open` checks; `reserve(bytes)` makes the image hold at least that many (`RangeError` past the limit, for a negative or fractional size); `free()` releases the image at once, a second `free()` does nothing, and a freed document refuses every operation with `TypeError`. A document never freed is released when the collector collects its object, through a `FinalizationRegistry`, as a safety net and not a plan; the module's memory grows up to 4 GB and is never returned to the system. An image is trusted input.

#### Scenario: Seal and open
- **WHEN** `{"a":[1,2,3]}` is parsed, sealed, its image sliced and opened, then its last byte is flipped and opened
- **THEN** the first open stringifies `{"a":[1,2,3]}` and the second throws `TypeError`; `doc.byteLength` equals `doc.image().byteLength`

#### Scenario: The collector
- **WHEN** 50 documents of 32 KB each are parsed and dropped without `free()`, with `--expose-gc`
- **THEN** `diagnostics.heapBytesInUse()` first rises by at least 50 times 32 KB and returns to within 4 KB of its start after collections; a freed document is not released a second time

### Requirement: JS: Images transferred to a Worker
A page SHALL be able to send a document to a Worker without a parse: it copies the image into an `ArrayBuffer` of its own (`doc.image().slice()`, since a view of WebAssembly memory cannot be transferred) and transfers the buffer (`worker.postMessage({ image: bytes.buffer }, [bytes.buffer])`), which moves in constant time and detaches it from the sender; the Worker opens the image in its own memory with `Document.open(new Uint8Array(data.image))`, one memory copy, and reads from it. `js/example/page.mjs`, `worker.mjs` and `image-worker.html` are that example, and a headless Chromium runs it and fails unless the Worker answered what the document holds (`name` `'Ada'`, `age` 36, `langs` `['C','Note G']`, `price` `'1.5'`, the text `{"user":{"name":"Ada","langs":["C","Note G"],"age":36},"price":1.5}`) and the transfer detached the buffer.

#### Scenario: The transfer detaches
- **WHEN** the Node smoke test transfers `{"price":1.5,"n":12345678901234567890}`'s image to a `worker_threads` Worker
- **THEN** `bytes.byteLength` is 0 afterwards and the Worker answers `{"price":1.5,"n":12345678901234567890}`

### Requirement: JS: StreamParser
`StreamParser` SHALL be the stream parser with `framing` `'concatenated'` (the default), `'lines'` or `'rfc7464'` and `onError` `'stop'` (the default) or `'skip'` (a `TypeError` for any other value, and for `'skip'` with `'concatenated'`): `write(chunk)` takes a string (encoded as UTF-8), an `ArrayBuffer` or a view of bytes (`TypeError` otherwise), `read()` the next complete `Document` or `undefined` (the error that ends a stream is thrown, `SyntaxError` for malformed text, once the documents before it are taken; with `'skip'` a dropped document is counted in `dropped`), `finish()` ends the stream (`SyntaxError` inside a document), and `StreamParser.from(source, options)` is an async generator over a `ReadableStream` of bytes or strings, or any async iterable of them, yielding a `Document` as soon as the bytes that end it arrive, whatever the chunks (the documents a failing write completed come out before its error does); the end of the source ends the stream as `finish()` does. Each document is a `Document` in WebAssembly memory, which the program frees. It is the way a sender written in JavaScript feeds the parser compiled from C++ chunk by chunk.

#### Scenario: NDJSON in odd chunks
- **WHEN** five documents (including `{"b":"café 😀"}` and `{"d":"x\ny"}`) as NDJSON are fed through `StreamParser.from` as chunks of 1, 3, 7 and 4096 bytes
- **THEN** the documents come out deep-equal to the five, whatever the chunk size

#### Scenario: A bad line
- **WHEN** `{"a":1}\n{bad\n[2]\n` is read with `lines`, with `stop` and with `skip`
- **THEN** with `stop` `{a:1}` is yielded and then a `SyntaxError` is thrown; with `skip` `{a:1}` and `[2]` are yielded

### Requirement: JS: Documents shared between workers
`js/shared/webcpp-document-shared.mjs` SHALL be a second build of the module (Emscripten with Wasm Workers on pthreads, `-sINITIAL_MEMORY=128MB` and no growth so that a full heap is an error value and not an abort, `-sABORTING_MALLOC=0`, `join.js` as the runtime of a Worker that joins) whose WebAssembly memory is one `SharedArrayBuffer`: the thread that imports it (a page, or Node's main thread) starts the module and lays out the memory once, a pool of 64 blocks of 512 KiB, 1,024 tickets for tokens in flight and 32 Workers' slots (64 KiB of stack each); a Worker the program creates receives `connection()` in a message, awaits `join(connection)` and from then on runs the same module on the same memory until it calls `leave()` (which drops every reference it holds, gives its slot back and may join again; the main thread cannot leave). Every document is in the pool: `Document.parse` takes a block (`RangeError` when none is free), `doc.share()` returns a token `{ offset, ticket, generation }` that `postMessage` clones and that keeps one reference until `SharedDocument.adopt(token)` takes it over once or `SharedDocument.drop(token)` gives it back (a spent token, a forged one or anything that is not a token is `TypeError`, even when its block holds another document by then), `useCount` counts the references of every Worker, the last given up by `free()` or the collector gives the block back, `makeMutable()` copies on write (`RangeError` when no block is free), a write past a block's room is `RangeError`, and `pool.available`, `pool.blocks` and `pool.blockBytes` tell what is left; `SharedDocument` is an alias of `Document`. A thirty-third `join` is `RangeError`; a Worker terminated without `leave()` keeps its references and slot for the life of the memory; a browser page needs `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` (Node needs neither); and `join.js` reads internals of Emscripten 6.0.11 (`createWasm`, `wasmMemory`, `updateMemoryViews`), so the module is tied to that version until checked against another.

#### Scenario: Four Workers read one image
- **WHEN** a page parses `{"user":{"name":"Ada","langs":["C","Note G"]},"price":1.50}` and gives its token to four Workers, one of which then calls `makeMutable()` and sets the name to `Grace`
- **THEN** the example prints `cross-origin isolated: true`, `page: parsed into one block of the pool, use count 1`, four lines `worker n: reads Ada and C, Note G, at the page's address: true, in a SharedArrayBuffer: true`, `page: use count 5, blocks in use 1`, `worker 4: makeMutable, then set: reads Grace, at the page's address: false`, `page: reads Ada, use count 4, blocks in use 2`, `page: every worker freed its reference, use count 1`, `page: freed, every block back in the pool: true`

#### Scenario: A spent token
- **WHEN** `doc.share()` is dropped with `SharedDocument.drop(token)` and then adopted or dropped again
- **THEN** `doc.useCount` is 1 after the first drop and both later calls throw `TypeError`

### Requirement: JS: The npm package @webcpp/document
`js/npm/` SHALL stage the npm package `@webcpp/document` into a directory of the build (`stage.mjs`; never into the source tree; the staging directory is made again from nothing): the release build of the module (`-O3`, `--emit-tsd` for its declarations) as `webcpp-document-core.js`, `.wasm` and `.d.ts` (the `.wasm` found through `new URL(..., import.meta.url)`, which bundlers resolve), the wrapper `webcpp-document.mjs`, the hand-written declarations `webcpp-document.d.mts`, `README.md` and `LICENSE_1_0.txt`, with the manifest written from `js/npm/package.json` (the template) and the version of the dev package `js/package.json` (0.1.0); the manifest's `files` and the staged files MUST agree (stage.mjs fails otherwise). The manifest SHALL name `@webcpp/document`, type `module`, `main` `./webcpp-document.mjs`, `types` `./webcpp-document.d.mts`, `exports` `.` (types and default) and `./package.json`, `sideEffects` false, `engines` `{"node": ">=22"}`, license `BSL-1.0`, author `WebCpp.org`, repository `git+https://github.com/webcpporg/document.git` directory `js`, homepage `https://webcpporg.github.io/libs/document/`, and `publishConfig` `{"access": "public", "provenance": true}`; the package has no dependencies. Publishing is never a b2 target.

#### Scenario: What npm pack ships
- **WHEN** `js/npm/test/run.mjs` runs `npm pack --dry-run --json` on the staged directory
- **THEN** the shipped files are exactly the manifest's `files` plus `package.json`, none empty

#### Scenario: Installed as a user receives it
- **WHEN** the tarball is installed offline into a fresh directory
- **THEN** `smoke.mjs` passes under `node --test`, and `tsc --noEmit` is clean for `tsconfig.nodenext.json` and `tsconfig.bundler.json`

### Requirement: JS: TypeScript declarations
`webcpp-document.d.mts` SHALL declare `JsonValue`, `Path` (`ReadonlyArray<string | number>`), `Chunk` (`string | ArrayBuffer | ArrayBufferView`), `Framing`, `StreamParserOptions`, `ChunkReadable`, `diagnostics`, `Document` (a private constructor; `parse`, `open`, `get`, `set`, `erase`, `push`, `splice` in two overloads, `length`, `getDecimal`, `keysInRange`, `stringify`, `toJSON`, `byteLength`, `capacity`, `reserve`, `pop`, `shift`, `unshift`, `reverse`, `setLength`, `assign`, `image`, `seal`, `free`) and `StreamParser` (`dropped`, `write`, `read`, `finish`, `from`), by hand, because embind's own declarations describe the module the wrapper uses and not the wrapper. `js/npm/test/consumer.ts` SHALL use every one under `strict`, and end with calls that must not type-check (`doc.get('user')`, `doc.get([true])`, `new StreamParser({ framing: 'csv' })`, `new Document()`, `doc.keysInRange([], 1, 2)`, `StreamParser.from(3)`).

#### Scenario: Types are checked
- **WHEN** `tsc --noEmit -p tsconfig.nodenext.json` is run on `consumer.ts` in the installed package
- **THEN** the declarations are accepted for the valid calls, which `smoke.mjs` runs as well

### Requirement: JS: Tests and the lane js
The JavaScript API SHALL be tested by the own lane `js` of the library (`test/js/Jamfile`; `b2 toolset=emscripten libs/document/test/js//js`), which runs on Emscripten and whose targets are driven tests (`webcpp.drive`, each building the module it checks so that the lint and the CI compile `js/binding.cpp`, `js/binding.hpp` and `js/shared/binding.cpp`): `js-tests` (Node's runner with `--expose-gc` on `api.test.mjs`, `corpus.test.mjs`, `finalization.test.mjs`, `image.test.mjs` and `stream.test.mjs`), `shared-tests` (`sharing.test.mjs` with Workers from `holder.worker.mjs`), `npm-package-test` (the staged package, packed and installed), `js-chrome` (the example page in headless Chromium through Playwright, cross-origin isolated) and `shared-chrome` (compares the lines the shared example prints with `shared-workers.expected`). `corpus.test.mjs` SHALL replay the oracle's cases through the JavaScript API (marking a case `js_only` as a documented difference in exact decimals). The dev package pins Playwright 1.59.0 and TypeScript 6.0.3; the browser is the Chrome the environment variable `CHROME` names, else Playwright's Chromium.

#### Scenario: The corpus through the wrapper
- **WHEN** `corpus.test.mjs` replays every case of `test/fixtures/cases` through `Document`
- **THEN** the written text and the returned values equal the expected files, byte for byte, for every case that is not `js_only`, and equal after `JSON.parse` and `JSON.stringify` for those that are

### Requirement: Tests: The unit tests and how they are registered
The library SHALL have `lightweight_test` programs, one per subject, under `test/`, each declared in `test/Jamfile` with `webcpp.run <name> : <file>_test.cpp`, `main` returning `boost::report_errors()` and each case a function named after the rule it checks (`handle_survives_growth`), on native, Emscripten, wasip2 and wasip3: `errors` (and `errors_raise`, built with exceptions), `config`, `config_developer_defined`, `config_all_off`, `number`, `traits`, `heap`, `selector_grammar`, `tree`, `literal`, `convert`, `value`, `order`, `image`, `stringify`, `parse` (and `parse_sugar` with exceptions), `streaming`, `selector`, `stream`, `stream_asio` (native only), `array`, `sugar` (with exceptions), `no_exceptions` (a compile-diagnostic test), `oracle_cases`, `bench` (the benchmarks' smoke pass), `image_stream`, `image_asio` (native only), `containers`, `seal`, `shared` (and `shared_interprocess`, native, with exceptions), `compact` and `format`. Every test is also built without exceptions unless it declares `<exception-handling>on`. The tests that need a large heap (`stringify`, `parse`, `streaming`, `containers`, the benchmarks) link Emscripten with `-sALLOW_MEMORY_GROWTH`. `test/require.hpp` SHALL give `webcpp::test::require(bool)`, which exits the program with `boost::report_errors()` when a precondition of a case fails.

#### Scenario: One test while iterating
- **WHEN** `b2 toolset=clang libs/document/test//value` is run
- **THEN** only the `value` program is built and run

### Requirement: Tests: Every public header compiles alone
`test/Jamfile` SHALL compile each public header, `document.hpp`, `document/[^a]*.hpp` and `document/detail/*.hpp`, as the only include of its translation unit on every target, and `document/asio.hpp` alone natively, so that a header that leans on an include it does not make fails and names it.

#### Scenario: A header leaning on another
- **WHEN** a header uses a type without including its header
- **THEN** its headers-alone target fails to compile

### Requirement: Tests: Counting allocations
Tests that claim allocation-freedom (`image_stream`, `shared`) SHALL replace the global `operator new` and `operator delete` of their own program, aligned forms included (the forms without `std::nothrow_t`, which the standard library's nothrow forms call; none inlined, so that GCC does not pair a `malloc` inside one with a `free` and warn of a mismatch), count every call, and assert the count made during the work is 0; the replacements are the test program's instrument, the one place a mutable global is allowed, and no library header holds one. Memory of an alignment comes from `aligned_alloc` or, on MSVC, `_aligned_malloc` and `_aligned_free`.

#### Scenario: Reading allocates nothing
- **WHEN** a pool is allocated, then a stream is read in every form and the documents taken and dropped
- **THEN** `global_allocations - before` is 0

### Requirement: Tests: Node as the oracle
`test/oracle/oracle.mjs` SHALL run every case of `test/fixtures/cases/*.json` on JavaScript itself (`JSON.parse`, the operations, `JSON.stringify(result, null, indent)`) and write `test/fixtures/expected/<name>.json` (`text` and `returns`, one per operation: the compact `JSON.stringify` text of what the operation returns, or null for `undefined`), needing no package (`b2 libs/document/test/oracle//oracle` is the lane that checks the committed files against Node; `//update-expected` writes them again, after a case is added or changed). A case is `text`, `indent` (0 to 10), `js_only` and `operations` (`set`, `erase`, `push`, `pop`, `shift`, `unshift`, `splice`, `reverse`, `set_length`, `assign_from`), every value JSON text in a string so that neither side reads a number through a double; `js_only` is true exactly when a number is one a double does not keep (18 to 30 digits, `9007199254740993`), and such a case runs with `js_traits` alone while every other runs with both traits. The script refuses a case that holds what the two sides do differently by design or JavaScript throws on: a lone surrogate, a number past a double's range or of more than 34 significant digits written, a path through a missing member or element, a key step on an array or an index step on an object, a method on what is not an array, `erase` or `assign_from` on what is not an object, the key `__proto__` in an operation, `pop` or `shift` of a hole. `test/oracle/generate.mjs` SHALL write the 200 random cases `random-000` to `random-199` from a fixed seed (xorshift32, seed 20261012): documents nested to five levels with keys from a set that includes array-index look-alikes, 1 to 10 operations each kept only once the oracle accepts it. The hand-written cases (54 files: `arrays-*`, `bytes-*`, `dates-tojson`, `indent-*`, `numbers-*`, `order-*`, `parse-whitespace`, `strings-*`) follow the rules of the page.

#### Scenario: A case through both sides
- **WHEN** the case `arrays-splice-1-2` (`[0,1,2,3,4]`, `splice([], 1, 2, [])`) is run
- **THEN** Node writes `[0,3,4]` with the return `"[1,2]"`, and `oracle_cases` produces the same text and return from the document under both traits

#### Scenario: A number a double does not keep
- **WHEN** the case `numbers-long` (`js_only`) is run
- **THEN** its expected text is Node's `[123456789012345680,1.2345678901234568e+29,...]` and C++ runs it under `js_traits` only

### Requirement: Tests: The oracle test names a missing file
`oracle_test.cpp` SHALL run every case of the fixtures' directory (`WEBCPP_TEST_DOCUMENT_FIXTURES`, defined with forward slashes by `test/Jamfile`; wasmtime is given the directory through `webcpp.data`) through the document's core and compare the text and each return with Node's, byte for byte, under `default_traits` and `js_traits`, and name the first case whose expected file is missing.

#### Scenario: A case without its expected file
- **WHEN** a case has no file in `fixtures/expected`
- **THEN** the test fails naming that case

### Requirement: Tests: Corpus, splits and limits
The tests SHALL pin the review's focus areas: a handle kept across growth (`handle_survives_growth`); a full external buffer in the middle of a compound operation, run in buffers filled to every size from empty to full (`splice_in_every_size`, `copy_in_every_size`, `parse_into_every_size`); a chunk boundary inside a token (a multi-byte UTF-8 character, an escape `"\u12` then `34"`, a number `12` then `.5e3`, a literal `tr` then `ue`, a key), every text of the corpus split at every byte and one byte at a time; keys that look like array indices; numbers at the edges of their text (`-0`, `0e0`, `1E400`, `1e-400`, `0.0000001`, `1e21`, 39 digits, `1.000000000000000000000000000000000000000`, `9223372036854775807`, `9223372036854775808`, `-9223372036854775809`); and depth and size at their limits (nesting at `max_depth` and one deeper, a 64 MB string, an object of 100,000 keys, a selector on a million elements).

#### Scenario: Every split
- **WHEN** `streaming_test` writes each text of the parse corpus (40 texts) split at every byte and one byte at a time
- **THEN** the document equals the one a whole parse gives, and a syntax error's offset is the same however the text is split

### Requirement: Docs: The page
`doc/document.adoc` SHALL be the guide, built by `b2 libs/document/doc` (`doc/Jamfile`: `webcpp.doc document : document.adoc ; webcpp.reference document ;`, strict) with the title "document: JSON as JavaScript has it, for {cpp}", the attributes `:toc: left`, `:toclevels: 1`, `:idprefix:`, `:sectanchors:`, `:example-caption!:` and `:attribute-missing: warn`, and one file per section, in this order: `introduction.adoc` (Introduction), `gains.adoc` (What C++ gives JavaScript), `js-module.adoc` (Using it from JavaScript), `javascript.adoc` (From JavaScript), `handles.adoc` (Handles), `numbers.adoc` (Numbers), `strings.adoc` (Strings, dates and bytes), `order.adoc` (Key order), `arrays.adoc` (Arrays), `containers.adoc` (Containers), `stringify.adoc` (Writing JSON), `parsing.adoc` (Parsing), `streams.adoc` (Streams of documents), `selectors.adoc` (Selectors), `traits.adoc` (Traits), `images.adoc` (Images), `wire.adoc` (Images on the wire), `sharing.adoc` (Sharing), `boost-json.adoc` (Compared with Boost.JSON), `exceptions.adoc` (The sugar that throws), `errors.adoc` (Errors), `testing.adoc` (Testing), `benchmarks.adoc` (Benchmarks), then the reference (`include::{reference}[leveloffset=+1]`) and `changelog.adoc`. The page leads with what C++ gives JavaScript, says where document loses (parsing slower than Boost.JSON and `JSON.parse`; an image many times its text and larger than the compressed text; a seal that is not authentication; a lookup that walks a tree), and teaches concept by concept from JavaScript's side.

#### Scenario: The sections
- **WHEN** `doc/document.adoc` is read
- **THEN** it includes the 23 section files above in that order, then the reference and the changelog

### Requirement: Docs: See lines name section titles
Each `@see "<title>"` of a Doc Comment SHALL name a section's title ("Introduction", "What C++ gives JavaScript", "Using it from JavaScript", "From JavaScript", "Handles", "Numbers", "Strings, dates and bytes", "Key order", "Arrays", "Containers", "Writing JSON", "Parsing", "Streams of documents", "Selectors", "Traits", "Images", "Images on the wire", "Sealed images", "Sharing", "Compared with Boost.JSON", "The sugar that throws", "Errors", "Testing", "Benchmarks"); renaming a section renames its `@see` lines in the same commit, and the doc-check fails otherwise. Every public symbol carries a Doc Comment for MrDocs's strict reference (a brief, `@param`, `@tparam`, `@return`, one per enumerator and data member); `doc/mrdocs.yml` sets `show-enum-constants: true` so that each enumerator of `errc` and `kind`, whose values are fixed, has its own section.

#### Scenario: Renaming a section
- **WHEN** a section is renamed and an `@see` line still names the old title
- **THEN** the documentation check fails

### Requirement: Docs: Examples are shown by tag and checked by output
The guide's programs SHALL be `example/document_<topic>.cpp`, each with its `.expected`, whose lines are what Node prints for the same JavaScript wherever the page does not say otherwise; the page shows a program's regions by tag (`tag=...`) and its outputs by `lines=`, so a change to a program's output checks the `lines=` ranges of the sections that show it; the README copies `example/document_quick_start.cpp[tag=program]` and its output, and the doc-check compares the copy at every doc build. The counts the page states (`{n-tests-native}`, `{n-headers}`, `{n-examples}` and the per-target ones) come from the data and not from the text.

#### Scenario: The README program
- **WHEN** the quick start runs
- **THEN** it prints `Ada was born in 1815` and the document written back with indent 2: `{`, `  "name": "Ada",`, `  "born": 1815,`, `  "langs": [`, `    "Analytical",`, `    "Note G"`, `  ],`, `  "known": true`, `}`

### Requirement: Docs: The changelog
`doc/changelog.adoc` SHALL list under "Unreleased" (the tags `unreleased` for the release notes) at most ten lines, each `* ` then Added, Changed, Fixed, Improved, Removed, Deprecated or Updated, one sentence, no cross-reference: the library; traits that choose the stored types and JavaScript's key order; handles, array methods and JavaScript-style sugar that throws, compiled only with exceptions; object and array views that the standard algorithms read, with key-ordered queries; stringify, parse, and selectors that stream in bounded memory; streams of documents (NDJSON, JSON text sequences, concatenated JSON, Asio, Cobalt and xactor); one relocatable binary image per document, with reserve, shrink_to_fit and compact; sealed images and Asio readers and writers that open them in place in pools of blocks; shared documents, reference counted over shared memory, with copy on write; a JavaScript module.

#### Scenario: Ten lines
- **WHEN** the unreleased section is counted
- **THEN** it has ten lines

### Requirement: Examples: The programs and where they build
`example/Jamfile` SHALL build every program of the page for `native emscripten wasip2 wasip3`, run it and compare its output with the `.expected` beside it, a program that returns anything but 0 failing too: `document_quick_start`, `document_javascript` (the sugar beside the core: `<exception-handling>on`), `document_numbers`, `document_arrays`, `document_streaming`, `document_selectors`, `document_traits`, `document_image`, `document_no_exceptions` (`<exception-handling>off` whatever the build asks), `document_stream_sync`, `document_image_errors`, `document_containers`, `document_vs_boost_json`, and, native only, `document_stream_asio` (exceptions on), `document_stream_cobalt` (exceptions on, `/webcpp//boost_cobalt`), `document_stream_beast` (exceptions on), `document_image_asio` (exceptions on), `document_image_interprocess` (exceptions on) and `document_threads`, and `document_stream_xactor` (native, wasip2 and wasip3, `/webcpp/xactor//xactor`). Every other example builds both with and without exceptions. A program whose output is a function of an image prints a digest, which is the same on every target (`document_image.expected` begins `4096 bytes, sha-256 ed85084394e7bf7f...`). The sources are UTF-8 (`<toolset>msvc:<cxxflags>/utf-8`).

#### Scenario: The comparison with Boost.JSON
- **WHEN** `example/document_vs_boost_json.cpp` runs
- **THEN** it prints seven parts, e.g. `document:   {"1":4,"2":2,"b":1,"a":3}` against `Boost.JSON: {"b":1,"2":2,"a":3,"1":4}`, `document:   {"b":2,"c":3,"d":4}` against `Boost.JSON: {"d":4,"b":2,"c":3}`, `document:   [0.1000000000000000000000000000000001,1.2345678901234567890123e+22]` against `Boost.JSON: [1E-1,1.2345678901234568E22]`, and `parse("[1,") -> malformed JSON text` against `incomplete JSON`

#### Scenario: What an image refuses, listed
- **WHEN** `example/document_image_errors.cpp` runs
- **THEN** its thirteen lines name `js_traits: incompatible_document`, `format version 2: incompatible_document`, `garbage`, `zeroed memory`, `misaligned buffer`, `truncated, unsealed`, `size` and `container offset` as `not_a_document`, `truncated, sealed` and `one byte changed` as `damaged_document`, `verify::none, accepted: {"user":"ada","langs":["Analytical","Note G"]}`, `reused memory: damaged_document` and `opened, sealed true: {"user":"Ada","langs":["Analytical","Note G"]}`

### Requirement: Bench: The benchmarks
`bench/` SHALL time seventeen operations three ways, with Boost.JSON, with document and with JavaScript's own objects in V8, the C++ natively and compiled to WebAssembly with Emscripten in the same Node.js and the same headless Chrome as the JavaScript: the fourteen of the survey of JSON and dictionary benchmarks (parse, stringify compact, prettify with indent 4, full traversal, key-path queries over `twitter.json`, lookup hit and miss, insert, update, delete then look every key up, iterate entries, array push and pop, array splice in the middle, array methods, deep copy) and three of binary transmission that no suite times (send and receive a document, send and receive a sealed document, pass a document to a Worker that answers each pass), over nativejson-benchmark's `canada.json`, `citm_catalog.json` and `twitter.json` (a submodule pinned at `478d572`, read where the checkout holds them, never copied) and synthetic objects of 10, 1,000 and 100,000 members with keys of 15 or 50 characters from Mulberry32 with a fixed seed. Every implementation must give the same answer to each operation before it is timed (a run whose implementations disagree writes no results); loading is never timed; the first pass is timed alone; the result is the median of 10 samples of at least 10 ms; JavaScript runs with `--expose-gc` and `gc()` before each sample. `b2 libs/document/bench//results` (explicit, always run again, about 9 minutes) writes `doc/benchmark-results.adoc` (generated: do not edit) and `bench//smoke` runs one verifying pass on any target. The sizes on the wire (raw, gzip level 6, brotli quality 11) of `JSON.stringify`'s text, Boost.JSON's text and document's image after `shrink_to_fit` are measured apart, not timed.

#### Scenario: What was measured
- **WHEN** the results of 2026-10-12 (Apple M5 Max, macOS 27.0, Apple clang 21, Boost 1.92, Emscripten 6.0.11, Node v26.7.0, Chrome 147) are read
- **THEN** document parses `canada.json` in 209 ms against Boost.JSON's 2.19 ms and Node's 3.58 ms, an image of `canada.json` is 32,099,376 bytes against 2,090,234 of text (gzip 2,432,828 against 551,870), and a deep copy is 441 us against Boost.JSON's 1.12 ms and `structuredClone`'s 5.43 ms

### Requirement: Bench: The smoke pass runs in the tests
`test/Jamfile` SHALL run `bench/bench.cpp` with `WEBCPP_BENCH_SMOKE` as the test `bench` on every target: one pass of every operation with Boost.JSON and document over nativejson-benchmark's files (without the objects of 100,000 members, which run the same code as those of 1,000), failing when the two answer differently.

#### Scenario: A disagreement
- **WHEN** Boost.JSON and document answer an operation differently
- **THEN** the `bench` test fails

### Requirement: Repo: Files of a webcpp library
The repository SHALL hold `build.jam`, `LICENSE_1_0.txt` (the Boost Software License, Version 1.0), `README.md` (the pitch, the minimal example with its output, building and testing through the superproject, documentation, license), `AGENTS.md` (what is specific to document: webcpp's own, Node as its oracle, the core never throws, the JavaScript API, one image, the cautions and the page), `.gitattributes` (`* text=auto eol=lf`, `*.bat text eol=crlf`, `*.input -text`), `.gitignore` (`doc/html/`, `node_modules`, `.node-modules/`), `meta/libraries.json` (`"key": "document"`, `"name": "document"`, `"authors": ["R"]`, `"description": "A JSON document for C++20 that behaves as JavaScript does, in one relocatable image."`, `"category": ["Data structures", "Parsing"]`, `"maintainers": ["R <github.overboard312@passmail.net>"]`, `"cxxstd": "20"`, `"port-of": null`), `.github/workflows/ci.yml` (the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: document`, on push to main, pull requests and dispatch, with `contents: read` and no cancellation of a run on main), the directories `include/`, `test/`, `example/`, `doc/`, `bench/` and `js/`. Every new source file (`.hpp .cpp .mjs .py .jam .yml .adoc`, `Jamfile`, `build.jam`) opens with the licence notice, "Copyright (c) 2026 WebCpp.org" and the Boost Software License, with `//` for C++, JavaScript and AsciiDoc and `#` for Jam, Python and YAML. Commits are authored as `R`, with no trailer of any kind.

#### Scenario: The maintainer
- **WHEN** `meta/libraries.json` is read
- **THEN** its only maintainer is `R <github.overboard312@passmail.net>`, the address the superproject's AGENTS.md gives

### Requirement: Repo: The npm workflow
`.github/workflows/npm.yml` SHALL build and publish the package on a tag `v<version>` (the version of `js/package.json`) or build and test it alone by hand (`workflow_dispatch`, which never publishes), with `permissions: contents: read` and no cancellation of a publication: the job `package` checks out the superproject at main with this commit in place of its submodule, installs Boost, emsdk and Node, registers the Emscripten toolset, builds from scratch (`b2 -a toolset=emscripten libs/document/test/js//npm-package-test`), checks that the tag names the version, and uploads the staged directory (`npm-package`) and the tarball (`npm-tarball`); the job `consumers` installs the tarball into a fresh directory on Node 22, 24 and 26 (`fail-fast: false`) and runs the smoke test and `tsc` there with TypeScript from `js/package-lock.json`; the job `publish`, on a tag only, after both, is the one job with `id-token: write` and holds no token secret: npm trusted publishing (OIDC) with npm 11.5.1 or newer (checked) runs `npm publish ./package --provenance --access public`. Actions are pinned by commit. The very first publication is made by the owner by hand, since the package must exist before a trusted publisher can be added to it.

#### Scenario: A tag that does not name the version
- **WHEN** the tag is `v0.1.1` and the package's version is 0.1.0
- **THEN** the step "Check the tag names the version" fails and nothing is published
