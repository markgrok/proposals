// Native track of the segment-persistence investigation (see ../PROTOCOL.md).
//
// One program, three modes:
//   seg bench <strategy> <sync> <dir> <size_mb> <commits>   commit latency and bytes per commit
//   seg write <strategy> <sync> <dir> <size_mb>             turns and commits until killed
//   seg check <strategy> <dir> <size_mb>                    recover, verify with equals_replay
//
// Strategies:
//   s0      no protocol: the dirty 4 KB pages written in place with pwrite, then the header, one sync
//   s1      two slots: the whole image into the free slot, sync, its header, sync
//   s1d     two slots, writing only the pages that differ from what the free slot holds (g - 2)
//   s2      write-ahead log of dirty pages (pwrite), checkpoint into the image every 64 commits
//   s2mmap  s2 with the image written through a MAP_SHARED mapping at checkpoint (msync MS_SYNC)
//   s2p     s2 with a preallocated log, overwritten from its start after each checkpoint instead of
//           appended and truncated, so a log sync never changes the file's size or its blocks
//   s3raw   S3 alone: the turns run in a raw MAP_SHARED mapping of the file, msync at commit
//   s3bip   S3 alone with Boost.Interprocess basic_managed_mapped_file holding the heap
//   tear    not a strategy: one 64 MB pwrite per step, to see whether SIGKILL can split one write
//
// Sync modes: fsync, fdatasync, full (F_FULLFSYNC), barrier (F_BARRIERFSYNC), and for the mapped
// strategies msync (MS_SYNC alone; with any other mode msync(MS_SYNC) runs first), and for s3bip
// bipflush (managed_mapped_file::flush() exactly as Boost provides it).

#include "../core/segment_core.hpp"

#include <boost/interprocess/managed_mapped_file.hpp>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifdef __APPLE__
#include <libproc.h>
#include <sys/resource.h>
extern "C" int fdatasync(int);  // exported by libsystem_kernel, not declared by the SDK
#endif

