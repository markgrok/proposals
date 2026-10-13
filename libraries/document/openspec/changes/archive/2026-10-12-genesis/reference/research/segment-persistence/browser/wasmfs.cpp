// The same commits through Emscripten's WasmFS OPFS backend: POSIX pwrite and fsync, which WasmFS
// proxies to a thread holding a FileSystemSyncAccessHandle (fsync becomes its flush()).
// S1: the image into the inactive slot, fsync, the header, fsync. S2: the dirty pages and a commit
// record appended to a log, fsync. 200 commits per size; latency printed as median and p99.
#include "../core/segment_core.hpp"

#include <emscripten/wasmfs.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

using clk = std::chrono::steady_clock;
constexpr std::size_t page = 4096;

static void report(const char* what, unsigned mb, std::vector<double> v, double bytes) {
    std::sort(v.begin(), v.end());
    std::printf("WASMFS {\"strategy\":\"%s\",\"mb\":%u,\"median\":%.3f,\"p99\":%.3f,\"bytesMean\":%.0f}\n", what, mb,
                v[v.size() / 2], v[v.size() * 99 / 100], bytes);
}

int main() {
    backend_t opfs = wasmfs_create_opfs_backend();
    wasmfs_create_directory("/opfs", 0777, opfs);
    for (unsigned mb : {1u, 16u, 64u}) {
        std::size_t size = std::size_t{mb} << 20;
        auto* body = static_cast<std::byte*>(std::aligned_alloc(page, size));
        auto* shadow = static_cast<std::byte*>(std::aligned_alloc(page, size));
        auto at = segment::create(body, size);
        std::memcpy(shadow, body, size);
        int s1 = open("/opfs/wasmfs-s1", O_RDWR | O_CREAT | O_TRUNC, 0600);
        int log = open("/opfs/wasmfs-s2.log", O_RDWR | O_CREAT | O_TRUNC, 0600);
        std::vector<double> l1, l2;
        std::vector<std::byte> rec;
        double b2 = 0;
        std::byte header[page] = {};
        for (std::uint64_t g = 1; g <= 200; ++g) {
            segment::turn(body, size, at, g);
            auto t = clk::now();
            int slot = g % 2;
            std::uint64_t d = segment::digest(body, size);
            pwrite(s1, body, size, 2 * page + slot * size);
            fsync(s1);
            std::memcpy(header, &g, 8);
            std::memcpy(header + 8, &d, 8);
            pwrite(s1, header, page, slot * page);
            fsync(s1);
            l1.push_back(std::chrono::duration<double, std::milli>(clk::now() - t).count());
            t = clk::now();
            rec.clear();
            for (std::size_t p = 0; p < size / page; ++p) {
                if (std::memcmp(body + p * page, shadow + p * page, page) != 0) {
                    std::memcpy(shadow + p * page, body + p * page, page);
                    auto n = static_cast<std::uint32_t>(p);
                    rec.insert(rec.end(), reinterpret_cast<std::byte*>(&n), reinterpret_cast<std::byte*>(&n) + 8);
                    rec.insert(rec.end(), body + p * page, body + (p + 1) * page);
                }
            }
            std::uint64_t rd = segment::digest(rec.data(), rec.size());
            rec.insert(rec.end(), reinterpret_cast<std::byte*>(&g), reinterpret_cast<std::byte*>(&g) + 8);
            rec.insert(rec.end(), reinterpret_cast<std::byte*>(&rd), reinterpret_cast<std::byte*>(&rd) + 8);
            write(log, rec.data(), rec.size());
            fsync(log);
            l2.push_back(std::chrono::duration<double, std::milli>(clk::now() - t).count());
            b2 += rec.size();
        }
        report("wasmfs-s1", mb, l1, double(size + page));
        report("wasmfs-s2", mb, l2, b2 / 200);
        close(s1);
        close(log);
        std::free(body);
        std::free(shadow);
    }
    std::printf("WASMFS-DONE\n");
    return 0;
}
