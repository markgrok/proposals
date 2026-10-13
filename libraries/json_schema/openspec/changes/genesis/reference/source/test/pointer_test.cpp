// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/json/pointer.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>

namespace js = webcpp::json_schema;

namespace {

// The value `pointer` names in `root`, through a scratch of its own.
const boost::json::value* resolve(const boost::json::value& root, std::string_view pointer) {
    std::string scratch;
    return js::detail::resolve_pointer(root, pointer, scratch);
}

std::string tokens(std::initializer_list<std::string_view> names) {
    std::pmr::string out;
    for (const auto name : names) {
        js::detail::append_token(out, name);
    }
    return std::string(out);
}

void tokens_are_escaped() {
    BOOST_TEST_EQ(tokens({"a"}), "/a");
    BOOST_TEST_EQ(tokens({""}), "/");
    BOOST_TEST_EQ(tokens({"a/b"}), "/a~1b");
    BOOST_TEST_EQ(tokens({"~1"}), "/~01");
    BOOST_TEST_EQ(tokens({"~", "/"}), "/~0/~1");
    BOOST_TEST_EQ(tokens({std::string_view("a\0b", 3)}), std::string("/a\0b", 4));
    BOOST_TEST_EQ(tokens({"\xF0\x9F\x90\xB2"}), "/\xF0\x9F\x90\xB2");
    std::pmr::string indexed;
    js::detail::append_index(indexed, 0);
    js::detail::append_index(indexed, 18446744073709551615ULL);
    BOOST_TEST_EQ(std::string(indexed), "/0/18446744073709551615");
    std::pmr::string tens;
    js::detail::append_index(tens, 9);
    js::detail::append_index(tens, 10);
    BOOST_TEST_EQ(std::string(tens), "/9/10");
}

void pointers_resolve() {
    // RFC 6901 section 5
    const auto doc = boost::json::parse(R"({
        "foo": ["bar", "baz"],
        "": 0,
        "a/b": 1,
        "c%d": 2,
        "e^f": 3,
        "g|h": 4,
        "i\\j": 5,
        "k\"l": 6,
        " ": 7,
        "m~n": 8
    })");
    BOOST_TEST(resolve(doc, "") == &doc);
    BOOST_TEST_EQ(*resolve(doc, "/foo/0"), "bar");
    BOOST_TEST_EQ(*resolve(doc, "/"), 0);
    BOOST_TEST_EQ(*resolve(doc, "/a~1b"), 1);
    BOOST_TEST_EQ(*resolve(doc, "/m~0n"), 8);
    BOOST_TEST(resolve(doc, "/foo/2") == nullptr);
    BOOST_TEST(resolve(doc, "/foo/01") == nullptr);
    BOOST_TEST(resolve(doc, "/foo/-") == nullptr);
    BOOST_TEST(!js::detail::is_valid_pointer("foo"));
    BOOST_TEST(!js::detail::is_valid_pointer("/~2"));
    BOOST_TEST(!js::detail::is_valid_pointer("/~"));
    BOOST_TEST(js::detail::is_valid_pointer("/~0~1"));
}

// Whether `pointer` names a value in `doc`, and that value is `expected`.
bool resolves_to(const boost::json::value& doc, std::string_view pointer,
                 const boost::json::value& expected) {
    const auto* found = resolve(doc, pointer);
    return found != nullptr && *found == expected;
}

