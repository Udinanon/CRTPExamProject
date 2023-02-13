#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_PROCESSES 256
#define BUFFER_SIZE 128
/* Shared Buffer, indexes and semaphores are held in shared memory
   readIdx is the index in the buffer of the next item to be retrieved
   writeIdx is the index in the buffer of the next item to be inserted
   Buffer empty condition corresponds to readIdx == writeIdx
   Buffer full condition corresponds to
   (writeIdx + 1)%BUFFER_SIZE == readIdx)
   Semaphores used for synchronization:
   mutexSem is used to protect the critical section
   dataAvailableSem is used to wait for data avilability
   roomAvailableSem is used to wait for room abailable in the buffer */

struct BufferData {
  int readIdx;
  int writeIdx;
  int buffer[BUFFER_SIZE];
  sem_t mutexSem;
  sem_t dataAvailableSem;
  sem_t roomAvailableSem;
};

struct BufferData *sharedBuf;

/* Consumer routine */
static void consumer() {
  int item;
  while(1) {
/* Wait for availability of at least one data slot */
    sem_wait(&sharedBuf->dataAvailableSem);
/* Enter critical section */
    sem_wait(&sharedBuf->mutexSem);
/* Get data item */
    item = sharedBuf->buffer[sharedBuf->readIdx];
/* Update read index */
    sharedBuf->readIdx = (sharedBuf->readIdx + 1)%BUFFER_SIZE;
/* Signal that a new empty slot is available */
    sem_post(&sharedBuf->roomAvailableSem);
/* Exit critical section */
    sem_post(&sharedBuf->mutexSem);
/* Consume data item and take actions (e.g return)*/
   // ...
  }
}
/* producer routine */
static void producer()
{
  int item = 0;
  while(1)
  {
/* Produce data item and take actions (e.g. return)*/
    // ...
/* Wait for availability of at least one empty slot */
    sem_wait(&sharedBuf->roomAvailableSem);
/* Enter critical section */
    sem_wait(&sharedBuf->mutexSem);
/* Write data item */
    sharedBuf->buffer[sharedBuf->writeIdx] = item;
/* Update write index */
    sharedBuf->writeIdx = (sharedBuf->writeIdx + 1)%BUFFER_SIZE;
/* Signal that a new data slot is available */
    sem_post(&sharedBuf->dataAvailableSem);
/* Exit critical section */
    sem_post(&sharedBuf->mutexSem);
  }
}
/* Main program: the passed argument specifies the number
   of consumers */
int main(int argc, char *args[])
{
  int memId;
  int i, nConsumers;
  pid_t pids[MAX_PROCESSES];
  if(argc != 2)
  {
    printf("Usage: prodcons  <numProcesses>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nConsumers);
/* Set-up shared memory */
  memId = shmget(IPC_PRIVATE, sizeof(struct BufferData), SHM_R | SHM_W);
  if(memId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
  sharedBuf = shmat(memId, NULL, 0);
  if(sharedBuf == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }
/* Initialize buffer indexes */
  sharedBuf->readIdx = 0;
  sharedBuf->writeIdx = 0;
/* Initialize semaphores. Initial value is 1 for mutexSem,
   0 for dataAvailableSem (no filled slots initially available)
   and BUFFER_SIZE for roomAvailableSem (all slots are
   initially free). The second argument specifies
   that the semaphore is shared among processes */
  sem_init(&sharedBuf->mutexSem, 1, 1);
  sem_init(&sharedBuf->dataAvailableSem, 1, 0);
  sem_init(&sharedBuf->roomAvailableSem, 1, BUFFER_SIZE);

/* Launch producer process */
  pids[0] = fork();
  if(pids[0] == 0)
  {
/* Child process */
    producer();
    exit(0);
  }
/* Launch consumer processes */
  for(i = 0; i < nConsumers; i++)
  {
    pids[i+1] = fork();
    if(pids[i+1] == 0)
    {
      consumer();
      exit(0);
    }
  }
/* Wait process termination */
  for(i = 0; i <= nConsumers; i++)
  {
    waitpid(pids[i], NULL, 0);
  }
  return 0;
}