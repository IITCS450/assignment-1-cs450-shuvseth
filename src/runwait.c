#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <cmd> [args]\n",a); exit(1);}
static double d(struct timespec a, struct timespec b){
 return (b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;}
int main(int c,char**v){

// check args
  if (c < 2) usage(v[0]);

  struct timespec t0, t1;
  
  timespec_get(&t0, TIME_UTC);
// fork+exec
  pid_t pid = fork();
  if (pid < 0) { perror("fork"); return 1; }
// child
  if (pid == 0) {
    execvp(v[1], &v[1]);
    
    perror("execvp");
    _exit(127);
  }
// parent
  int st = 0;
  if (waitpid(pid, &st, 0) < 0) { perror("waitpid"); return 1; }

  timespec_get(&t1, TIME_UTC);
  double elapsed = d(t0, t1);
// get exit code
  int exitcode = 1;
  if (WIFEXITED(st)) exitcode = WEXITSTATUS(st);
  else if (WIFSIGNALED(st)) exitcode = 128 + WTERMSIG(st);

  printf("pid=%d elapsed=%.3f exit=%d\n", (int)pid, elapsed, exitcode);


 return 0;
}

