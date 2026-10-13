// Reference on this machine's disk: latency of 1 MB pwrite plus each macOS sync primitive, to read
// the browsers' flush() latencies against (fsync, fcntl F_BARRIERFSYNC, fcntl F_FULLFSYNC).
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static int cmp(const void* a, const void* b) { double x = *(const double*)a, y = *(const double*)b; return x < y ? -1 : x > y; }
int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "fsync_ref.bin";
    size_t size = 1 << 20; int n = 200;
    char* buf = malloc(size);
    const char* names[] = {"write only", "fsync", "F_BARRIERFSYNC", "F_FULLFSYNC"};
    for (int k = 0; k < 4; ++k) {
        int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
        double lat[200];
        for (int i = 0; i < n; ++i) {
            memset(buf, i + k, size);
            double t = now();
            pwrite(fd, buf, size, 0);
            if (k == 1) fsync(fd);
            if (k == 2) fcntl(fd, F_BARRIERFSYNC);
            if (k == 3) fcntl(fd, F_FULLFSYNC);
            lat[i] = now() - t;
        }
        qsort(lat, n, sizeof lat[0], cmp);
        printf("%-15s 1 MB: median %.3f ms, p99 %.3f ms\n", names[k], lat[n / 2], lat[n * 99 / 100]);
        close(fd);
    }
    unlink(path);
    return 0;
}
