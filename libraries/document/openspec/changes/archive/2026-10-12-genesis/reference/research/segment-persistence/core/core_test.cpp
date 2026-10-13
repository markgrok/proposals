#include "segment_core.hpp"
#include <memory>
#include <new>
int main() {
    constexpr std::uint64_t size = 1u << 20;
    auto* a = static_cast<std::byte*>(::operator new(size, std::align_val_t{16}));
    auto* b = static_cast<std::byte*>(::operator new(size, std::align_val_t{16}));
    auto* s = static_cast<std::byte*>(::operator new(size, std::align_val_t{16}));
    auto at = segment::create(a, size);
    for (std::uint64_t g = 1; g <= 1000; ++g) segment::turn(a, size, at, g);
    std::memcpy(b, a, size);
    bool same = segment::equals_replay(b, size, at, 1000, s);
    bool other = segment::equals_replay(b, size, at, 999, s);
    std::printf("offset %llu, replay 1000 %d, replay 999 %d, digest %016llx, replay digest %016llx\n",
        (unsigned long long)at, same, other, (unsigned long long)segment::digest(b, size),
        (unsigned long long)segment::digest(s, size));
    return same && !other ? 0 : 1;
}
