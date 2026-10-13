#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/null_mutex.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/container/string.hpp>
#include <boost/decimal.hpp>
#include <boost/throw_exception.hpp>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <string_view>
namespace bi = boost::interprocess;
using std::uint64_t;
template <class T> using ptr = bi::offset_ptr<T, std::int64_t, uint64_t, 8>;
// The mutex family rbtree_best_fit asks for, without mutex_family.hpp (which pulls in the spin
// recursive mutex and its bare throws).
struct no_mutexes { using mutex_type = bi::null_mutex; using recursive_mutex_type = bi::null_mutex; };
using heap = bi::rbtree_best_fit<no_mutexes, ptr<void>, 16>;
template <class T> struct alloc {
    using value_type = T; using pointer = ptr<T>; using const_pointer = ptr<const T>;
    using void_pointer = ptr<void>; using size_type = uint64_t; using difference_type = std::int64_t;
    template <class U> struct rebind { using other = alloc<U>; };
    ptr<heap> h;
    explicit alloc(heap* p) : h(p) {}
    template <class U> alloc(const alloc<U>& o) : h(o.h) {}
    pointer allocate(size_type n) {
        void* p = h->allocate_aligned(n * sizeof(T), alignof(T));
        if (p == nullptr) boost::throw_exception(std::bad_alloc());
        std::memset(p, 0, n * sizeof(T));
        return pointer(static_cast<T*>(p));
    }
    void deallocate(pointer p, size_type n) { std::memset(static_cast<void*>(p.get()), 0, n * sizeof(T)); h->deallocate(p.get()); }
    bool operator==(const alloc& o) const { return h == o.h; }
};
namespace mi = boost::multi_index;
using text = boost::container::basic_string<char, std::char_traits<char>, alloc<char>>;
struct node { uint64_t parent; uint64_t index; text key; boost::decimal::decimal128_t number;
  node(uint64_t p, uint64_t i, std::string_view k, boost::decimal::decimal128_t d, const alloc<char>& a) : parent(p), index(i), key(k.data(), k.size(), a), number(d) {} };
using nodes = mi::multi_index_container<node, mi::indexed_by<
    mi::ordered_unique<mi::composite_key<node, mi::member<node, uint64_t, &node::parent>, mi::member<node, uint64_t, &node::index>>>,
    mi::ordered_non_unique<mi::composite_key<node, mi::member<node, uint64_t, &node::parent>, mi::member<node, text, &node::key>>,
        mi::composite_key_compare<std::less<>, std::less<>>>>, alloc<node>>;
int main() {
    constexpr uint64_t size = 64 * 1024;
    auto* a = static_cast<std::byte*>(::operator new(size, std::align_val_t{16}));
    std::memset(a, 0, size);
    auto* h = new (a) heap(size, sizeof(heap));
    void* raw = h->allocate_aligned(sizeof(nodes), alignof(nodes));
    auto* n = new (raw) nodes(alloc<node>(h));
    for (uint64_t i = 0; i < 100; ++i) { char k[16]; std::snprintf(k, sizeof k, "k%03llu", (unsigned long long)i);
        n->emplace(uint64_t{0}, i, std::string_view(k), boost::decimal::decimal128_t{i, -2}, alloc<char>(h)); }
    for (uint64_t i = 0; i < 100; i += 2) n->erase(n->find(std::make_tuple(uint64_t{0}, i)));
    uint64_t at = static_cast<uint64_t>(static_cast<std::byte*>(raw) - a);
    void* too_big = h->allocate(size);
    auto* b = static_cast<std::byte*>(::operator new(size, std::align_val_t{16}));
    std::memcpy(b, a, size); std::memset(a, 0xAB, size);
    auto& m = *reinterpret_cast<nodes*>(b + at);
    auto it = m.get<1>().find(std::make_tuple(uint64_t{0}, std::string_view("k043")));
    if (it == m.get<1>().end()) return 1;
    // The copy keeps allocating: its heap is addressed by offsets too.
    auto* hb = reinterpret_cast<heap*>(b);
    m.emplace(uint64_t{0}, uint64_t{1000}, std::string_view("new"), boost::decimal::decimal128_t{5}, alloc<char>(hb));
    uint64_t fnv = 1469598103934665603ULL; for (uint64_t i = 0; i < size; ++i) { fnv ^= static_cast<uint64_t>(b[i]); fnv *= 1099511628211ULL; }
    { std::FILE* f = std::fopen("image.bin", "wb"); std::fwrite(b, 1, size, f); std::fclose(f); }
    std::printf("image fnv1a %016llx\n", (unsigned long long)fnv);
    char out[64]; auto r = boost::decimal::to_chars(out, out + 63, it->number); *r.ptr = 0;
    std::printf("copy found index %llu number %s, nodes %llu, free %llu, too big -> %s, sizeof(heap) %zu\n",
        (unsigned long long)it->index, out, (unsigned long long)m.size(),
        (unsigned long long)hb->get_free_memory(), too_big ? "allocated" : "null", sizeof(heap));
}
