#define _POSIX_C_SOURCE 200809L

#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *a) {
    fprintf(stderr, "Usage: %s <cmd> [args]\n", a);
    exit(1);
}

static double diff_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int c, char **v) {
    if (c < 2) usage(v[0]);

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        perror("clock_gettime");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(v[1], &v[1]);
        fprintf(stderr, "execvp failed: %s\n", strerror(errno));
        _exit(127);
    }

    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        return 1;
    }

    double elapsed = diff_sec(t0, t1);

    int exitcode = 1;
    if (WIFEXITED(st)) exitcode = WEXITSTATUS(st);
    else if (WIFSIGNALED(st)) exitcode = 128 + WTERMSIG(st);

    // Match sample format
    printf("pid=%d elapsed=%.3f exit=%d\n", pid, elapsed, exitcode);

    return 0;
}
