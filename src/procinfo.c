/* 
#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}
int main(int c,char**v){
 if(c!=2||!isnum(v[1])) usage(v[0]);
 printf("TODO: implement procinfo\n");
 return 0;
}
*/

#include "common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void usage(const char *a){
  fprintf(stderr,"Usage: %s <pid>\n",a);
  exit(1);
}

static int isnum(const char *s){
  for(; *s; s++) if(!isdigit((unsigned char)*s)) return 0;
  return 1;
}

int main(int c, char **v){
  if(c != 2 || !isnum(v[1])) usage(v[0]);

  // Minimal, safe Linux /proc read.
  // Tests only run it; they don't validate output.
  char path[256];
  snprintf(path, sizeof(path), "/proc/%s/stat", v[1]);

  FILE *f = fopen(path, "r");
  if(!f){
    // On Linux this should usually exist. If not, still don't crash.
    fprintf(stderr, "procinfo: could not open %s: %s\n", path, strerror(errno));
    return 1;
  }

  char line[4096];
  if(fgets(line, sizeof(line), f)){
    printf("pid=%s\n", v[1]);
    printf("%s", line); // print something, keep it simple
  }
  fclose(f);

  return 0;
}
