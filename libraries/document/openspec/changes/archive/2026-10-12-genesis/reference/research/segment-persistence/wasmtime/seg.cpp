// The wasmtime track of the segment-persistence investigation: a segment (../core/segment_core.hpp)
// made durable in a file reached through wasi-libc's POSIX calls, which map to wasi:filesystem.
//
//   seg write <S0|S1|S2> <dir> <size-MB> <sync|data> [last-generation]
//       Recovers the file in dir (or creates it), then loops: one turn, one commit, and prints
//       "C <generation>" once the commit's last sync has returned.
//   seg check <S0|S1|S2> <dir> <size-MB>
//       Recovers and prints "R <generation> <ok|torn> <why>", ok meaning equals_replay holds, or
//       "R - none" when the file does not exist yet.
//   seg bench <S0|S1|S2> <dir> <size-MB> <sync|data> <commits>
//       A fresh file after 1000 turns, then <commits> turns each committed; prints latency figures.
//
// File layouts (PAGE = 4096):
//   S0  s0.img: header page, image.                       Commit: dirty pages in place, header, sync.
//   S1  s1.img: header A, header B, slot A, slot B.       Commit: image to free slot, sync, header, sync.
//   S2  s2.img: header page, image; s2.log: commits.      Commit: append dirty pages and a commit record
//       to the log, sync the log. Every 64 commits (or 8 MB of log): checkpoint = the pages dirtied since
//       the last checkpoint and the header into s2.img, sync, truncate the log, sync.

#include "segment_core.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

using segment::uint64_t;
constexpr uint64_t page = 4096;
constexpr uint64_t magic_header = 0x5345474845414452ULL;  // "SEGHEADR"
constexpr uint64_t magic_begin = 0x5345474C4F474247ULL;   // "SEGLOGBG"
constexpr uint64_t magic_commit = 0x5345474C4F47434DULL;  // "SEGLOGCM"
constexpr uint64_t checkpoint_every = 64;
constexpr uint64_t checkpoint_log_bytes = 8u << 20;

[[noreturn]] void die(const char* what) {
    std::fprintf(stderr, "error: %s: %s\n", what, std::strerror(errno));
    std::exit(2);
}

double now_ms() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return double(t.tv_sec) * 1e3 + double(t.tv_nsec) / 1e6;
}

std::byte* alloc_body(uint64_t size) {
    return static_cast<std::byte*>(::operator new(size, std::align_val_t{page}));
}

/** A file and the counters a commit is measured with. */
struct file {
    int fd = -1;
    bool data_only = true;  // fdatasync (sync-data) rather than fsync (sync)
    uint64_t bytes = 0;
    double sync_ms = 0;
    int syncs = 0;

    void write(const void* buf, uint64_t n, uint64_t at) {
        auto* p = static_cast<const char*>(buf);
        while (n > 0) {
            ssize_t w = ::pwrite(fd, p, n, off_t(at));
            if (w < 0) {
                if (errno == EINTR) continue;
                die("pwrite");
            }
            p += w;
            n -= uint64_t(w);
            at += uint64_t(w);
            bytes += uint64_t(w);
        }
    }
    // Reads up to n bytes; returns how many the file had.
    uint64_t read(void* buf, uint64_t n, uint64_t at) const {
        auto* p = static_cast<char*>(buf);
        uint64_t got = 0;
        while (got < n) {
            ssize_t r = ::pread(fd, p + got, n - got, off_t(at + got));
            if (r < 0) {
                if (errno == EINTR) continue;
                die("pread");
            }
            if (r == 0) break;
            got += uint64_t(r);
        }
        return got;
    }
    void sync() {
        double t = now_ms();
        if ((data_only ? ::fdatasync(fd) : ::fsync(fd)) != 0) die("sync");
        sync_ms += now_ms() - t;
        ++syncs;
    }
    void truncate(uint64_t n) {
        if (::ftruncate(fd, off_t(n)) != 0) die("ftruncate");
    }
    uint64_t length() const {
        struct stat st{};
        if (::fstat(fd, &st) != 0) die("fstat");
        return uint64_t(st.st_size);
    }
};

int open_file(const std::string& path, bool create) {
    int fd = ::open(path.c_str(), O_RDWR | (create ? O_CREAT | O_TRUNC : 0), 0644);
    if (fd < 0 && create) die(path.c_str());
    return fd;
}

bool exists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

struct header {
    uint64_t magic, generation, size, offset, digest, check;
    uint64_t sum() const {
        return segment::digest(reinterpret_cast<const std::byte*>(this), 5 * sizeof(uint64_t));
    }
    void seal() { check = sum(); }
    bool valid(uint64_t want_size) const { return magic == magic_header && check == sum() && size == want_size; }
};

