// The workload every track of the segment-persistence investigation runs: a Boost.Interprocess
// managed_external_buffer over a caller's buffer, holding a multi_index of nodes, changed one turn
// at a time by a deterministic function of the turn's number. A state after g turns can be checked
// against a replay of g turns in a fresh buffer, so a reopened file is either generation g exactly
// or wrong. Requires exceptions (on WebAssembly: its exceptions).

#ifndef SEGMENT_CORE_HPP
#define SEGMENT_CORE_HPP

#include <boost/container/string.hpp>
#include <boost/decimal.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <tuple>

namespace segment {

namespace bi = boost::interprocess;
namespace mi = boost::multi_index;
using std::uint64_t;

using void_ptr = bi::offset_ptr<void, std::int64_t, uint64_t, 8>;
using heap = bi::basic_managed_external_buffer<char, bi::rbtree_best_fit<bi::null_mutex_family, void_ptr, 16>,
                                               bi::null_index>;
template <class T> using alloc = bi::allocator<T, heap::segment_manager>;
using text = boost::container::basic_string<char, std::char_traits<char>, alloc<char>>;

struct node {
    uint64_t parent;
    uint64_t index;
    text key;
    boost::decimal::decimal128_t number;
    node(uint64_t p, uint64_t i, std::string_view k, boost::decimal::decimal128_t d, const alloc<char>& a)
        : parent(p), index(i), key(k.data(), k.size(), a), number(d) {}
};

using nodes = mi::multi_index_container<
    node,
    mi::indexed_by<mi::ordered_unique<mi::composite_key<node, mi::member<node, uint64_t, &node::parent>,
                                                        mi::member<node, uint64_t, &node::index>>>,
                   mi::ordered_non_unique<mi::composite_key<node, mi::member<node, uint64_t, &node::parent>,
                                                            mi::member<node, text, &node::key>>,
                                          mi::composite_key_compare<std::less<>, std::less<>>>>,
    alloc<node>>;

// The first bytes of a heap's buffer hold the segment manager; the container is the first object
// allocated in it, so its offset is the same in every buffer made by create().
inline nodes& container(std::byte* body, uint64_t offset) { return *reinterpret_cast<nodes*>(body + offset); }

// Builds an empty segment in body (zeroed, 16-aligned) and returns the container's offset.
inline uint64_t create(std::byte* body, uint64_t size) {
    std::memset(body, 0, size);
    heap h(bi::create_only, body, size);
    void* raw = h.allocate_aligned(sizeof(nodes), alignof(nodes));
    new (raw) nodes(alloc<node>(h.get_segment_manager()));
    return static_cast<uint64_t>(static_cast<std::byte*>(raw) - body);
}

inline void key_of(uint64_t i, char (&k)[24]) { std::snprintf(k, sizeof k, "key-%08llu", (unsigned long long)i); }

// Turn g (1, 2, ...): inserts node g with a 40-byte key, changes node g/2's number, and erases node
// g-200 when it exists, so the live set stays near 200 nodes and the heap reuses freed blocks.
inline void turn(std::byte* body, uint64_t size, uint64_t offset, uint64_t g) {
    heap h(bi::open_only, body, size);
    auto& n = container(body, offset);
    char k[24];
    key_of(g, k);
    std::string pad(std::string_view(k).size() < 40 ? 40 - std::string_view(k).size() : 0, 'x');
    std::string key = std::string(k) + pad;
    n.emplace(uint64_t{0}, g, std::string_view(key), boost::decimal::decimal128_t{g, -2},
              alloc<char>(h.get_segment_manager()));
    auto half = n.find(std::make_tuple(uint64_t{0}, g / 2));
    if (half != n.end()) {
        n.modify(half, [g](node& x) { x.number = boost::decimal::decimal128_t{g * 3, -1}; });
    }
    if (g > 200) {
        auto old = n.find(std::make_tuple(uint64_t{0}, g - 200));
        if (old != n.end()) {
            n.erase(old);
        }
    }
}

inline uint64_t digest(const std::byte* bytes, uint64_t size) {
    uint64_t h = 1469598103934665603ULL;
    for (uint64_t i = 0; i < size; ++i) {
        h ^= static_cast<uint64_t>(bytes[i]);
        h *= 1099511628211ULL;
    }
    return h;
}

// Whether body holds exactly the logical state after g turns: the same nodes, keys and numbers as a
// replay, and a heap that passes check_sanity.
inline bool equals_replay(std::byte* body, uint64_t size, uint64_t offset, uint64_t g, std::byte* scratch) {
    uint64_t at = create(scratch, size);
    for (uint64_t t = 1; t <= g; ++t) {
        turn(scratch, size, at, t);
    }
    heap h(bi::open_only, body, size);
    if (!h.check_sanity()) {
        return false;
    }
    auto& a = container(body, offset);
    auto& b = container(scratch, at);
    if (a.size() != b.size()) {
        return false;
    }
    for (auto i = a.begin(), j = b.begin(); i != a.end(); ++i, ++j) {
        if (i->parent != j->parent || i->index != j->index || i->key != j->key || i->number != j->number) {
            return false;
        }
    }
    return true;
}

}  // namespace segment

#endif
