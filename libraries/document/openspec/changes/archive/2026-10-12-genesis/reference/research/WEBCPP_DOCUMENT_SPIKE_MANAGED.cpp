#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/container/string.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/decimal.hpp>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <string_view>
namespace bi = boost::interprocess; namespace mi = boost::multi_index; using std::uint64_t;
using void_ptr = bi::offset_ptr<void, std::int64_t, uint64_t, 8>;
using heap = bi::basic_managed_external_buffer<char, bi::rbtree_best_fit<bi::null_mutex_family, void_ptr, 16>, bi::null_index>;
template <class T> using alloc = bi::allocator<T, heap::segment_manager>;
using text = boost::container::basic_string<char, std::char_traits<char>, alloc<char>>;
struct node { uint64_t parent; uint64_t index; text key; boost::decimal::decimal128_t number;
  node(uint64_t p, uint64_t i, std::string_view k, boost::decimal::decimal128_t d, const alloc<char>& a) : parent(p), index(i), key(k.data(), k.size(), a), number(d) {} };
using nodes = mi::multi_index_container<node, mi::indexed_by<
    mi::ordered_unique<mi::composite_key<node, mi::member<node, uint64_t, &node::parent>, mi::member<node, uint64_t, &node::index>>>,
    mi::ordered_non_unique<mi::composite_key<node, mi::member<node, uint64_t, &node::parent>, mi::member<node, text, &node::key>>,
        mi::composite_key_compare<std::less<>, std::less<>>>>, alloc<node>>;
int main() {
    uint64_t size = 16 * 1024;
    auto* a = static_cast<std::byte*>(::operator new(size, std::align_val_t{16}));
    std::memset(a, 0, size);
    heap h(bi::create_only, a, size);
    auto* n = new (h.allocate_aligned(sizeof(nodes), alignof(nodes))) nodes(alloc<node>(h.get_segment_manager()));
    uint64_t at = static_cast<uint64_t>(reinterpret_cast<std::byte*>(n) - a);
    uint64_t i = 0, grown = 0;
    while (i < 1000) {
        char k[16]; std::snprintf(k, sizeof k, "k%04llu", (unsigned long long)i);
        try {
            n->emplace(uint64_t{0}, i, std::string_view(k), boost::decimal::decimal128_t{i, -2}, alloc<char>(h.get_segment_manager()));
            ++i;
        } catch (const bi::bad_alloc&) {
            // Full: a bigger buffer, the image copied, the heap grown in place of the copy.
            uint64_t bigger = size * 2;
            auto* b = static_cast<std::byte*>(::operator new(bigger, std::align_val_t{16}));
            std::memset(b, 0, bigger); std::memcpy(b, a, size);
            ::operator delete(a, std::align_val_t{16});
            a = b; h = heap(bi::open_only, a, size);
            h.grow(bigger - size); size = bigger; ++grown;
            n = reinterpret_cast<nodes*>(a + at);
        }
    }
    auto it = n->get<1>().find(std::make_tuple(uint64_t{0}, std::string_view("k0777")));
    char out[64]; auto r = boost::decimal::to_chars(out, out + 63, it->number); *r.ptr = 0;
    std::printf("nodes %llu, grown %llu times to %llu bytes, k0777 = %s, sane %d\n", (unsigned long long)n->size(),
        (unsigned long long)grown, (unsigned long long)size, out, (int)h.check_sanity());
}
