// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/json/deep_copy.hpp>
#include <webcpp/json_schema/detail/json/equal.hpp>
#include <webcpp/json_schema/detail/json/value_arena.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace js = webcpp::json_schema;
using alloc = std::pmr::polymorphic_allocator<std::byte>;

namespace {

void a_copy_is_equal_and_lives_in_the_arena() {
    webcpp::test::counting_resource counted;
    std::pmr::monotonic_buffer_resource scratch;
    const auto source =
        boost::json::parse(R"({"a": [1, 2.5, "text long enough to allocate"], "b": {"c": null}})");
    {
        const auto arena =
            std::allocate_shared<js::detail::value_arena<alloc>>(alloc(&counted), alloc(&counted));
        const boost::json::value copy = js::detail::deep_copy(source, arena->storage(), scratch);
        BOOST_TEST(js::detail::equal(copy, source, scratch));
        BOOST_TEST(copy.storage().get() == arena->storage().get());
        BOOST_TEST(counted.allocations > 0);
    }
    BOOST_TEST_EQ(counted.live, 0U);
}

void a_deep_value_copies_and_dies_without_recursion() {
    boost::json::monotonic_resource source_storage;
    boost::json::value deep = boost::json::object(&source_storage);
    for (int i = 0; i < 100000; ++i) {
        boost::json::object outer(&source_storage);
        outer["items"] = std::move(deep);
        deep = std::move(outer);
    }
    std::pmr::monotonic_buffer_resource scratch;
    const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(alloc(), alloc());
    const boost::json::value copy = js::detail::deep_copy(deep, arena->storage(), scratch);
    BOOST_TEST(js::detail::equal(copy, deep, scratch));
}  // copy and arena die here: trivially deallocating storage, no recursive destructor

// The arena's storage tells Boost.JSON that its deallocation is trivial, and that it owns nothing.
void the_storage_is_trivially_deallocating() {
    const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(alloc(), alloc());
    BOOST_TEST(arena->storage().is_deallocate_trivial());
    BOOST_TEST(!arena->storage().is_shared());
    BOOST_TEST(&arena->resource() == std::pmr::get_default_resource());
}

// Every kind is copied as it is: an int64, a uint64 and a double stay what they were, which
// equality alone would not see (it reads 1 and 1.0 as one number), and so do member order, empty
// containers, NUL bytes and an infinity.
void every_kind_is_copied_as_it_is() {
    std::pmr::monotonic_buffer_resource scratch;
    const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(alloc(), alloc());
    boost::json::array source{
        nullptr,
        true,
        false,
        std::int64_t{1},
        std::uint64_t{1},
        1.0,
        -0.0,
        std::numeric_limits<std::int64_t>::min(),
        std::numeric_limits<std::uint64_t>::max(),
        std::numeric_limits<double>::infinity(),
        "",
        std::string_view("a\0b", 3),
        std::string(1000, 'x'),
        boost::json::array{},
        boost::json::object{},
    };
    source.push_back(boost::json::parse(R"({"z": 1, "a": 2, "m": {"y": [], "b": {}}})"));
    boost::json::object nul;
    nul[std::string_view("k\0", 2)] = 1;
    nul["k"] = 2;
    source.push_back(std::move(nul));
    const boost::json::value original = source;
    const boost::json::value copy = js::detail::deep_copy(original, arena->storage(), scratch);
    BOOST_TEST_EQ(boost::json::serialize(copy), boost::json::serialize(original));
    const auto& copied = copy.as_array();
    if (!BOOST_TEST_EQ(copied.size(), source.size())) {
        return;
    }
    for (std::size_t i = 0; i < source.size(); ++i) {
        BOOST_TEST(copied[i].kind() == source[i].kind());
    }
    BOOST_TEST(std::signbit(copied[6].as_double()));
    BOOST_TEST_EQ(copied[11].as_string().size(), 3U);
    const boost::json::object& last = copied.back().as_object();
    BOOST_TEST_EQ(last.size(), 2U);
    BOOST_TEST(last.contains(std::string_view("k\0", 2)));
    for (const auto& element : copied) {
        BOOST_TEST(element.storage().get() == arena->storage().get());
    }
    const auto root = boost::json::parse(R"("a root that is a string, long enough to allocate")");
    const boost::json::value text = js::detail::deep_copy(root, arena->storage(), scratch);
    BOOST_TEST_EQ(text, root);
    BOOST_TEST(text.storage().get() == arena->storage().get());
    BOOST_TEST_EQ(js::detail::deep_copy(boost::json::value(7), arena->storage(), scratch), 7);
}

// The copy builds its values in the arena and keeps its own stacks in the scratch resource, which
// it gives back whole; a wide value spills its builder's stack there.
void temporaries_stay_in_the_scratch_resource() {
    boost::json::array wide;
    for (int i = 0; i < 10000; ++i) {
        wide.push_back(i);
    }
    const boost::json::value source = std::move(wide);
    webcpp::test::recording_resource scratch;
    const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(alloc(), alloc());
    const boost::json::value copy = js::detail::deep_copy(source, arena->storage(), scratch);
    BOOST_TEST(scratch.allocations > 0);
    BOOST_TEST_EQ(scratch.live_blocks(), 0U);
    BOOST_TEST_EQ(scratch.mismatched, 0U);
    BOOST_TEST_EQ(scratch.misaligned, 0U);
    BOOST_TEST(js::detail::equal(copy, source, scratch));
}

// Every block the arena takes from its allocator is aligned as asked and goes back with the size
// and alignment it was taken with, through a pmr allocator and through an adapted one.
void the_arena_gives_back_what_it_takes() {
    const auto source = boost::json::parse(
        R"({"a": [1, 2.5, "text long enough to allocate"], "b": {"c": null}, "d": [[[[]]]]})");
    {
        webcpp::test::recording_resource recorded;
        {
            std::pmr::monotonic_buffer_resource scratch;
            const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(
                alloc(&recorded), alloc(&recorded));
            for (int i = 0; i < 1000; ++i) {
                const auto copy = js::detail::deep_copy(source, arena->storage(), scratch);
                BOOST_TEST(js::detail::equal(copy, source, scratch));
            }
            BOOST_TEST(recorded.allocations > 2);
        }
        BOOST_TEST_EQ(recorded.live_blocks(), 0U);
        BOOST_TEST_EQ(recorded.mismatched, 0U);
        BOOST_TEST_EQ(recorded.misaligned, 0U);
    }
    {
        using adapted = webcpp::test::recording_allocator<std::byte>;
        webcpp::test::recording_resource recorded;
        {
            std::pmr::monotonic_buffer_resource scratch;
            const auto arena = std::allocate_shared<js::detail::value_arena<adapted>>(
                adapted(&recorded), adapted(&recorded));
            const auto copy = js::detail::deep_copy(source, arena->storage(), scratch);
            BOOST_TEST(js::detail::equal(copy, source, scratch));
            BOOST_TEST(recorded.allocations > 1);
        }
        BOOST_TEST_EQ(recorded.live_blocks(), 0U);
        BOOST_TEST_EQ(recorded.mismatched, 0U);
        BOOST_TEST_EQ(recorded.misaligned, 0U);
    }
}

// The arena's storage serves an alignment past the fundamental one, which Boost.JSON's own
// containers never ask for, but a request through the storage may: first a block as large as its
// alignment, on a fresh arena, which Boost.JSON 1.92's monotonic_resource answers with nullptr.
void the_arena_serves_every_alignment() {
    for (const std::size_t alignment :
         {std::size_t{1}, std::size_t{8}, std::size_t{16}, std::size_t{64}, std::size_t{4096}}) {
        webcpp::test::recording_resource recorded;
        {
            const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(
                alloc(&recorded), alloc(&recorded));
            for (const std::size_t size :
                 {alignment, std::size_t{1}, std::size_t{100}, std::size_t{5000}}) {
                void* p = arena->storage()->allocate(size, alignment);
                BOOST_TEST(p != nullptr);
                BOOST_TEST_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0U);
                arena->storage()->deallocate(p, size, alignment);
            }
        }
        BOOST_TEST_EQ(recorded.live_blocks(), 0U);
        BOOST_TEST_EQ(recorded.mismatched, 0U);
        BOOST_TEST_EQ(recorded.misaligned, 0U);
    }
}