// The rest of RFC 6901 section 5, and what a pointer that is not one, a token that is not an index,
// or a step into a scalar resolves to.
void the_other_steps() {
    const auto doc = boost::json::parse(R"({
        "foo": ["bar", "baz"],
        "": "empty",
        "c%d": 2,
        "e^f": 3,
        "g|h": 4,
        "i\\j": 5,
        "k\"l": 6,
        " ": 7,
        "s": "text"
    })");
    BOOST_TEST(resolve(doc, "/foo") == doc.as_object().if_contains("foo"));
    BOOST_TEST(resolves_to(doc, "/foo/1", "baz"));
    BOOST_TEST(resolves_to(doc, "/", "empty"));
    BOOST_TEST(resolves_to(doc, "/c%d", 2));
    BOOST_TEST(resolves_to(doc, "/e^f", 3));
    BOOST_TEST(resolves_to(doc, "/g|h", 4));
    BOOST_TEST(resolves_to(doc, "/i\\j", 5));
    BOOST_TEST(resolves_to(doc, "/k\"l", 6));
    BOOST_TEST(resolves_to(doc, "/ ", 7));
    for (const std::string_view absent : {
             "/foo/",
             "/foo/+1",
             "/foo/ 1",
             "/foo/1a",
             "/foo/-1",
             "/foo/0x1",
             "/foo/1/0",
             "/foo/4294967295",
             "/foo/4294967296",
             "/foo/4294967297",
             "/foo/18446744073709551615",
             "/foo/18446744073709551616",
             "/foo/18446744073709551617",
             "/foo/99999999999999999999999",
             "/s/0",
             "/s/x",
             "/x",
             "/foo/0/x",
             "/c%d/0",
             "foo",
             "x",
             "xfoo",
             "~1",
             "/~",
             "/foo/~2",
         }) {
        BOOST_TEST(resolve(doc, absent) == nullptr);
    }
    const auto scalar = boost::json::parse("17");
    BOOST_TEST(resolve(scalar, "") == &scalar);
    BOOST_TEST(resolve(scalar, "/0") == nullptr);
}

// A token compares with a key as the key it spells, and a token that is not well formed spells no
// key.
void tokens_compare_with_keys() {
    BOOST_TEST(js::detail::token_equals("", ""));
    BOOST_TEST(js::detail::token_equals("a~1b", "a/b"));
    BOOST_TEST(js::detail::token_equals("a~0b", "a~b"));
    BOOST_TEST(js::detail::token_equals("~01", "~1"));
    BOOST_TEST(js::detail::token_equals("~10", "/0"));
    BOOST_TEST(js::detail::token_equals(std::string_view("a\0b", 3), std::string_view("a\0b", 3)));
    BOOST_TEST(!js::detail::token_equals("a", "ab"));
    BOOST_TEST(!js::detail::token_equals("ab", "a"));
    BOOST_TEST(!js::detail::token_equals("a~1b", "a~1b"));
    BOOST_TEST(!js::detail::token_equals("", "a"));
    BOOST_TEST(!js::detail::token_equals("a~", "a~"));
    BOOST_TEST(!js::detail::token_equals("a~", "a/"));
    BOOST_TEST(!js::detail::token_equals("a~", "a"));
    BOOST_TEST(!js::detail::token_equals("~2", "~2"));
    BOOST_TEST(!js::detail::token_equals("~2", "/"));
    BOOST_TEST(!js::detail::token_equals("~2", "~"));
}

// Every name a property may have, written as a token, is a valid pointer that resolves to that
// property: names that are not plain identifiers, and one in an object large enough that
// Boost.JSON indexes its keys.
void every_name_resolves() {
    constexpr std::array<std::string_view, 16> names{
        "",
        "a",
        "a/b",
        "~1",
        "~0",
        "~",
        "/",
        "//~~",
        "m~n",
        " ",
        "\xF0\x9F\x90\xB2",
        "a~1b",
        "0",
        "-",
        "~01",
        "\xC3\xA9t\xC3\xA9",
    };
    boost::json::object object;
    for (const auto name : names) {
        object[name] = name;
    }
    object[std::string_view("a\0b", 3)] = "nul";
    for (int i = 0; i < 100; ++i) {
        object["k" + std::to_string(i)] = i;
    }
    const boost::json::value doc = object;
    for (const auto name : names) {
        const std::string pointer = tokens({name});
        BOOST_TEST(js::detail::is_valid_pointer(pointer));
        const auto* found = resolve(doc, pointer);
        if (BOOST_TEST(found != nullptr)) {
            BOOST_TEST(found->is_string() && found->get_string() == name);
        }
    }
    const auto* nul = resolve(doc, tokens({std::string_view("a\0b", 3)}));
    if (BOOST_TEST(nul != nullptr)) {
        BOOST_TEST_EQ(*nul, "nul");
    }
    BOOST_TEST(resolve(doc, "/a") != nullptr);
    BOOST_TEST(resolve(doc, std::string_view("/a\0", 3)) == nullptr);
    BOOST_TEST_EQ(*resolve(doc, "/k57"), 57);
    BOOST_TEST(resolve(doc, "/k~157") == nullptr);
}

// A chain of one-element arrays, `depth` levels above an empty one.
boost::json::value chain(boost::json::monotonic_resource& storage, std::size_t depth) {
    boost::json::value v = boost::json::array(&storage);
    for (std::size_t i = 0; i < depth; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(v));
        v = std::move(outer);
    }
    return v;
}

