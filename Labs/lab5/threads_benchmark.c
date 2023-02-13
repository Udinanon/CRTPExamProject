#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include <time.h>
#include <errno.h>

#define MAX_THREADS 256
#define ROWS 10000
#define COLS 10000

/* Arguments exchanged with threads */
struct argument{
  int startRow;
  int nRows;
  long partialSum;
  struct timespec t_start, t_end;
} threadArgs[MAX_THREADS];

/* Matrix pointer: it will be dynamically allocated */
long *bigMatrix;

/* Thread routine: make the summation of all the elements of the
   assigned matrix rows */
static void *threadRoutine(void *arg)
{
  int i, j;
/* Type-cast passed pointer to expected structure
   containing the start row, the number of rows to be summed
   and the return sum argument */  
  struct argument *currArg = (struct argument *)arg;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &currArg->t_start);
  long sum = 0;
  for(i = 0; i < currArg->nRows; i++)
    for(j = 0; j < COLS; j++)
      sum += bigMatrix[(currArg->startRow + i) * COLS + j];
  currArg->partialSum = sum;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &currArg->t_end);
  return NULL;
}

static inline long _abs(long x) { return x>0?x:-x; }

static void fillMatrix() {
  int i,j;
  for(i=0; i<ROWS;  ++i) 
    for(j=0; j<COLS;  ++j) 
      bigMatrix[i*COLS+j] = _abs((i+j)%512-256);
}


static int save_picture_pix(long *mat, const char *filename) {
  FILE *fs = fopen(filename, "w");
  if(!fs) {
    perror("could not open file");
    exit(1);
  }
  fprintf(fs, "%d %d\n", ROWS, COLS);
  int i,j;
  for(i=0; i<ROWS;  ++i) {
    for(j=0; j<COLS;  ++j) 
      fprintf(fs, "%d ", mat[i*COLS+j]);
    fprintf(fs, "\n");
  }
  fclose(fs);
}


double benchmark_sum_threads(int nThreads, int clock_mode) {

/* Array of thread identifiers */
  pthread_t threads[MAX_THREADS];
  long totalSum;
  int i, j, rowsPerThread, lastThreadRows;
  struct timespec t_start, t_end;
  double t_process_us;

/* If the number of rows cannot be divided exactly by the number of
   threads, let the last thread handle also the remaining rows */
  rowsPerThread = ROWS / nThreads;
  if(ROWS % nThreads == 0)
    lastThreadRows = rowsPerThread;
  else
    lastThreadRows = rowsPerThread + ROWS % nThreads;

/* Prepare arguments for threads */
  for(i = 0; i < nThreads; i++)
  {
/* Prepare Thread arguments */
    threadArgs[i].startRow = i*rowsPerThread;
    if(i == nThreads - 1)
      threadArgs[i].nRows = lastThreadRows;
    else
      threadArgs[i].nRows = rowsPerThread;
  }

  clock_gettime(clock_mode, &t_start);
 
/* Start the threads using default thread attributes */
  for(i = 0; i < nThreads; i++)
    pthread_create(&threads[i], NULL, threadRoutine, &threadArgs[i]);

/* Wait thread termination and use the corresponding
   sum value for the final summation */
  totalSum = 0;
  for(i = 0; i < nThreads; i++)
  {
    pthread_join(threads[i], NULL);
    totalSum += threadArgs[i].partialSum;
  }
  clock_gettime(clock_mode, &t_end);
  t_process_us = (t_end.tv_sec - t_start.tv_sec) * 1e6 +
                      (t_end.tv_nsec - t_start.tv_nsec) / 1e3;

  return t_process_us;
}



int main(int argc, char *args[])
{
  int nThreads;
/* Get the number of threads from command parameter */
  if(argc != 2)
  {
    printf("Usage: threads <numThreads>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nThreads);
  
  struct timespec t_dummy;
  int e = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t_dummy);
  if(e == ENOENT) {
    printf("Warning clock might return bogus results if a process is migrated to another CPU\n");
  }

/* Allocate  the matrix M */
  bigMatrix = malloc(ROWS*COLS*sizeof(long));

  fillMatrix();
  // save_picture_pix(bigMatrix, "bigMatrix.pix");

  FILE* bm = fopen("threads_benchmark.csv", "w");
  if(!bm) {
    perror("could not write benchmark.csv file");
    exit(1);
  }
  fprintf(bm, "nThreads,clock_realtime,clock_cputime\n");
  double t1,t2;
  for (int i=1; i<=nThreads; ++i) {
    t1 = benchmark_sum_threads(i, CLOCK_REALTIME);
    t2 = benchmark_sum_threads(i, CLOCK_PROCESS_CPUTIME_ID);
    printf("Elapsed time(us): %d, %f, %f\n", i, t1, t2);
    fprintf(bm, "%d,%f,%f\n", i, t1*1e-3, t2*1e-3);
  }

  fclose(bm);
  return 0;
}