namespace {

using std::uint64_t;
constexpr uint64_t hdr_region = 65536;  // header area: a multiple of every VM page size (4K, 16K, 64K)
constexpr uint64_t pg = 4096;           // the protocol's page
constexpr uint64_t head_magic = 0x5345474845414452ULL;
constexpr uint64_t log_magic = 0x5345474c4f474844ULL;
constexpr uint64_t log_trail = 0x5345474c4f475452ULL;
constexpr uint64_t checkpoint_every = 64;

[[noreturn]] void die(const char* what) {
    std::fprintf(stderr, "seg: %s: %s\n", what, std::strerror(errno));
    std::_Exit(2);
}

uint64_t mix(uint64_t h, uint64_t v) {
    h ^= v + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
    h *= 0xff51afd7ed558ccdULL;
    return h ^ (h >> 33);
}

// A word-wise hash, about 8 bytes per cycle; segment::digest (FNV-1a, a byte at a time) is too slow
// for a 64 MB slot (measured in the bench's "fnv" line).
uint64_t fast_hash(const std::byte* p, uint64_t n) {
    uint64_t a = 1, b = 2, c = 3, d = 4;
    uint64_t i = 0;
    for (; i + 32 <= n; i += 32) {
        uint64_t w[4];
        std::memcpy(w, p + i, 32);
        a = (a ^ w[0]) * 0x9E3779B97F4A7C15ULL;
        b = (b ^ w[1]) * 0xC2B2AE3D27D4EB4FULL;
        c = (c ^ w[2]) * 0x165667B19E3779F9ULL;
        d = (d ^ w[3]) * 0x27D4EB2F165667C5ULL;
        a ^= a >> 29;
        b ^= b >> 31;
        c ^= c >> 27;
        d ^= d >> 30;
    }
    uint64_t h = mix(mix(mix(mix(n, a), b), c), d);
    for (; i < n; ++i) h = mix(h, static_cast<uint64_t>(p[i]));
    return h;
}

enum class strat { s0, s1, s1d, s2, s2mmap, s2p, s3raw, s3bip, tear };
enum class syncm { fsync, fdatasync, full, barrier, msync, bipflush };

strat parse_strat(std::string_view s) {
    if (s == "s0") return strat::s0;
    if (s == "s1") return strat::s1;
    if (s == "s1d") return strat::s1d;
    if (s == "s2") return strat::s2;
    if (s == "s2mmap") return strat::s2mmap;
    if (s == "s2p") return strat::s2p;
    if (s == "s3raw") return strat::s3raw;
    if (s == "s3bip") return strat::s3bip;
    if (s == "tear") return strat::tear;
    std::fprintf(stderr, "unknown strategy\n");
    std::_Exit(2);
}

syncm parse_sync(std::string_view s) {
    if (s == "fsync") return syncm::fsync;
    if (s == "fdatasync") return syncm::fdatasync;
    if (s == "full") return syncm::full;
    if (s == "barrier") return syncm::barrier;
    if (s == "msync") return syncm::msync;
    if (s == "bipflush") return syncm::bipflush;
    std::fprintf(stderr, "unknown sync\n");
    std::_Exit(2);
}

// The file-descriptor sync the mode names; msync and bipflush name none.
void fd_sync(int fd, syncm m) {
    int r = 0;
    switch (m) {
    case syncm::fsync: r = ::fsync(fd); break;
    case syncm::fdatasync: r = ::fdatasync(fd); break;
#ifdef __APPLE__
    case syncm::full: r = ::fcntl(fd, F_FULLFSYNC); break;
    case syncm::barrier: r = ::fcntl(fd, F_BARRIERFSYNC); break;
#else
    case syncm::full:
    case syncm::barrier: errno = ENOTSUP; r = -1; break;
#endif
    case syncm::msync:
    case syncm::bipflush: break;
    }
    if (r != 0) die("sync");
}

// The log of s2 and the files a pwrite strategy writes need a real sync even in mode msync.
syncm fd_mode(syncm m) { return (m == syncm::msync || m == syncm::bipflush) ? syncm::fdatasync : m; }

std::byte* aligned_buffer(uint64_t size) {
    void* p = nullptr;
    if (posix_memalign(&p, hdr_region, size) != 0) die("posix_memalign");
    std::memset(p, 0, size);
    return static_cast<std::byte*>(p);
}

void pwrite_all(int fd, const void* buf, uint64_t n, uint64_t off) {
    auto* p = static_cast<const char*>(buf);
    while (n > 0) {
        ssize_t w = ::pwrite(fd, p, n, static_cast<off_t>(off));
        if (w < 0) {
            if (errno == EINTR) continue;
            die("pwrite");
        }
        p += w;
        n -= static_cast<uint64_t>(w);
        off += static_cast<uint64_t>(w);
    }
}

// Reads up to n bytes; returns how many the file had.
uint64_t pread_all(int fd, void* buf, uint64_t n, uint64_t off) {
    auto* p = static_cast<char*>(buf);
    uint64_t got = 0;
    while (got < n) {
        ssize_t r = ::pread(fd, p + got, n - got, static_cast<off_t>(off + got));
        if (r < 0) {
            if (errno == EINTR) continue;
            die("pread");
        }
        if (r == 0) break;
        got += static_cast<uint64_t>(r);
    }
    return got;
}

uint64_t os_bytes_written() {
#ifdef __APPLE__
    rusage_info_v4 ri{};
    if (proc_pid_rusage(getpid(), RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t*>(&ri)) != 0) return 0;
    return ri.ri_diskio_byteswritten;
#else
    FILE* f = std::fopen("/proc/self/io", "r");
    if (!f) return 0;
    char line[128];
    uint64_t v = 0;
    while (std::fgets(line, sizeof line, f)) {
        if (std::strncmp(line, "write_bytes:", 12) == 0) v = std::strtoull(line + 12, nullptr, 10);
    }
    std::fclose(f);
    return v;
#endif
}

void fsync_dir(const std::string& dir) {
    int d = ::open(dir.c_str(), O_RDONLY);
    if (d < 0) die("open dir");
    if (::fsync(d) != 0) die("fsync dir");
    ::close(d);
}

// The status file: what the writer last reported as committed and where it was. A MAP_SHARED store
// survives SIGKILL (it is in the page cache), so the driver reads it after the kill.
enum phase : uint64_t {
    ph_start = 1, ph_turn, ph_diff, ph_write, ph_sync, ph_hdr_write, ph_hdr_sync, ph_msync, ph_report,
    ph_log_write, ph_log_sync, ph_ck_write, ph_ck_sync, ph_truncate, ph_trunc_sync
};
const char* phase_name(uint64_t p) {
    static const char* names[] = {"none", "start", "turn", "diff", "write", "sync", "hdr_write", "hdr_sync",
                                  "msync", "report", "log_write", "log_sync", "ck_write", "ck_sync",
                                  "truncate", "trunc_sync"};
    return p < sizeof names / sizeof names[0] ? names[p] : "?";
}
struct status {
    volatile uint64_t reported;
    volatile uint64_t phase;
    volatile uint64_t commits;
};
status* g_status = nullptr;
uint64_t g_diff_ns = 0;  // time spent finding dirty pages, for the bench
status scratch_status{};
void set_phase(uint64_t p) { g_status->phase = p; }

status* map_status(const std::string& dir) {
    std::string path = dir + "/status";
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) die("open status");
    if (::ftruncate(fd, 4096) != 0) die("ftruncate status");
    void* m = ::mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) die("mmap status");
    ::close(fd);
    return static_cast<status*>(m);
}

struct header {
    uint64_t magic, gen, offset, size, digest, check;
    uint64_t compute() const { return mix(mix(mix(mix(mix(magic, gen), offset), size), digest), 7); }
    bool valid(uint64_t sz) const { return magic == head_magic && size == sz && check == compute(); }
};
header make_header(uint64_t gen, uint64_t offset, uint64_t size, uint64_t digest) {
    header h{head_magic, gen, offset, size, digest, 0};
    h.check = h.compute();
    return h;
}

uint64_t image_digest(const std::byte* img, uint64_t size) {
    uint64_t h = size;
    for (uint64_t p = 0; p < size; p += pg) h = mix(h, fast_hash(img + p, pg));
    return h;
}

struct recovered {
    bool ok = false;      // a valid generation was found (strategies without digests: the header is valid)
    bool present = true;  // the file exists
    uint64_t gen = 0, offset = 0, size = 0;
    std::byte* body = nullptr;  // 16-aligned image to verify (a private copy)
    std::string note;
};

std::string p_image(const std::string& d) { return d + "/image"; }
std::string p_log(const std::string& d) { return d + "/log"; }
std::string p_meta(const std::string& d) { return d + "/meta"; }

bool exists(const std::string& p) {
    struct stat st{};
    return ::stat(p.c_str(), &st) == 0;
}

// --- recovery, shared by the checker and by a writer reopening its files -------------------------