// A pointer as deep as a value nests resolves without recursion.
void depth_never_reaches_the_stack() {
    for (const std::size_t depth : {std::size_t{100000}, std::size_t{1000000}}) {
        boost::json::monotonic_resource storage;
        const boost::json::value deep = chain(storage, depth);
        std::pmr::string pointer;
        for (std::size_t i = 0; i < depth; ++i) {
            js::detail::append_index(pointer, 0);
        }
        BOOST_TEST(js::detail::is_valid_pointer(pointer));
        const auto* leaf = resolve(deep, pointer);
        if (BOOST_TEST(leaf != nullptr)) {
            BOOST_TEST(leaf->is_array() && leaf->get_array().empty());
        }
        js::detail::append_index(pointer, 0);
        BOOST_TEST(resolve(deep, pointer) == nullptr);
    }
}

std::string fragment(std::string_view pointer) {
    std::pmr::string out = "urn:x#";
    js::detail::append_fragment(out, pointer);
    return std::string(out);
}

// A JSON Pointer in a URI fragment (RFC 6901 section 6): each byte outside RFC 3986's fragment
// characters, and "%", as %XX of its UTF-8, so that decoding gives the pointer back.
void fragments_are_percent_encoded() {
    BOOST_TEST_EQ(fragment(""), "urn:x#");
    BOOST_TEST_EQ(fragment("/properties/a b"), "urn:x#/properties/a%20b");
    BOOST_TEST_EQ(fragment("/properties/100%"), "urn:x#/properties/100%25");
    BOOST_TEST_EQ(fragment("/properties/a#b"), "urn:x#/properties/a%23b");
    BOOST_TEST_EQ(fragment("/properties/\xC3\xA9"), "urn:x#/properties/%C3%A9");
    BOOST_TEST_EQ(fragment("/properties/a\"b"), "urn:x#/properties/a%22b");
    BOOST_TEST_EQ(fragment("/properties/a%41"), "urn:x#/properties/a%2541");
    BOOST_TEST_EQ(fragment(std::string_view("/a\0b", 4)), "urn:x#/a%00b");
    BOOST_TEST_EQ(fragment("/\x7F\xFF"), "urn:x#/%7F%FF");
    BOOST_TEST_EQ(fragment("/<>[]\\^`{|}"), "urn:x#/%3C%3E%5B%5D%5C%5E%60%7B%7C%7D");
    BOOST_TEST_EQ(fragment("/$defs/a~1b"), "urn:x#/$defs/a~1b");
    const std::string_view kept =
        "/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~!$&'()*+,;=:@/?";
    BOOST_TEST_EQ(fragment(kept), "urn:x#" + std::string(kept));
}

// A scratch string that counts the bytes written into it.
class counting_string {
public:
    void clear() noexcept { text_.clear(); }

    void push_back(char c) {
        text_.push_back(c);
        ++written;
    }

    [[nodiscard]] const char* data() const noexcept { return text_.data(); }

    [[nodiscard]] std::size_t size() const noexcept { return text_.size(); }

    std::size_t written = 0;

private:
    std::string text_;
};

// A token holding `~` is unescaped once into the scratch and looked up there, never compared
// with each key of its object: resolving a pointer to each of m such keys writes, in all, the
// keys' bytes, linear in the object however many keys it holds.
void escaped_tokens_are_unescaped_once() {
    boost::json::object object;
    std::size_t bytes = 0;
    for (std::size_t i = 0; i < 2'000; ++i) {
        const std::string key = "~/k" + std::to_string(i);
        object[key] = i;
        bytes += key.size();
    }
    const boost::json::value doc = object;
    counting_string scratch;
    for (std::size_t i = 0; i < 2'000; ++i) {
        const auto* found =
            js::detail::resolve_pointer(doc, tokens({"~/k" + std::to_string(i)}), scratch);
        if (BOOST_TEST(found != nullptr)) {
            BOOST_TEST_EQ(found->as_uint64(), i);
        }
    }
    BOOST_TEST_EQ(scratch.written, bytes);
}

}  // namespace

int main() {
    tokens_are_escaped();
    pointers_resolve();
    the_other_steps();
    tokens_compare_with_keys();
    every_name_resolves();
    depth_never_reaches_the_stack();
    fragments_are_percent_encoded();
    escaped_tokens_are_unescaped_once();
    return boost::report_errors();
}
