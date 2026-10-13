#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(void) {
    int fd = open("seg.bin", O_RDWR | O_CREAT, 0644);
    if (fd < 0) { printf("open: %s\n", strerror(errno)); return 1; }
    if (ftruncate(fd, 4096) != 0) printf("ftruncate: %s\n", strerror(errno));
    char* p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { printf("mmap MAP_SHARED rw: %s\n", strerror(errno)); }
    else { strcpy(p, "written through the mapping"); munmap(p, 4096); }
    char* q = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    printf("mmap MAP_PRIVATE rw: %s\n", q == MAP_FAILED ? strerror(errno) : "ok");
    char buf[32] = {0}; pread(fd, buf, 27, 0); printf("file holds: '%s'\n", buf);
    return 0;
}
