// The same measurement as native_sync.c inside wasmtime: fsync is wasi:filesystem sync, fdatasync sync-data.
// fcntl(F_FULLFSYNC), 200 times each, to compare with what wasi:filesystem's sync costs.
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static double ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static int cmp(const void* a, const void* b) { double x = *(const double*)a, y = *(const double*)b; return x < y ? -1 : x > y; }
int main(int argc, char** argv) {
    int fd = open(argc > 1 ? argv[1] : "native.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    char page[4096]; memset(page, 7, sizeof page);
    const char* names[] = {"fsync", "fdatasync", "F_FULLFSYNC"};
    for (int k = 0; k < NK; ++k) {
        double v[200];
        for (int i = 0; i < 200; ++i) {
            page[0] = (char)i;
            double t = ms();
            pwrite(fd, page, sizeof page, (off_t)(i % 64) * 4096);
            int r = k == 0 ? fsync(fd) : k == 1 ? fdatasync(fd) : -1;
            v[i] = ms() - t;
            if (r != 0) { perror(names[k]); return 1; }
        }
        qsort(v, 200, sizeof v[0], cmp);
        printf("%-12s median %.3f ms p99 %.3f ms\n", names[k], v[100], v[198]);
    }
    return 0;
}
