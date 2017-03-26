/**** N-queens since 2003-10   by Kenji KISE ****/
/**** N-queens Simple Version in C           ****/
/************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/************************************************/
#define NAME "qn24b base"
#define VER  "version 1.0.1 2004-09-02"
#define MAX 29 /** 32 is a real max! **/
#define MIN 2

/************************************************/
typedef struct array_t{
  unsigned int cdt; /* candidates        */
  unsigned int col; /* column            */
  unsigned int pos; /* positive diagonal */
  unsigned int neg; /* negative diagonal */
} array;

/** N-queens kernel                            **/
/** n: problem size, h: height, r: row         **/
/************************************************/
long long qn(int n, int h, int r, array *a){
  long long answers = 0;

  for(;;){
    if(r){
      int lsb = (-r) & r;
      a[h+1].cdt = (       r & ~lsb);
      a[h+1].col = (a[h].col & ~lsb);
      a[h+1].pos = (a[h].pos |  lsb) << 1;
      a[h+1].neg = (a[h].neg |  lsb) >> 1;
      
      r = a[h+1].col & ~(a[h+1].pos | a[h+1].neg);
      h++;
    }
    else{
      r = a[h].cdt;
      h--;

      if(h==0) break;
      if(h==n) answers++;
    }
  }
  return answers;
}

/** function to return the time                **/
/************************************************/
long long get_time(){
  struct timeval  tp;
  struct timezone tz;
  gettimeofday(&tp, &tz);
  return tp.tv_sec * 1000000ull + tp.tv_usec;
}

/** print the answer                           **/
/************************************************/
void print_result(int n, long long usec, 
		  long long solution){
  int i;
  static long long answer[30] = {
    0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 
    2680, 14200, 73712, 365596, 2279184,
    14772512, 95815104, 666090624ull, 
    4968057848ull, 39029188884ull, 
    314666222712ull, 2691008701644ull,
    24233937684440ull,
    0,0,0,0,0,0
  };
  
  for(i=0;i<45;i++) printf("="); printf("\n");
  printf("%s %s\n", NAME, VER);
  printf("problem size n        : %d\n", n);
  printf("total   solutions     : %lld\n", 
	 solution);
  printf("correct solutions     : %lld\n", 
	 answer[n]);
  printf("million solutions/sec : %.3f\n", 
	 (double)solution/usec);
  printf("elapsed time (sec)    : %.3f\n",
         usec/1000000.0);
  if(solution!=answer[n])
    printf(" ### Wrong answer\n");
  for(i=0;i<45;i++) printf("="); printf("\n");
  fflush(stdout);
}

/** set the problem size                       **/
/************************************************/
int set_problem_size(int argc, char** argv){
  int n; /* problem size */
  printf("%s %s\n", NAME, VER);

  if(argc!=2){
    printf("Usage: %s n\n", argv[0]);
    exit(0);
  }
  n = atoi(argv[1]);
  if(MIN>n || MAX<n){
    printf("board size range: %d-%d\n",MIN, MAX);
    exit(0);
  }
  return n;
}

/** main function                              **/
/************************************************/
int main(int argc, char *argv[]){
  int i;
  int n    = set_problem_size(argc, argv);
  array *a = calloc(MAX, sizeof(array)); 
  long long usec    = get_time();
  long long answers = 0;
  
  for(i=0; i<(n/2+n%2); i++){
    long long ret;
    int h = 1;      /* height or level  */
    int r = 1 << i; /* candidate vector */
    a[h].col = (1<<n)-1;
    a[h].pos = 0;
    a[h].neg = 0;

    ret = qn(n, h, r, a); /* kernel loop */

    answers += ret;
    if(i!=n/2) answers += ret;
  }

  print_result(n, get_time()-usec, answers);
  return 0;
}
/************************************************/