// A chain of one-member objects, `depth` levels deep, and one of one-element arrays.
boost::json::value object_chain(boost::json::monotonic_resource& storage, std::size_t depth) {
    boost::json::value deep = boost::json::object(&storage);
    for (std::size_t i = 0; i < depth; ++i) {
        boost::json::object outer(&storage);
        outer["items"] = std::move(deep);
        deep = std::move(outer);
    }
    return deep;
}

boost::json::value array_chain(boost::json::monotonic_resource& storage, std::size_t depth) {
    boost::json::value deep = boost::json::array(&storage);
    for (std::size_t i = 0; i < depth; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(deep));
        deep = std::move(outer);
    }
    return deep;
}

// Nesting of 10^5 and 10^6 levels copies, compares and dies without recursion, its storage given
// back whole.
void every_depth_copies_and_dies_without_recursion() {
    for (const std::size_t depth : {std::size_t{100000}, std::size_t{1000000}}) {
        webcpp::test::recording_resource recorded;
        {
            boost::json::monotonic_resource source_storage;
            std::pmr::monotonic_buffer_resource scratch;
            const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(
                alloc(&recorded), alloc(&recorded));
            const boost::json::value objects = object_chain(source_storage, depth);
            const boost::json::value copy =
                js::detail::deep_copy(objects, arena->storage(), scratch);
            BOOST_TEST(js::detail::equal(copy, objects, scratch));
            const boost::json::value arrays = array_chain(source_storage, depth);
            const boost::json::value other =
                js::detail::deep_copy(arrays, arena->storage(), scratch);
            BOOST_TEST(js::detail::equal(other, arrays, scratch));
            BOOST_TEST(!js::detail::equal(other, copy, scratch));
        }
        BOOST_TEST_EQ(recorded.live_blocks(), 0U);
        BOOST_TEST_EQ(recorded.mismatched, 0U);
        BOOST_TEST_EQ(recorded.misaligned, 0U);
    }
}

}  // namespace

int main() {
    a_copy_is_equal_and_lives_in_the_arena();
    a_deep_value_copies_and_dies_without_recursion();
    the_storage_is_trivially_deallocating();
    every_kind_is_copied_as_it_is();
    temporaries_stay_in_the_scratch_resource();
    the_arena_gives_back_what_it_takes();
    the_arena_serves_every_alignment();
    every_depth_copies_and_dies_without_recursion();
    return boost::report_errors();
}
