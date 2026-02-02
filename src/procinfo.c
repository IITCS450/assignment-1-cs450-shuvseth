#include "common.h"              
#include <ctype.h>          
#include <errno.h>              
#include <stdio.h>              
#include <stdlib.h>              
#include <string.h>              
#include <unistd.h>  
static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}
int main(int c,char**v){
 if(c!=2||!isnum(v[1])) usage(v[0]);
 
{
  int pid = 0;
  if (sscanf(v[1], "%d", &pid) != 1) usage(v[0]);

// read /proc/[pid]/stat
  char path[256];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);

  FILE *fs = fopen(path, "r");
  if (!fs) { perror("fopen"); return 1; }

  char buf[8192];
  if (!fgets(buf, sizeof(buf), fs)) { fclose(fs); return 1; }
  fclose(fs);

  
  int pid_i = 0;
  char comm[256];
  char state = '?';
  int ppid = -1;

  int pgrp=0, session=0, tty_nr=0, tpgid=0;
  unsigned flags=0;
  unsigned long minflt=0, cminflt=0, majflt=0, cmajflt=0;
  unsigned long utime=0, stime=0;
  long cutime=0, cstime=0, priority=0, nicev=0, num_threads=0, itrealvalue=0;
  unsigned long long starttime=0;
  unsigned long vsize=0;
  long rss=0;
  unsigned long rsslim=0, startcode=0, endcode=0, startstack=0, kstkesp=0, kstkeip=0;
  unsigned long signal=0, blocked=0, sigignore=0, sigcatch=0, wchan=0, nswap=0, cnswap=0;
  int exit_signal=0;
  int processor=0;
  unsigned rt_priority=0, policy=0;

  int n = sscanf(
    buf,
    "%d (%255[^)]) %c %d %d %d %d %d %u "
    "%lu %lu %lu %lu "
    "%lu %lu "
    "%ld %ld %ld %ld %ld %ld "
    "%llu %lu %ld "
    "%lu %lu %lu %lu %lu %lu "
    "%lu %lu %lu %lu %lu %lu %lu "
    "%d %d %u %u",
    &pid_i, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
    &minflt, &cminflt, &majflt, &cmajflt,
    &utime, &stime,
    &cutime, &cstime, &priority, &nicev, &num_threads, &itrealvalue,
    &starttime, &vsize, &rss,
    &rsslim, &startcode, &endcode, &startstack, &kstkesp, &kstkeip,
    &signal, &blocked, &sigignore, &sigcatch, &wchan, &nswap, &cnswap,
    &exit_signal, &processor, &rt_priority, &policy
  );

  if (n < 6) {
    fprintf(stderr, "procinfo: could not parse /proc/%d/stat\n", pid);
    return 1;
  }

// read command line from /proc/[pid]/cmdline   
  char cmd[256];
  cmd[0] = '\0';

  snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
  FILE *fc = fopen(path, "r");
  if (fc) {
    int ch, i = 0;
    while ((ch = fgetc(fc)) != EOF && ch != '\0' && i < (int)sizeof(cmd) - 1) {
      cmd[i++] = (char)ch;
    }
    cmd[i] = '\0';
    fclose(fc);
  }
  if (cmd[0] == '\0') {
    /* fallback */
    int i = 0;
    while (comm[i] && i < (int)sizeof(cmd) - 1) { cmd[i] = comm[i]; i++; }
    cmd[i] = '\0';
  }

// read VmRSS from /proc/[pid]/status
  long vmrss_kb = -1;
  snprintf(path, sizeof(path), "/proc/%d/status", pid);
  FILE *fstatus = fopen(path, "r");
  if (fstatus) {
    char line[512];
    while (fgets(line, sizeof(line), fstatus)) {
      if (line[0]=='V' && line[1]=='m' && line[2]=='R' && line[3]=='S' && line[4]=='S' && line[5]==':') {
        /* line like: VmRSS:    4096 kB */
        sscanf(line + 6, "%ld", &vmrss_kb);
        break;
      }
    }
    fclose(fstatus);
  }

 // compute CPU time in seconds 
  long ticks = sysconf(_SC_CLK_TCK);
  double cpu_sec = 0.0;
  if (ticks > 0) cpu_sec = (double)(utime + stime) / (double)ticks;

// print info   
  printf("PID:%d\n", pid_i);
  printf("State:%c\n", state);
  printf("PPID:%d\n", ppid);
  printf("Cmd:%s\n", cmd);
  printf("CPU:%d %.3f\n", processor, cpu_sec);
  if (vmrss_kb >= 0) printf("VmRSS:%ld\n", vmrss_kb);
  else printf("VmRSS:-1\n");
}


 return 0;
}

