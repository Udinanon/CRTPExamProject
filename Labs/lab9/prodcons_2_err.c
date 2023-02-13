#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

pthread_mutex_t mutex;

pthread_cond_t roomAvailable, dataAvailable;
#define BUFFER_SIZE 5
#define MAX_THREADS 10
#define HISTORY_LEN 10
int buffer[ BUFFER_SIZE];

int readIdx = 0;
int writeIdx = 0;
int finish = 0;

static struct timespec sendTimes[HISTORY_LEN];
static struct timespec receiveTimes[HISTORY_LEN];

static inline double get_us(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)*1E6 + (end.tv_nsec - start.tv_nsec)/1E3;
}

static inline double get_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)*1E3 + (end.tv_nsec - start.tv_nsec)/1E6;
}

static void random_wait_ns(int max_ns) {
    static struct timespec waitTime;
    int ns = rand() % max_ns;
    waitTime.tv_sec = 0;
    waitTime.tv_nsec = ns;
    nanosleep(&waitTime, NULL);
}

static void wait_us(int us) {
    static struct timespec waitTime;
    us %= 1000;
    waitTime.tv_sec = 0;
    waitTime.tv_nsec = us*1000;
    nanosleep(&waitTime, NULL);
}


static void *consumer(void *arg)
{
    int item, i;
    for(;;)
    {
        pthread_mutex_lock(& mutex);
        while(readIdx == writeIdx) {
            pthread_cond_wait(&dataAvailable, &mutex);
            if(finish) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }
        item = buffer[readIdx];
        readIdx = (readIdx + 1)% BUFFER_SIZE;
        pthread_cond_signal(&roomAvailable);
        // simulating a complex operation with 1us average computation time
        wait_us(1);
        printf("%d\n",item);
        pthread_mutex_unlock(&mutex);
        clock_gettime(CLOCK_REALTIME, &receiveTimes[item]);
     }
   return NULL;
}


static void *producer(void *arg)
{
    int item = 0, i;
    for(i = 0; i < HISTORY_LEN; i++)
    {
        clock_gettime(CLOCK_REALTIME, &sendTimes[item]);
        pthread_mutex_lock(& mutex);
        while( (writeIdx+1)%BUFFER_SIZE == readIdx ) {
            pthread_cond_wait(&roomAvailable, &mutex);
        }
        // printf("+");
        buffer[writeIdx] = item;
        writeIdx = (writeIdx + 1)% BUFFER_SIZE;
        pthread_cond_signal(&dataAvailable);
        pthread_mutex_unlock(& mutex);
        item++;
    }
    pthread_mutex_lock(& mutex);
    finish = 1;
    printf("fin.\n");
    pthread_cond_broadcast(&dataAvailable);
    pthread_mutex_unlock(& mutex);
    return NULL;
}



int main(int argc , char *args[])
{
    pthread_t threads[ MAX_THREADS];
    int nConsumers;
    int i;
    struct timespec t_start, t_end;
    if(argc != 2)
    {
        printf("Usage: prodcons <numConsumers >\n");
        exit(0);
    }
    sscanf(args[1], "%d", &nConsumers);
    pthread_mutex_init(& mutex , NULL);
    pthread_cond_init(& dataAvailable , NULL);
    pthread_cond_init(& roomAvailable , NULL);
    clock_gettime(CLOCK_REALTIME, &t_start);
    pthread_create(&threads[0], NULL , producer , NULL);
    for(i = 0; i < nConsumers; i++) {
        pthread_create(& threads[i+1], NULL , consumer , NULL);
    }
    for(i = 0; i < nConsumers + 1; i++) {
        pthread_join( threads[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &t_end);
    double avg = 0, dev = 0;
    for(i = 0; i < HISTORY_LEN; i++) {
       avg += get_ms(sendTimes[i], receiveTimes[i]);
    }
    avg /= HISTORY_LEN;
    for(i = 0; i < HISTORY_LEN; i++) {
        dev += pow(get_ms(sendTimes[i], receiveTimes[i]), 2);
    }
    dev /= HISTORY_LEN;
    dev = sqrt(dev);
    printf("Average Communication time: %.2f ms (Std. Dev: %.2f ms)\n", avg, dev);
    printf("Overall Communication time: %.2f ms\n", get_ms(t_start, t_end));
   return 0;
}