/** The segment in linear memory: the live body, a copy of what the last commit made durable. */
struct state {
    uint64_t size = 0;
    std::byte* body = nullptr;
    std::byte* shadow = nullptr;  // S0 and S2 only: the image as of the last commit
    uint64_t offset = 0;
    uint64_t generation = 0;
};

// Runs of consecutive pages that differ between body and shadow: [first, last).
std::vector<std::pair<uint64_t, uint64_t>> dirty_runs(const state& s) {
    std::vector<std::pair<uint64_t, uint64_t>> runs;
    uint64_t pages = s.size / page;
    for (uint64_t p = 0; p < pages; ++p) {
        if (std::memcmp(s.body + p * page, s.shadow + p * page, page) == 0) continue;
        if (!runs.empty() && runs.back().second == p) {
            runs.back().second = p + 1;
        } else {
            runs.emplace_back(p, p + 1);
        }
    }
    return runs;
}

struct recovered {
    bool found = false;
    uint64_t generation = 0;
    uint64_t offset = 0;
    std::string why;
};

// ---------------------------------------------------------------------------------------------- S0

struct s0 {
    std::string dir;
    file img;
    std::string path() const { return dir + "/s0.img"; }

    void create(state& s) {
        std::string tmp = path() + ".new";
        img.fd = open_file(tmp, true);
        header h{magic_header, s.generation, s.size, s.offset, 0, 0};
        h.seal();
        img.write(&h, sizeof h, 0);
        img.write(s.body, s.size, page);
        img.sync();
        if (::rename(tmp.c_str(), path().c_str()) != 0) die("rename");
    }
    recovered recover(state& s) {
        recovered r;
        img.fd = open_file(path(), false);
        if (img.fd < 0) return r;
        header h{};
        img.read(&h, sizeof h, 0);
        if (!h.valid(s.size)) {
            r.why = "header invalid";
            return r;
        }
        if (img.read(s.body, s.size, page) != s.size) {
            r.why = "image short";
            return r;
        }
        r = {true, h.generation, h.offset, "header"};
        return r;
    }
    void resume(state&) {}
    void commit(state& s) {
        for (auto [first, last] : dirty_runs(s)) {
            img.write(s.body + first * page, (last - first) * page, page + first * page);
            std::memcpy(s.shadow + first * page, s.body + first * page, (last - first) * page);
        }
        header h{magic_header, s.generation, s.size, s.offset, 0, 0};
        h.seal();
        img.write(&h, sizeof h, 0);
        img.sync();
    }
};

// ---------------------------------------------------------------------------------------------- S1

struct s1 {
    std::string dir;
    file img;
    int active = 0;
    std::string path() const { return dir + "/s1.img"; }
    uint64_t slot_at(int slot, uint64_t size) const { return 2 * page + uint64_t(slot) * size; }

    void create(state& s) {
        std::string tmp = path() + ".new";
        img.fd = open_file(tmp, true);
        img.write(s.body, s.size, slot_at(0, s.size));
        header h{magic_header, s.generation, s.size, s.offset, segment::digest(s.body, s.size), 0};
        h.seal();
        img.write(&h, sizeof h, 0);
        header none{};
        img.write(&none, sizeof none, page);
        img.write(&none, 8, slot_at(1, s.size) + s.size - 8);  // the file's full length
        img.sync();
        if (::rename(tmp.c_str(), path().c_str()) != 0) die("rename");
        active = 0;
    }
    recovered recover(state& s) {
        recovered r;
        img.fd = open_file(path(), false);
        if (img.fd < 0) return r;
        header h[2]{};
        img.read(&h[0], sizeof h[0], 0);
        img.read(&h[1], sizeof h[1], page);
        int order[2] = {0, 1};
        if (h[1].generation > h[0].generation) std::swap(order[0], order[1]);
        r.why = "no valid slot";
        for (int slot : order) {
            if (!h[slot].valid(s.size)) continue;
            if (img.read(s.body, s.size, slot_at(slot, s.size)) != s.size) continue;
            if (segment::digest(s.body, s.size) != h[slot].digest) continue;
            active = slot;
            r = {true, h[slot].generation, h[slot].offset, slot == 0 ? "slot A" : "slot B"};
            return r;
        }
        return r;
    }
    void resume(state&) {}
    void commit(state& s) {
        int next = 1 - active;
        img.write(s.body, s.size, slot_at(next, s.size));
        img.sync();
        header h{magic_header, s.generation, s.size, s.offset, segment::digest(s.body, s.size), 0};
        h.seal();
        img.write(&h, sizeof h, uint64_t(next) * page);
        img.sync();
        active = next;
    }
};

// ---------------------------------------------------------------------------------------------- S2

struct log_begin {
    uint64_t magic, generation, pages;
};
struct log_commit {
    uint64_t magic, generation, pages, digest;
};

