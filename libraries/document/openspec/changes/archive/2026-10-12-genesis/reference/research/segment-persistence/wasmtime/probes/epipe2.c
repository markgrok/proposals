// Like epipe.c but ignores write errors, as a program that never checks printf would, and stops
// after 1000 lines; reports to stderr how long the writes after the failure took.
#include <stdio.h>
#include <time.h>
static double ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
int main(void) {
    double start = ms(); long failed = -1;
    for (long i = 0; i < 1000; ++i) {
        if ((printf("line %ld\n", i) < 0 || fflush(stdout) != 0) && failed < 0) { failed = i; start = ms(); fprintf(stderr, "first failure at %ld\n", i); }
        if (failed >= 0 && i % 100 == 0) fprintf(stderr, "i=%ld %.0f ms after the failure\n", i, ms() - start);
    }
    fprintf(stderr, "done %.0f ms after the failure\n", ms() - start);
}
