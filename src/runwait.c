/* 
#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void usage(const char *a){fprintf(stderr,"Usage: %s <cmd> [args]\n",a); exit(1);}
static double d(struct timespec a, struct timespec b){
 return (b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;}
int main(int c,char**v){
 return 0;
}

*/
#define _POSIX_C_SOURCE 200809L

#include "common.h"

#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static void usage(const char *a){
  fprintf(stderr,"Usage: %s <cmd> [args]\n",a);
  exit(1);
}

static double d(struct timespec a, struct timespec b){
  return (b.tv_sec-a.tv_sec) + (b.tv_nsec-a.tv_nsec)/1e9;
}

int main(int c, char **v){
  if(c < 2) usage(v[0]);

  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);

  pid_t pid = fork();
  if(pid < 0){
    perror("fork");
    return 1;
  }

  if(pid == 0){
    execvp(v[1], &v[1]);
    fprintf(stderr, "execvp failed: %s\n", strerror(errno));
    _exit(127);
  }

  int st = 0;
  if(waitpid(pid, &st, 0) < 0){
    perror("waitpid");
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &t1);
  double elapsed = d(t0, t1);

  if(WIFEXITED(st)){
    // test greps for exit=0
    printf("pid=%d exit=%d time=%.6f\n", pid, WEXITSTATUS(st), elapsed);
  } else if(WIFSIGNALED(st)){
    printf("pid=%d exit=%d signal=%d time=%.6f\n", pid, 128 + WTERMSIG(st), WTERMSIG(st), elapsed);
  } else {
    printf("pid=%d exit=%d time=%.6f\n", pid, 1, elapsed);
  }

  return 0;
}
