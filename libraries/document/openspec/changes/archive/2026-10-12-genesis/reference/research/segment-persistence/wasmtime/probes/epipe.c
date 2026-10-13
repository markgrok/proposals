// Writes lines to stdout forever; run piped into `head -1` to see what a closed stdout does.
#include <stdio.h>
int main(void) {
    for (unsigned long i = 0;; ++i) {
        if (printf("line %lu\n", i) < 0 || fflush(stdout) != 0) { fprintf(stderr, "write failed at %lu\n", i); return 3; }
    }
}
