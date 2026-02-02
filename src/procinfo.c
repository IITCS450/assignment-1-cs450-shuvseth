#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


static void usage(const char *a) {
    fprintf(stderr, "Usage: %s <pid>\n", a);
    exit(1);
}

static int isnum(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

/*
 * Parse /proc/<pid>/stat safely:
 * Format starts like:  pid (comm with spaces) state ppid ...
 */
static int read_stat(pid_t pid, char *state, int *ppid,
                     unsigned long long *utime_ticks,
                     unsigned long long *stime_ticks,
                     char *comm_out, size_t comm_sz) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[8192];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    // Find command name inside parentheses: ( ... )
    char *lp = strchr(line, '(');
    char *rp = strrchr(line, ')');
    if (!lp || !rp || rp < lp) return -1;

    // Extract comm
    size_t clen = (size_t)(rp - lp - 1);
    if (clen >= comm_sz) clen = comm_sz - 1;
    memcpy(comm_out, lp + 1, clen);
    comm_out[clen] = '\0';

    // After ") " comes: state ppid pgrp session ...
    // We'll tokenize and count fields starting at field #3 = state.
    char *after = rp + 2; // should point to state
    if (!after || !*after) return -1;

    char *save = NULL;
    int field = 3;  // state is field 3
    unsigned long long ut = 0, st = 0;
    int ppid_local = -1;
    char stch = '?';

    for (char *tok = strtok_r(after, " ", &save);
         tok != NULL;
         tok = strtok_r(NULL, " ", &save), field++) {

        if (field == 3) {
            stch = tok[0];
        } else if (field == 4) {
            ppid_local = atoi(tok);
        } else if (field == 14) {
            ut = strtoull(tok, NULL, 10);
        } else if (field == 15) {
            st = strtoull(tok, NULL, 10);
            break; // we have what we need
        }
    }

    if (ppid_local < 0) return -1;

    *state = stch;
    *ppid = ppid_local;
    *utime_ticks = ut;
    *stime_ticks = st;

    return 0;
}

static long read_vmrss_kb(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[1024];
    long kb = -1;

    while (fgets(line, sizeof(line), f)) {
        // Line looks like: "VmRSS:\t  4096 kB\n"
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // scan the first number after "VmRSS:"
            char *p = line + 6;
            while (*p && (*p == ' ' || *p == '\t')) p++;
            kb = strtol(p, NULL, 10);
            break;
        }
    }

    fclose(f);
    return kb;
}

int main(int c, char **v) {
    if (c != 2 || !isnum(v[1])) usage(v[0]);

    pid_t pid = (pid_t)atoi(v[1]);

    char state = '?';
    int ppid = -1;
    unsigned long long uticks = 0, sticks = 0;
    char comm[4096];
    comm[0] = '\0';

    if (read_stat(pid, &state, &ppid, &uticks, &sticks, comm, sizeof(comm)) != 0) {
        fprintf(stderr, "procinfo: could not read /proc/%d/stat\n", pid);
        return 1;
    }

    long rss_kb = read_vmrss_kb(pid);

    long hz = sysconf(_SC_CLK_TCK);
    if (hz <= 0) hz = 100;

    double u_sec = (double)uticks / (double)hz;
    double s_sec = (double)sticks / (double)hz;

    // Match the sample-style output
    printf("PID:%d\n", pid);
    printf("State:%c\n", state);
    printf("PPID:%d\n", ppid);
    printf("Cmd:%s\n", comm);

    // Sample shows "CPU:3 0.030" (first often looks integer-ish)
    printf("CPU:%g %.3f\n", u_sec, s_sec);

    if (rss_kb >= 0) printf("VmRSS:%ld\n", rss_kb);
    else             printf("VmRSS:%d\n", -1);

    return 0;
}
