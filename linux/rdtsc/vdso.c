#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
static double ns(struct timespec a, struct timespec b){return (b.tv_sec-a.tv_sec)*1e9+(b.tv_nsec-a.tv_nsec);}
int main(int argc,char**argv){
  long n=1000000; struct timespec t,a,b; int mode=argc>1?atoi(argv[1]):2;
  if(mode&1){clock_gettime(CLOCK_MONOTONIC_RAW,&a);for(long i=0;i<n;i++)clock_gettime(CLOCK_MONOTONIC,&t);clock_gettime(CLOCK_MONOTONIC_RAW,&b);printf("vDSO   clock_gettime: %.1f ns/call\n",ns(a,b)/n);}
  if(mode&2){clock_gettime(CLOCK_MONOTONIC_RAW,&a);for(long i=0;i<n;i++)syscall(SYS_clock_gettime,CLOCK_MONOTONIC,&t);clock_gettime(CLOCK_MONOTONIC_RAW,&b);printf("syscall clock_gettime: %.1f ns/call\n",ns(a,b)/n);}
}