recovered recover_s0(const std::string& dir, uint64_t size) {
    recovered r;
    int fd = ::open(p_image(dir).c_str(), O_RDONLY);
    if (fd < 0) { r.present = false; return r; }
    header h{};
    pread_all(fd, &h, sizeof h, 0);
    r.size = size;
    r.body = aligned_buffer(size);
    uint64_t got = pread_all(fd, r.body, size, hdr_region);
    ::close(fd);
    if (!h.valid(size) || got != size) { r.note = "bad header or short file"; return r; }
    r.ok = true;
    r.gen = h.gen;
    r.offset = h.offset;
    return r;
}

recovered recover_s1(const std::string& dir, uint64_t size) {
    recovered r;
    int fd = ::open(p_image(dir).c_str(), O_RDONLY);
    if (fd < 0) { r.present = false; return r; }
    r.size = size;
    r.body = aligned_buffer(size);
    std::byte* tmp = aligned_buffer(size);
    for (int s = 0; s < 2; ++s) {
        header h{};
        pread_all(fd, &h, sizeof h, static_cast<uint64_t>(s) * pg);
        if (!h.valid(size)) continue;
        if (pread_all(fd, tmp, size, hdr_region + static_cast<uint64_t>(s) * size) != size) continue;
        if (image_digest(tmp, size) != h.digest) { r.note += "slot digest mismatch;"; continue; }
        if (!r.ok || h.gen > r.gen) {
            r.ok = true;
            r.gen = h.gen;
            r.offset = h.offset;
            std::memcpy(r.body, tmp, size);
        }
    }
    std::free(tmp);
    ::close(fd);
    return r;
}

struct log_head { uint64_t magic, gen, npages, offset; };
struct log_tail { uint64_t magic, gen, digest; };

// Applies every complete, valid commit of the log, in order, over the image. Returns the log's last
// generation (0 when none) and the byte length of its valid prefix.
std::pair<uint64_t, uint64_t> replay_log(const std::string& dir, std::byte* img, uint64_t size, uint64_t image_gen,
                                         std::string& note, bool& gap) {
    gap = false;
    int fd = ::open(p_log(dir).c_str(), O_RDONLY);
    if (fd < 0) return {0, 0};
    struct stat st{};
    ::fstat(fd, &st);
    std::vector<std::byte> log(static_cast<size_t>(st.st_size));
    pread_all(fd, log.data(), log.size(), 0);
    ::close(fd);
    // Pass 1 finds the valid chain; pass 2 applies it, only when it reaches the image's generation
    // (a chain ending before it is an older cycle's left-overs of a reused log).
    uint64_t pos = 0, last = 0;
    for (int pass = 0; pass < 2; ++pass) {
    if (pass == 1) {
        if (last < image_gen) { note += "log: chain older than image, ignored;"; return {0, 0}; }
        pos = 0;
        last = 0;
    }
    while (pos + sizeof(log_head) <= log.size()) {
        log_head h;
        std::memcpy(&h, log.data() + pos, sizeof h);
        if (h.magic != log_magic || h.npages > size / pg) { note += "log: bad head;"; break; }
        uint64_t body = sizeof h + h.npages * 8 + h.npages * pg;
        if (pos + body + sizeof(log_tail) > log.size()) { note += "log: short commit;"; break; }
        log_tail t;
        std::memcpy(&t, log.data() + pos + body, sizeof t);
        if (t.magic != log_trail || t.gen != h.gen || t.digest != fast_hash(log.data() + pos, body)) {
            note += "log: bad commit digest;";
            break;
        }
        if (last == 0 && h.gen > image_gen + 1) { gap = true; note += "log: gap after image;"; break; }
        if (last != 0 && h.gen != last + 1) { note += "log: non-consecutive;"; break; }
        const std::byte* nos = log.data() + pos + sizeof h;
        const std::byte* pages = nos + h.npages * 8;
        for (uint64_t i = 0; i < h.npages; ++i) {
            uint64_t no;
            std::memcpy(&no, nos + i * 8, 8);
            if (no * pg + pg > size) { note += "log: page out of range;"; break; }
            if (pass == 1) std::memcpy(img + no * pg, pages + i * pg, pg);
        }
        last = h.gen;
        pos += body + sizeof(log_tail);
    }
    }
    return {last, pos};
}

recovered recover_s2(const std::string& dir, uint64_t size) {
    recovered r = recover_s0(dir, size);
    if (!r.ok) return r;
    bool gap = false;
    auto [last, len] = replay_log(dir, r.body, size, r.gen, r.note, gap);
    (void)len;
    if (gap) { r.ok = false; return r; }
    r.gen = std::max(r.gen, last);
    return r;
}

struct bip_meta { uint64_t user_offset, seg_size, offset, gen_offset; };

recovered recover_s3bip(const std::string& dir, uint64_t) {
    recovered r;
    int mfd = ::open(p_meta(dir).c_str(), O_RDONLY);
    int fd = ::open(p_image(dir).c_str(), O_RDONLY);
    if (mfd < 0 || fd < 0) {
        r.present = false;
        if (mfd >= 0) ::close(mfd);
        if (fd >= 0) ::close(fd);
        return r;
    }
    bip_meta m{};
    pread_all(mfd, &m, sizeof m, 0);
    ::close(mfd);
    struct stat st{};
    ::fstat(fd, &st);
    auto total = static_cast<uint64_t>(st.st_size);
    std::byte* all = aligned_buffer(total);
    uint64_t got = pread_all(fd, all, total, 0);
    ::close(fd);
    if (got != total || m.user_offset + m.seg_size > total) { r.note = "short file"; return r; }
    // The image under test is the segment manager's area; copy it to a fresh 16-aligned buffer.
    r.size = m.seg_size;
    r.body = aligned_buffer(m.seg_size);
    std::memcpy(r.body, all + m.user_offset, m.seg_size);
    std::free(all);
    r.offset = m.offset;
    std::memcpy(&r.gen, r.body + m.gen_offset, 8);
    r.ok = true;
    return r;
}

