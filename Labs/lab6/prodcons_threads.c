#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_THREADS 250

/* The mutex used to protect shared data */
pthread_mutex_t mutex;
/* Condition variables to signal availability
   of room and data in the buffer */
pthread_cond_t roomAvailable, dataAvailable;

#define BUFFER_SIZE 128
/* Shared data */
int buffer[BUFFER_SIZE];
/* readIdx is the index in the buffer of the next item to be retrieved */
int readIdx = 0;
/* writeIdx is the index in the buffer of the next item to be inserted */
int writeIdx = 0;
/* Buffer empty condition corresponds to readIdx == writeIdx. Buffer full
   condition corresponds to (writeIdx + 1)%BUFFER_SIZE == readIdx */

/* Consumer Code: the passed argument is not used */
static void *consumer(void *arg)
{
  int item;
  while(1)
  {
/* Enter critical section */
    pthread_mutex_lock(&mutex);
/* If the buffer is empty, wait for new data */
    while(readIdx == writeIdx)
    {
      pthread_cond_wait(&dataAvailable, &mutex);
    }
/* At this point data are available
   Get the item from the buffer */
    item = buffer[readIdx];
    readIdx = (readIdx + 1)%BUFFER_SIZE;
/* Signal availability of room in the buffer */
    pthread_cond_signal(&roomAvailable);
/* Exit critical section */
    pthread_mutex_unlock(&mutex);

 /* Consume the item and take actions (e.g. return)*/
   // ...
  }
  return NULL;
}
/* Producer code. Passed argument is not used */
static void *producer(void *arg)
{
  int item = 0;
  while(1)
  {
/* Produce a new item and take actions (e.g. return) */
    //  ...
/* Enter critical section */
    pthread_mutex_lock(&mutex);
/* Wait for room availability */
    while((writeIdx + 1)%BUFFER_SIZE == readIdx)
    {
      pthread_cond_wait(&roomAvailable, &mutex);
    }
/* At this point room is available
   Put the item in the buffer */
    buffer[writeIdx] = item;
    writeIdx = (writeIdx + 1)%BUFFER_SIZE;
/* Signal data avilability */
    pthread_cond_signal(&dataAvailable);
/* Exit critical section */
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main(int argc, char *args[])
{
  pthread_t threads[MAX_THREADS];
  int nConsumers;
  int i;
/* The number of consumer is passed as argument */
  if(argc != 2)
  {
    printf("Usage: prod_cons <numConsumers>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nConsumers);

/* Initialize mutex and condition variables */
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&dataAvailable, NULL);
  pthread_cond_init(&roomAvailable, NULL);

/* Create producer thread */
  pthread_create(&threads[0], NULL, producer, NULL);
/* Create consumer threads */
  for(i = 0; i < nConsumers; i++)
    pthread_create(&threads[i+1], NULL, consumer, NULL);

/* Wait termination of all threads */
  for(i = 0; i < nConsumers + 1; i++)
  {
    pthread_join(threads[i], NULL);
  }
  return 0;
}