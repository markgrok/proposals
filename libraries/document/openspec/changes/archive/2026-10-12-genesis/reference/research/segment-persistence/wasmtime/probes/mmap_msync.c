// Whether wasi-libc has msync, and what a MAP_SHARED mapping does after msync and munmap.
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(void) {
    int fd = open("msync.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || ftruncate(fd, 4096) != 0) { printf("setup: %s\n", strerror(errno)); return 1; }
    char* p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { printf("mmap: %s\n", strerror(errno)); return 1; }
    strcpy(p, "through the mapping");
    int r = msync(p, 4096, MS_SYNC);
    printf("msync: %d (%s)\n", r, r == 0 ? "ok" : strerror(errno));
    char buf[32] = {0}; pread(fd, buf, 19, 0); printf("file after msync: '%s'\n", buf);
    munmap(p, 4096);
    memset(buf, 0, sizeof buf); pread(fd, buf, 19, 0); printf("file after munmap: '%s'\n", buf);
    return 0;
}
