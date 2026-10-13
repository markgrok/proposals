// The core's workload exported to JavaScript: the segment lives in the wasm memory and the worker
// hands ranges of it to OPFS or IndexedDB. Pointers and sizes are 32-bit (wasm32); generations fit.
#include "../core/segment_core.hpp"

#include <emscripten/emscripten.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

using u32 = std::uint32_t;
constexpr u32 page = 4096;

extern "C" {

/** Page-aligned, rounded up to whole pages (aligned_alloc returns null for any other size). */
EMSCRIPTEN_KEEPALIVE void* seg_alloc(u32 size) {
    void* p = std::aligned_alloc(page, (size + page - 1) / page * page);
    if (p == nullptr) {
        std::abort();
    }
    return p;
}

EMSCRIPTEN_KEEPALIVE void seg_free(void* p) { std::free(p); }

EMSCRIPTEN_KEEPALIVE u32 seg_create(std::byte* body, u32 size) {
    return static_cast<u32>(segment::create(body, size));
}

EMSCRIPTEN_KEEPALIVE void seg_turn(std::byte* body, u32 size, u32 offset, u32 g) {
    segment::turn(body, size, offset, g);
}

/** 1 when body is exactly generation g, 0 when not, -1 when checking it threw. */
EMSCRIPTEN_KEEPALIVE int seg_equals_replay(std::byte* body, u32 size, u32 offset, u32 g, std::byte* scratch) {
    try {
        return segment::equals_replay(body, size, offset, g, scratch) ? 1 : 0;
    } catch (...) {
        return -1;
    }
}

/** Writes the 64-bit digest of [bytes, bytes + size) at out, little-endian. */
EMSCRIPTEN_KEEPALIVE void seg_digest(const std::byte* bytes, u32 size, std::uint64_t* out) {
    *out = segment::digest(bytes, size);
}

/** Lists the pages of cur that differ from shadow into out and copies them into shadow. */
EMSCRIPTEN_KEEPALIVE u32 seg_dirty(const std::byte* cur, std::byte* shadow, u32 size, u32* out) {
    u32 n = 0;
    for (u32 p = 0; p < size / page; ++p) {
        if (std::memcmp(cur + p * page, shadow + p * page, page) != 0) {
            std::memcpy(shadow + p * page, cur + p * page, page);
            out[n++] = p;
        }
    }
    return n;
}

/**
 * Writes the log records of the pages of cur that differ from shadow into out, each a tag 'PAGE',
 * the page number and the page's bytes (4104 bytes), copies them into shadow and returns how many.
 */
EMSCRIPTEN_KEEPALIVE u32 seg_log_records(const std::byte* cur, std::byte* shadow, u32 size, std::byte* out) {
    u32 n = 0;
    for (u32 p = 0; p < size / page; ++p) {
        if (std::memcmp(cur + p * page, shadow + p * page, page) != 0) {
            std::memcpy(shadow + p * page, cur + p * page, page);
            std::byte* r = out + std::size_t{n} * (8 + page);
            const u32 tag = 0x45474150;  // "PAGE"
            std::memcpy(r, &tag, 4);
            std::memcpy(r + 4, &p, 4);
            std::memcpy(r + 8, cur + p * page, page);
            ++n;
        }
    }
    return n;
}
}
