#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <unistd.h> // fork()

#define MAX_PROCESSES 256
#define ROWS 10000L
#define COLS 10000L

#define PAUSE sleep(60);

/* Arguments exchanged with child processes */
struct argument{
  int startRow;
  int nRows;
  long partialSum;
};
/* The shared memory contains the arguments exchanged between parent
   and child processes and is pointer by processArgs */
struct argument *processArgs;

/* Matrix pointer: it will be dynamically allocated */
long *bigMatrix;

/* The current process index, incremented by the parent process before
   every fork() call. */
int currProcessIdx;

/* Child process routine: make the summation of all the elements of the
   assigned matrix rows. */
static void processRoutine()
{
  int i, j;
  long sum = 0;
  printf("I'm working on process %d: pid = %d\n", currProcessIdx, getpid());
/* processArgs is the pointer to the shared memory inherited by the
   parent process. processArg[currProcessIdx] is the argument
   structure specific to the child process */
  for(i = 0; i < processArgs[currProcessIdx].nRows; i++)
    for(j = 0; j < COLS; j++)
      sum += bigMatrix[(processArgs[currProcessIdx].startRow + i) * COLS + j];
/* Report the computed sum into the argument structure */
  processArgs[currProcessIdx].partialSum = sum;
  PAUSE
}


int main(int argc, char *args[])
{
  int memId;
  long totalSum;
  int i, j, nProcesses, rowsPerProcess, lastProcessRows;
/* Array of process identifiers used by parent process in the wait cycle */
  pid_t pids[MAX_PROCESSES];

/* Get the number of processes from command parameter */
  if(argc != 2)
  {
    printf("Usage: processs <numProcesses>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nProcesses);

  printf("Parent pid: %d\n", getpid());
/* Create a shared memory segment to contain the argument structures
   for all child processes. Set Read/Write permission in flags argument. */
  memId = shmget(IPC_PRIVATE, nProcesses * sizeof(struct argument), 0666);
  if(memId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
/* Attach the shared memory segment. Child processes will inherit the
   shared segment already attached */
  processArgs = shmat(memId, NULL, 0);
  if(processArgs == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }

/* Allocate  the matrix M */
  bigMatrix = malloc(ROWS*COLS*sizeof(long));
/* Fill the matrix with some values */
  // ...

/* If the number of rows cannot be divided exactly by the number of
   processs, let the last thread handle also the remaining rows  */
  rowsPerProcess = ROWS / nProcesses;
  if(ROWS % nProcesses == 0)
    lastProcessRows = rowsPerProcess;
  else
    lastProcessRows = rowsPerProcess + ROWS % nProcesses;

/* Prepare arguments for processes */
  for(i = 0; i < nProcesses; i++)
  {
    processArgs[i].startRow = i*rowsPerProcess;
    if(i == nProcesses - 1)
      processArgs[i].nRows = lastProcessRows;
    else
      processArgs[i].nRows = rowsPerProcess;
  }

/* Spawn child processes */
  for(currProcessIdx = 0; currProcessIdx < nProcesses; currProcessIdx++)
  {
    pids[currProcessIdx] = fork();
    if(pids[currProcessIdx] == 0)
    {
/* This is the child process which inherits a private copy of all
   the parent process memory except for the region pointed by
   processArgs which is shared with the parent process */
      processRoutine();
/* After computing partial sum the child process exits */
      exit(0);
    }
  }
/* Wait termination of child processes and perform final summation */
  totalSum = 0;
  for(currProcessIdx = 0; currProcessIdx < nProcesses; currProcessIdx++)
  {
/* Wait child process termination */
    waitpid(pids[currProcessIdx], NULL, 0);
    totalSum += processArgs[currProcessIdx].partialSum;
  }

}