recovered recover(strat s, const std::string& dir, uint64_t size) {
    switch (s) {
    case strat::s0:
    case strat::s3raw: return recover_s0(dir, size);
    case strat::s1:
    case strat::s1d: return recover_s1(dir, size);
    case strat::s2:
    case strat::s2mmap:
    case strat::s2p: return recover_s2(dir, size);
    case strat::s3bip: return recover_s3bip(dir, size);
    case strat::tear: break;
    }
    return {};
}

// --- engines -------------------------------------------------------------------------------------

struct engine {
    virtual ~engine() = default;
    virtual std::byte* body() = 0;
    virtual uint64_t size() const = 0;
    virtual uint64_t offset() const = 0;
    virtual uint64_t gen() const = 0;
    virtual void commit(uint64_t g) = 0;
    uint64_t app_bytes = 0;  // bytes this process handed to pwrite or copied into a mapping
};

// Writes a fresh file at path.tmp with fill(fd), syncs it and renames it into place: a file that is
// visible under its name is complete.
template <class F> void create_atomically(const std::string& dir, const std::string& path, syncm m, F fill) {
    std::string tmp = path + ".tmp";
    int fd = ::open(tmp.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) die("open tmp");
    fill(fd);
    fd_sync(fd, fd_mode(m));
    ::close(fd);
    if (::rename(tmp.c_str(), path.c_str()) != 0) die("rename");
    fsync_dir(dir);
}

// Working image plus the copy of the last committed one: the dirty tracking WebAssembly has.
struct tracked {
    uint64_t size = 0, offset = 0, gen = 0;
    std::byte* work = nullptr;
    std::byte* committed = nullptr;
    std::vector<uint64_t> dirty;
    void init(std::byte* img, uint64_t sz, uint64_t off, uint64_t g) {
        size = sz;
        offset = off;
        gen = g;
        work = img;
        committed = aligned_buffer(sz);
        std::memcpy(committed, work, sz);
    }
    void diff() {
        set_phase(ph_diff);
        auto t0 = std::chrono::steady_clock::now();
        dirty.clear();
        for (uint64_t p = 0; p < size; p += pg) {
            if (std::memcmp(work + p, committed + p, pg) != 0) dirty.push_back(p / pg);
        }
        g_diff_ns += static_cast<uint64_t>(std::chrono::nanoseconds(std::chrono::steady_clock::now() - t0).count());
    }
    void accept() {
        for (uint64_t no : dirty) std::memcpy(committed + no * pg, work + no * pg, pg);
    }
};

// Writes runs of consecutive dirty pages from src at base.
uint64_t write_pages(int fd, const std::byte* src, const std::vector<uint64_t>& pages, uint64_t base) {
    uint64_t bytes = 0;
    for (size_t i = 0; i < pages.size();) {
        size_t j = i + 1;
        while (j < pages.size() && pages[j] == pages[j - 1] + 1) ++j;
        uint64_t n = (j - i) * pg;
        pwrite_all(fd, src + pages[i] * pg, n, base + pages[i] * pg);
        bytes += n;
        i = j;
    }
    return bytes;
}

struct s0_engine : engine {
    tracked t;
    int fd = -1;
    syncm m;
    s0_engine(const std::string& dir, uint64_t size, syncm mode) : m(mode) {
        if (!exists(p_image(dir))) {
            std::byte* img = aligned_buffer(size);
            uint64_t off = segment::create(img, size);
            create_atomically(dir, p_image(dir), m, [&](int f) {
                header h = make_header(0, off, size, 0);
                pwrite_all(f, &h, sizeof h, 0);
                pwrite_all(f, img, size, hdr_region);
            });
            std::free(img);
        }
        recovered r = recover_s0(dir, size);
        if (!r.ok) die("s0 reopen");
        t.init(r.body, size, r.offset, r.gen);
        fd = ::open(p_image(dir).c_str(), O_RDWR);
        if (fd < 0) die("open image");
    }
    std::byte* body() override { return t.work; }
    uint64_t size() const override { return t.size; }
    uint64_t offset() const override { return t.offset; }
    uint64_t gen() const override { return t.gen; }
    void commit(uint64_t g) override {
        t.diff();
        set_phase(ph_write);
        app_bytes += write_pages(fd, t.work, t.dirty, hdr_region);
        set_phase(ph_hdr_write);
        header h = make_header(g, t.offset, t.size, 0);
        pwrite_all(fd, &h, sizeof h, 0);
        app_bytes += sizeof h;
        set_phase(ph_sync);
        fd_sync(fd, m);
        t.accept();
        t.gen = g;
    }
};