struct s2 {
    std::string dir;
    file img;
    file log;
    uint64_t log_end = 0;
    uint64_t since_checkpoint = 0;
    std::vector<bool> touched;  // pages committed to the log since the last checkpoint
    int checkpoints = 0;
    std::vector<std::byte> buffer;

    std::string path() const { return dir + "/s2.img"; }
    std::string log_path() const { return dir + "/s2.log"; }

    void create(state& s) {
        std::string tmp = path() + ".new";
        img.fd = open_file(tmp, true);
        header h{magic_header, s.generation, s.size, s.offset, 0, 0};
        h.seal();
        img.write(&h, sizeof h, 0);
        img.write(s.body, s.size, page);
        img.sync();
        log.fd = open_file(log_path(), true);
        log.sync();
        if (::rename(tmp.c_str(), path().c_str()) != 0) die("rename");
        touched.assign(s.size / page, false);
    }
    recovered recover(state& s) {
        recovered r;
        img.fd = open_file(path(), false);
        log.fd = open_file(log_path(), false);
        if (img.fd < 0 || log.fd < 0) return r;
        header h{};
        img.read(&h, sizeof h, 0);
        if (!h.valid(s.size)) {
            r.why = "header invalid";
            return r;
        }
        if (img.read(s.body, s.size, page) != s.size) {
            r.why = "image short";
            return r;
        }
        uint64_t length = log.length();
        std::vector<std::byte> all(length);
        log.read(all.data(), length, 0);
        uint64_t generation = h.generation;
        uint64_t at = 0;
        int replayed = 0;
        bool first = true;
        while (at + sizeof(log_begin) <= length) {
            log_begin b{};
            std::memcpy(&b, all.data() + at, sizeof b);
            if (b.magic != magic_begin) break;
            uint64_t records = b.pages * (8 + page);
            uint64_t end = at + sizeof b + records + sizeof(log_commit);
            if (end > length) break;
            log_commit c{};
            std::memcpy(&c, all.data() + end - sizeof c, sizeof c);
            if (c.magic != magic_commit || c.generation != b.generation || c.pages != b.pages) break;
            if (segment::digest(all.data() + at, sizeof b + records) != c.digest) break;
            if (first ? b.generation > h.generation + 1 : b.generation != generation + 1) {
                r.why = "log gap";
                return r;
            }
            for (uint64_t i = 0; i < b.pages; ++i) {
                const std::byte* rec = all.data() + at + sizeof b + i * (8 + page);
                uint64_t p = 0;
                std::memcpy(&p, rec, 8);
                if (p >= s.size / page) {
                    r.why = "page out of range";
                    return r;
                }
                std::memcpy(s.body + p * page, rec + 8, page);
            }
            generation = b.generation;
            first = false;
            ++replayed;
            at = end;
        }
        log_end = at;
        r = {true, std::max(generation, h.generation), h.offset,
             "image " + std::to_string(h.generation) + " + " + std::to_string(replayed) + " log commits"};
        return r;
    }
    // After recovery the replayed state becomes the image and the log starts empty.
    void resume(state& s) {
        header h{magic_header, s.generation, s.size, s.offset, 0, 0};
        h.seal();
        img.write(s.body, s.size, page);
        img.write(&h, sizeof h, 0);
        img.sync();
        log.truncate(0);
        log.sync();
        log_end = 0;
        touched.assign(s.size / page, false);
    }
    void checkpoint(state& s) {
        uint64_t pages = s.size / page;
        for (uint64_t p = 0; p < pages;) {
            if (!touched[p]) {
                ++p;
                continue;
            }
            uint64_t q = p;
            while (q < pages && touched[q]) ++q;
            img.write(s.shadow + p * page, (q - p) * page, page + p * page);
            p = q;
        }
        header h{magic_header, s.generation, s.size, s.offset, 0, 0};
        h.seal();
        img.write(&h, sizeof h, 0);
        img.sync();
        log.truncate(0);
        log.sync();
        log_end = 0;
        since_checkpoint = 0;
        touched.assign(pages, false);
        ++checkpoints;
    }
    void commit(state& s) {
        auto runs = dirty_runs(s);
        uint64_t n = 0;
        for (auto [first, last] : runs) n += last - first;
        buffer.resize(sizeof(log_begin) + n * (8 + page) + sizeof(log_commit));
        log_begin b{magic_begin, s.generation, n};
        std::memcpy(buffer.data(), &b, sizeof b);
        std::byte* out = buffer.data() + sizeof b;
        for (auto [first, last] : runs) {
            for (uint64_t p = first; p < last; ++p) {
                std::memcpy(out, &p, 8);
                std::memcpy(out + 8, s.body + p * page, page);
                out += 8 + page;
                touched[p] = true;
            }
            std::memcpy(s.shadow + first * page, s.body + first * page, (last - first) * page);
        }
        log_commit c{magic_commit, s.generation, n, segment::digest(buffer.data(), uint64_t(out - buffer.data()))};
        std::memcpy(out, &c, sizeof c);
        log.write(buffer.data(), buffer.size(), log_end);
        log.sync();
        log_end += buffer.size();
        if (++since_checkpoint >= checkpoint_every || log_end >= checkpoint_log_bytes) checkpoint(s);
    }
};

// ---------------------------------------------------------------------------------------------- driver

template <class Strategy> int run(const std::string& mode, Strategy st, uint64_t size, bool data_only,
                                  uint64_t count) {
    st.img.data_only = data_only;
    if constexpr (requires { st.log; }) st.log.data_only = data_only;
    state s;
    s.size = size;
    s.body = alloc_body(size);

    if (mode == "check") {
        recovered r = st.recover(s);
        if (!r.found) {
            std::printf(r.why.empty() ? "R - none no file\n" : "R - torn %s\n", r.why.c_str());
            return 0;
        }
        std::byte* scratch = alloc_body(size);
        bool ok = segment::equals_replay(s.body, size, r.offset, r.generation, scratch);
        std::printf("R %llu %s %s\n", (unsigned long long)r.generation, ok ? "ok" : "torn", r.why.c_str());
        return 0;
    }

    s.shadow = alloc_body(size);
    if (mode == "write") {
        recovered r = st.recover(s);
        if (r.found) {
            s.generation = r.generation;
            s.offset = r.offset;
            std::memcpy(s.shadow, s.body, size);
            st.resume(s);
        } else {
            s.offset = segment::create(s.body, size);
            std::memcpy(s.shadow, s.body, size);
            st.create(s);
        }
        std::printf("S %llu\n", (unsigned long long)s.generation);
        std::fflush(stdout);
        while (count == 0 || s.generation < count) {
            segment::turn(s.body, size, s.offset, ++s.generation);
            st.commit(s);
            std::printf("C %llu\n", (unsigned long long)s.generation);
            std::fflush(stdout);
        }
        return 0;
    }

    // bench
    s.offset = segment::create(s.body, size);
    for (uint64_t g = 1; g <= 1000; ++g) segment::turn(s.body, size, s.offset, g);
    s.generation = 1000;
    std::memcpy(s.shadow, s.body, size);
    st.create(s);
    std::vector<double> lat;
    std::vector<double> syncs;
    uint64_t bytes = 0;
    for (uint64_t i = 0; i < count; ++i) {
        segment::turn(s.body, size, s.offset, ++s.generation);
        uint64_t b0 = st.img.bytes;
        double s0 = st.img.sync_ms;
        if constexpr (requires { st.log; }) {
            b0 += st.log.bytes;
            s0 += st.log.sync_ms;
        }
        double t = now_ms();
        st.commit(s);
        lat.push_back(now_ms() - t);
        uint64_t b1 = st.img.bytes;
        double s1v = st.img.sync_ms;
        if constexpr (requires { st.log; }) {
            b1 += st.log.bytes;
            s1v += st.log.sync_ms;
        }
        bytes += b1 - b0;
        syncs.push_back(s1v - s0);
    }
    auto pct = [](std::vector<double> v, double q) {
        std::sort(v.begin(), v.end());
        return v[std::min(v.size() - 1, size_t(q * double(v.size())))];
    };
    double mean = 0;
    for (double x : lat) mean += x;
    mean /= double(lat.size());
    int checkpoints = 0;
    if constexpr (requires { st.checkpoints; }) checkpoints = st.checkpoints;
    std::printf("median_ms %.3f p99_ms %.3f mean_ms %.3f max_ms %.3f sync_median_ms %.3f bytes_per_commit %llu "
                "checkpoints %d\n",
                pct(lat, 0.5), pct(lat, 0.99), mean, pct(lat, 1.0), pct(syncs, 0.5),
                (unsigned long long)(bytes / count), checkpoints);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        std::fprintf(stderr, "usage: seg <write|check|bench> <S0|S1|S2> <dir> <size-MB> [sync|data] [n]\n");
        return 2;
    }
    std::string mode = argv[1], strategy = argv[2], dir = argv[3];
    uint64_t size = std::strtoull(argv[4], nullptr, 10) << 20;
    bool data_only = argc > 5 ? std::string(argv[5]) != "sync" : true;
    uint64_t count = argc > 6 ? std::strtoull(argv[6], nullptr, 10) : 0;
    if (strategy == "S0") return run(mode, s0{dir, {}}, size, data_only, count);
    if (strategy == "S1") return run(mode, s1{dir, {}}, size, data_only, count);
    if (strategy == "S2") return run(mode, s2{dir, {}, {}}, size, data_only, count);
    return 2;
}