struct s1_engine : engine {
    tracked t;
    int fd = -1;
    syncm m;
    bool delta;
    int active = 0;
    std::byte* slot_copy[2] = {nullptr, nullptr};  // what each slot holds, for s1d
    s1_engine(const std::string& dir, uint64_t size, syncm mode, bool d) : m(mode), delta(d) {
        if (!exists(p_image(dir))) {
            std::byte* img = aligned_buffer(size);
            uint64_t off = segment::create(img, size);
            create_atomically(dir, p_image(dir), m, [&](int f) {
                header a = make_header(0, off, size, image_digest(img, size));
                header b{};
                pwrite_all(f, &a, sizeof a, 0);
                pwrite_all(f, &b, sizeof b, pg);
                pwrite_all(f, img, size, hdr_region);
                pwrite_all(f, img, size, hdr_region + size);
            });
            std::free(img);
        }
        recovered r = recover_s1(dir, size);
        if (!r.ok) die("s1 reopen");
        t.init(r.body, size, r.offset, r.gen);
        fd = ::open(p_image(dir).c_str(), O_RDWR);
        if (fd < 0) die("open image");
        // Which slot holds the recovered generation: the one whose valid header has it.
        for (int s = 0; s < 2; ++s) {
            header h{};
            pread_all(fd, &h, sizeof h, static_cast<uint64_t>(s) * pg);
            if (h.valid(size) && h.gen == r.gen) active = s;
        }
        if (delta) {
            for (int s = 0; s < 2; ++s) {
                slot_copy[s] = aligned_buffer(size);
                pread_all(fd, slot_copy[s], size, hdr_region + static_cast<uint64_t>(s) * size);
            }
        }
    }
    std::byte* body() override { return t.work; }
    uint64_t size() const override { return t.size; }
    uint64_t offset() const override { return t.offset; }
    uint64_t gen() const override { return t.gen; }
    void commit(uint64_t g) override {
        int target = 1 - active;
        uint64_t base = hdr_region + static_cast<uint64_t>(target) * t.size;
        set_phase(ph_write);
        if (delta) {
            std::vector<uint64_t> pages;
            for (uint64_t p = 0; p < t.size; p += pg) {
                if (std::memcmp(t.work + p, slot_copy[target] + p, pg) != 0) pages.push_back(p / pg);
            }
            app_bytes += write_pages(fd, t.work, pages, base);
            for (uint64_t no : pages) std::memcpy(slot_copy[target] + no * pg, t.work + no * pg, pg);
        } else {
            pwrite_all(fd, t.work, t.size, base);
            app_bytes += t.size;
        }
        set_phase(ph_sync);
        fd_sync(fd, m);
        uint64_t dg = image_digest(t.work, t.size);
        set_phase(ph_hdr_write);
        header h = make_header(g, t.offset, t.size, dg);
        pwrite_all(fd, &h, sizeof h, static_cast<uint64_t>(target) * pg);
        app_bytes += sizeof h;
        set_phase(ph_hdr_sync);
        fd_sync(fd, m);
        active = target;
        t.gen = g;
    }
};

struct s2_engine : engine {
    tracked t;
    int fd = -1, lfd = -1;
    syncm m;
    bool mapped;
    bool prealloc;
    static constexpr uint64_t log_cap = 16ull << 20;
    std::byte* map = nullptr;  // s2mmap: the image file, header area included
    uint64_t log_end = 0, since = 0;
    std::vector<bool> ck_dirty;
    std::vector<std::byte> rec;
    s2_engine(const std::string& dir, uint64_t size, syncm mode, bool mm, bool pre = false)
        : m(mode), mapped(mm), prealloc(pre) {
        if (!exists(p_image(dir))) {
            int l = ::open(p_log(dir).c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (l < 0) die("open log");
            if (prealloc) {  // real zeros, not fallocate: an unwritten extent still changes on first write
                std::vector<std::byte> zeros(log_cap);
                pwrite_all(l, zeros.data(), zeros.size(), 0);
                fd_sync(l, fd_mode(m));
            }
            ::close(l);
            std::byte* img = aligned_buffer(size);
            uint64_t off = segment::create(img, size);
            create_atomically(dir, p_image(dir), m, [&](int f) {
                header h = make_header(0, off, size, 0);
                pwrite_all(f, &h, sizeof h, 0);
                pwrite_all(f, img, size, hdr_region);
            });
            std::free(img);
        }
        recovered r = recover_s2(dir, size);
        if (!r.ok) die("s2 reopen");
        t.init(r.body, size, r.offset, r.gen);
        ck_dirty.assign(size / pg, false);
        fd = ::open(p_image(dir).c_str(), O_RDWR);
        lfd = ::open(p_log(dir).c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0 || lfd < 0) die("open s2 files");
        if (mapped) {
            void* p = ::mmap(nullptr, hdr_region + size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (p == MAP_FAILED) die("mmap image");
            map = static_cast<std::byte*>(p);
        }
        // The recovered state becomes the image, and the log starts empty.
        std::fill(ck_dirty.begin(), ck_dirty.end(), true);
        checkpoint(r.gen);
    }
    std::byte* body() override { return t.work; }
    uint64_t size() const override { return t.size; }
    uint64_t offset() const override { return t.offset; }
    uint64_t gen() const override { return t.gen; }
    void checkpoint(uint64_t g) {
        set_phase(ph_ck_write);
        std::vector<uint64_t> pages;
        for (uint64_t i = 0; i < ck_dirty.size(); ++i) {
            if (ck_dirty[i]) pages.push_back(i);
        }
        header h = make_header(g, t.offset, t.size, 0);
        if (mapped) {
            for (uint64_t no : pages) std::memcpy(map + hdr_region + no * pg, t.committed + no * pg, pg);
            std::memcpy(map, &h, sizeof h);
            app_bytes += pages.size() * pg + sizeof h;
            set_phase(ph_msync);
            if (::msync(map, hdr_region + t.size, MS_SYNC) != 0) die("msync");
            set_phase(ph_ck_sync);
            fd_sync(fd, fd_mode(m));
        } else {
            app_bytes += write_pages(fd, t.committed, pages, hdr_region);
            pwrite_all(fd, &h, sizeof h, 0);
            app_bytes += sizeof h;
            set_phase(ph_ck_sync);
            fd_sync(fd, fd_mode(m));
        }
        if (!prealloc) {
            set_phase(ph_truncate);
            if (::ftruncate(lfd, 0) != 0) die("ftruncate log");
            set_phase(ph_trunc_sync);
            fd_sync(lfd, fd_mode(m));
        }
        log_end = 0;
        since = 0;
        std::fill(ck_dirty.begin(), ck_dirty.end(), false);
    }
    void commit(uint64_t g) override {
        t.diff();
        uint64_t n = t.dirty.size();
        log_head h{log_magic, g, n, t.offset};
        uint64_t body_len = sizeof h + n * 8 + n * pg;
        rec.resize(body_len + sizeof(log_tail));
        std::memcpy(rec.data(), &h, sizeof h);
        for (uint64_t i = 0; i < n; ++i) {
            std::memcpy(rec.data() + sizeof h + i * 8, &t.dirty[i], 8);
            std::memcpy(rec.data() + sizeof h + n * 8 + i * pg, t.work + t.dirty[i] * pg, pg);
        }
        log_tail tl{log_trail, g, fast_hash(rec.data(), body_len)};
        std::memcpy(rec.data() + body_len, &tl, sizeof tl);
        if (prealloc && log_end + rec.size() > log_cap) checkpoint(t.gen);
        set_phase(ph_log_write);
        pwrite_all(lfd, rec.data(), rec.size(), log_end);
        app_bytes += rec.size();
        set_phase(ph_log_sync);
        fd_sync(lfd, fd_mode(m));
        log_end += rec.size();
        t.accept();
        for (uint64_t no : t.dirty) ck_dirty[no] = true;
        t.gen = g;
        if (++since >= checkpoint_every) checkpoint(g);
    }
};

struct s3raw_engine : engine {
    int fd = -1;
    syncm m;
    std::byte* map = nullptr;
    uint64_t sz, off, g0;
    s3raw_engine(const std::string& dir, uint64_t size, syncm mode) : m(mode), sz(size) {
        if (!exists(p_image(dir))) {
            std::byte* img = aligned_buffer(size);
            uint64_t o = segment::create(img, size);
            create_atomically(dir, p_image(dir), fd_mode(m), [&](int f) {
                header h = make_header(0, o, size, 0);
                pwrite_all(f, &h, sizeof h, 0);
                pwrite_all(f, img, size, hdr_region);
            });
            std::free(img);
        }
        fd = ::open(p_image(dir).c_str(), O_RDWR);
        if (fd < 0) die("open image");
        void* p = ::mmap(nullptr, hdr_region + size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED) die("mmap");
        map = static_cast<std::byte*>(p);
        header h;
        std::memcpy(&h, map, sizeof h);
        if (!h.valid(size)) die("s3raw header");
        off = h.offset;
        g0 = h.gen;
    }
    std::byte* body() override { return map + hdr_region; }
    uint64_t size() const override { return sz; }
    uint64_t offset() const override { return off; }
    uint64_t gen() const override { return g0; }
    void commit(uint64_t g) override {
        set_phase(ph_hdr_write);
        header h = make_header(g, off, sz, 0);
        std::memcpy(map, &h, sizeof h);
        set_phase(ph_msync);
        if (::msync(map, hdr_region + sz, MS_SYNC) != 0) die("msync");
        set_phase(ph_sync);
        if (m != syncm::msync) fd_sync(fd, m);
        g0 = g;
    }
};

using bip_file = boost::interprocess::basic_managed_mapped_file<
    char, boost::interprocess::rbtree_best_fit<boost::interprocess::null_mutex_family, segment::void_ptr, 16>,
    boost::interprocess::null_index>;
static_assert(std::is_same_v<bip_file::segment_manager, segment::heap::segment_manager>,
              "the mapped file must hold the workload's own heap");

struct s3bip_engine : engine {
    std::unique_ptr<bip_file> mf;
    int fd = -1;
    syncm m;
    bip_meta meta{};
    std::byte* seg = nullptr;
    uint64_t g0 = 0;
    s3bip_engine(const std::string& dir, uint64_t size, syncm mode) : m(mode) {
        namespace bi = boost::interprocess;
        if (!exists(p_image(dir))) {
            std::string tmp = p_image(dir) + ".tmp";
            ::unlink(tmp.c_str());
            {
                bip_file f(bi::create_only, tmp.c_str(), hdr_region + size);
                auto* sm = f.get_segment_manager();
                void* raw = f.allocate_aligned(sizeof(segment::nodes), alignof(segment::nodes));
                new (raw) segment::nodes(segment::alloc<segment::node>(sm));
                void* gp = f.allocate(8);
                std::memset(gp, 0, 8);
                auto* base = reinterpret_cast<std::byte*>(sm);
                meta = {static_cast<uint64_t>(base - static_cast<std::byte*>(f.get_address())), sm->get_size(),
                        static_cast<uint64_t>(static_cast<std::byte*>(raw) - base),
                        static_cast<uint64_t>(static_cast<std::byte*>(gp) - base)};
                if (::msync(f.get_address(), f.get_size(), MS_SYNC) != 0) die("msync create");
            }
            create_atomically(dir, p_meta(dir), fd_mode(m), [&](int f) { pwrite_all(f, &meta, sizeof meta, 0); });
            int t = ::open(tmp.c_str(), O_RDWR);
            fd_sync(t, fd_mode(m));
            ::close(t);
            if (::rename(tmp.c_str(), p_image(dir).c_str()) != 0) die("rename");
            fsync_dir(dir);
        }
        int mfd = ::open(p_meta(dir).c_str(), O_RDONLY);
        if (mfd < 0) die("open meta");
        pread_all(mfd, &meta, sizeof meta, 0);
        ::close(mfd);
        mf = std::make_unique<bip_file>(bi::open_only, p_image(dir).c_str());
        seg = reinterpret_cast<std::byte*>(mf->get_segment_manager());
        if (mf->get_segment_manager()->get_size() != meta.seg_size) die("s3bip size");
        std::memcpy(&g0, seg + meta.gen_offset, 8);
        fd = ::open(p_image(dir).c_str(), O_RDWR);
        if (fd < 0) die("open image");
    }
    std::byte* body() override { return seg; }
    uint64_t size() const override { return meta.seg_size; }
    uint64_t offset() const override { return meta.offset; }
    uint64_t gen() const override { return g0; }
    void commit(uint64_t g) override {
        set_phase(ph_hdr_write);
        std::memcpy(seg + meta.gen_offset, &g, 8);
        set_phase(ph_msync);
        if (m == syncm::bipflush) {
            if (!mf->flush()) die("flush");  // Boost: mapped_region::flush(0, 0, async = true), MS_ASYNC
        } else {
            if (::msync(mf->get_address(), mf->get_size(), MS_SYNC) != 0) die("msync");
            set_phase(ph_sync);
            if (m != syncm::msync) fd_sync(fd, m);
        }
        g0 = g;
    }
};

std::unique_ptr<engine> open_engine(strat s, syncm m, const std::string& dir, uint64_t size) {
    switch (s) {
    case strat::s0: return std::make_unique<s0_engine>(dir, size, m);
    case strat::s1: return std::make_unique<s1_engine>(dir, size, m, false);
    case strat::s1d: return std::make_unique<s1_engine>(dir, size, m, true);
    case strat::s2: return std::make_unique<s2_engine>(dir, size, m, false);
    case strat::s2mmap: return std::make_unique<s2_engine>(dir, size, m, true);
    case strat::s2p: return std::make_unique<s2_engine>(dir, size, m, false, true);
    case strat::s3raw: return std::make_unique<s3raw_engine>(dir, size, m);
    case strat::s3bip: return std::make_unique<s3bip_engine>(dir, size, m);
    case strat::tear: break;
    }
    return nullptr;
}

// --- modes ---------------------------------------------------------------------------------------

int bench(strat s, syncm m, const std::string& dir, uint64_t size, uint64_t commits) {
    g_status = &scratch_status;
    for (const char* f : {"/image", "/log", "/meta", "/image.tmp"}) ::unlink((dir + f).c_str());
    auto e = open_engine(s, m, dir, size);
    uint64_t g = e->gen();
    constexpr uint64_t warm = 20;
    for (uint64_t i = 0; i < warm; ++i) {
        ++g;
        segment::turn(e->body(), e->size(), e->offset(), g);
        e->commit(g);
    }
    std::vector<double> us;
    us.reserve(commits);
    uint64_t app0 = e->app_bytes, os0 = os_bytes_written();
    g_diff_ns = 0;
    for (uint64_t i = 0; i < commits; ++i) {
        ++g;
        segment::turn(e->body(), e->size(), e->offset(), g);
        auto t0 = std::chrono::steady_clock::now();
        e->commit(g);
        auto t1 = std::chrono::steady_clock::now();
        us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    uint64_t app = e->app_bytes - app0, os = os_bytes_written() - os0;
    std::vector<double> sorted = us;
    std::sort(sorted.begin(), sorted.end());
    auto pct = [&](double q) { return sorted[std::min<size_t>(sorted.size() - 1, static_cast<size_t>(q * sorted.size()))]; };
    double mean = 0;
    for (double v : us) mean += v;
    mean /= static_cast<double>(us.size());
    std::printf("{\"median_us\":%.1f,\"p99_us\":%.1f,\"max_us\":%.1f,\"mean_us\":%.1f,\"app_bytes_per_commit\":%llu,"
                "\"os_bytes_per_commit\":%llu,\"diff_us_mean\":%.1f}\n",
                pct(0.5), pct(0.99), sorted.back(), mean, (unsigned long long)(app / commits),
                (unsigned long long)(os / commits), static_cast<double>(g_diff_ns) / 1000.0 / static_cast<double>(commits));
    e.reset();
    return 0;
}

int write_loop(strat s, syncm m, const std::string& dir, uint64_t size) {
    g_status = map_status(dir);
    set_phase(ph_start);
    if (s == strat::tear) {
        std::string path = p_image(dir);
        int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0) die("open tear");
        std::byte* buf = aligned_buffer(size);
        for (uint64_t k = g_status->reported + 1;; ++k) {
            std::memset(buf, static_cast<int>(k % 251 + 1), size);
            set_phase(ph_write);
            pwrite_all(fd, buf, size, 0);  // one call: the kernel takes all 64 MB at once
            set_phase(ph_report);
            g_status->reported = k;
            g_status->commits = g_status->commits + 1;
        }
    }
    auto e = open_engine(s, m, dir, size);
    uint64_t g = e->gen();
    // What recovery found on disk is committed, whether or not the previous writer reported it.
    if (g > g_status->reported) g_status->reported = g;
    for (;;) {
        set_phase(ph_turn);
        segment::turn(e->body(), e->size(), e->offset(), g + 1);
        e->commit(g + 1);
        set_phase(ph_report);
        g_status->reported = ++g;
        g_status->commits = g_status->commits + 1;
    }
}

// Verifies the recovered image in a child process: a torn heap may crash or loop in check_sanity.
// Exit 0: equals_replay holds and the bytes are those of a replay; 1: equals_replay fails; 4:
// equals_replay holds but the bytes differ (heap metadata half-updated: a latent corruption).
int verify_in_child(strat s, const recovered& r, const bip_meta& meta) {
    pid_t pid = ::fork();
    if (pid == 0) {
        ::alarm(60);
        std::byte* scratch = aligned_buffer(r.size);
        bool ok = segment::equals_replay(r.body, r.size, r.offset, r.gen, scratch);
        if (!ok) std::_Exit(1);
        if (s == strat::s3bip) {
            // The byte replay of a mapped-file heap: the same allocations managed_mapped_file made.
            namespace bi = boost::interprocess;
            std::memset(scratch, 0, r.size);
            {
                segment::heap h(bi::create_only, scratch, r.size);
                void* raw = h.allocate_aligned(sizeof(segment::nodes), alignof(segment::nodes));
                new (raw) segment::nodes(segment::alloc<segment::node>(h.get_segment_manager()));
                void* gp = h.allocate(8);
                std::memcpy(gp, &r.gen, 8);
            }
            for (uint64_t t = 1; t <= r.gen; ++t) segment::turn(scratch, r.size, meta.offset, t);
        }
        std::_Exit(std::memcmp(r.body, scratch, r.size) == 0 ? 0 : 4);
    }
    int st = 0;
    ::waitpid(pid, &st, 0);
    if (WIFEXITED(st)) return WEXITSTATUS(st);  // 0 replays, 1 does not
    return 3;                                   // crashed or timed out on a torn heap
}

int check(strat s, const std::string& dir, uint64_t size) {
    status* st = map_status(dir);
    uint64_t reported = st->reported, ph = st->phase;
    const char* verdict = "consistent";
    std::string why;
    uint64_t gen = 0;
    if (s == strat::tear) {
        int fd = ::open(p_image(dir).c_str(), O_RDONLY);
        std::byte* buf = aligned_buffer(size);
        uint64_t got = fd >= 0 ? pread_all(fd, buf, size, 0) : 0;
        if (fd >= 0) ::close(fd);
        uint64_t switches = 0;
        for (uint64_t i = 1; i < got; ++i) switches += buf[i] != buf[i - 1];
        auto first = got ? static_cast<unsigned>(buf[0]) : 0u, last = got ? static_cast<unsigned>(buf[got - 1]) : 0u;
        if (switches > 0) {
            verdict = "torn";
            uint64_t at = 0;
            while (at + 1 < got && buf[at + 1] == buf[0]) ++at;
            why = "one pwrite split: first " + std::to_string(at + 1) + " bytes new";
        }
        std::printf("{\"verdict\":\"%s\",\"gen\":%u,\"last\":%u,\"reported\":%llu,\"phase\":\"%s\",\"why\":\"%s\"}\n", verdict,
                    first, last, (unsigned long long)reported, phase_name(ph), why.c_str());
        return 0;
    }
    recovered r = recover(s, dir, size);
    if (!r.present) {
        verdict = reported == 0 ? "none" : "lost";
        why = "no file";
    } else if (!r.ok) {
        verdict = "torn";
        why = "no valid generation: " + r.note;
    } else {
        gen = r.gen;
        bip_meta meta{};
        if (s == strat::s3bip) {
            int mfd = ::open(p_meta(dir).c_str(), O_RDONLY);
            pread_all(mfd, &meta, sizeof meta, 0);
            ::close(mfd);
        }
        int v = verify_in_child(s, r, meta);
        if (v == 4) {
            verdict = "latent";
            why = "equals_replay holds, bytes differ from the replay";
        } else if (v != 0) {
            verdict = "torn";
            why = v == 1 ? "does not replay" : "checker crashed on the heap";
        } else if (gen < reported) {
            verdict = "lost";
        } else if (gen > reported + 1) {
            verdict = "ahead";
        }
    }
    std::printf("{\"verdict\":\"%s\",\"gen\":%llu,\"reported\":%llu,\"phase\":\"%s\",\"why\":\"%s\"}\n", verdict,
                (unsigned long long)gen, (unsigned long long)reported, phase_name(ph), why.c_str());
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: seg bench|write|check|fnv ...\n");
        return 2;
    }
    std::string_view mode = argv[1];
    if (mode == "fnv") {  // throughput of the core's digest against fast_hash, 64 MB
        uint64_t n = 64ull << 20;
        std::byte* b = aligned_buffer(n);
        for (uint64_t i = 0; i < n; ++i) b[i] = static_cast<std::byte>(i * 131);
        auto t0 = std::chrono::steady_clock::now();
        volatile uint64_t a = segment::digest(b, n);
        auto t1 = std::chrono::steady_clock::now();
        volatile uint64_t c = image_digest(b, n);
        auto t2 = std::chrono::steady_clock::now();
        (void)a;
        (void)c;
        std::printf("segment::digest 64MB %.1f ms, page-hash digest 64MB %.1f ms\n",
                    std::chrono::duration<double, std::milli>(t1 - t0).count(),
                    std::chrono::duration<double, std::milli>(t2 - t1).count());
        return 0;
    }
    if (mode == "bench" && argc == 7) {
        return bench(parse_strat(argv[2]), parse_sync(argv[3]), argv[4], std::strtoull(argv[5], nullptr, 10) << 20,
                     std::strtoull(argv[6], nullptr, 10));
    }
    if (mode == "write" && argc == 6) {
        return write_loop(parse_strat(argv[2]), parse_sync(argv[3]), argv[4], std::strtoull(argv[5], nullptr, 10) << 20);
    }
    if (mode == "check" && argc == 5) {
        return check(parse_strat(argv[2]), argv[3], std::strtoull(argv[4], nullptr, 10) << 20);
    }
    std::fprintf(stderr, "bad arguments\n");
    return 2;
}
