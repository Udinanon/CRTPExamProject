#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <sched.h> // sched_setaffinity

pthread_mutex_t mutex;

pthread_cond_t roomAvailable, dataAvailable;
#define BUFFER_SIZE 1000
#define MAX_THREADS 10
#define HISTORY_LEN 10000
#define PRODUCER_DELAY 10000

int buffer[ BUFFER_SIZE];

int readIdx = 0;
int writeIdx = 0;
int finished = 0;

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
    int item;
    // SET Affinity
    pthread_t thread_id = pthread_self();
    cpu_set_t *cpuset = (cpu_set_t *)arg;
    
    int status;
    status = pthread_setaffinity_np(thread_id, sizeof(cpu_set_t), cpuset);
    if (status) {
        perror("pthread_setaffinity_np");
        exit(EXIT_FAILURE);
    }
    
    // CHECK CPU LOCATION //
    status = pthread_getaffinity_np(thread_id, sizeof(cpu_set_t), cpuset);
    if (status) {
        perror("pthread_getaffinity_np");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(& mutex);
    printf("CONSUMER deployed in CPU:");
    for (int j = 0; j < CPU_SETSIZE; j++)
        if (CPU_ISSET(j, cpuset)) printf(" %d", j);
    printf("\n");
    pthread_mutex_unlock(&mutex);

    for(;;)
    {
        pthread_mutex_lock(& mutex);
        while(!finished && readIdx == writeIdx) {
            pthread_cond_wait(&dataAvailable, &mutex);
        }
        if(finished && readIdx == writeIdx) { pthread_mutex_unlock(& mutex); return NULL; }
        item = buffer[readIdx];
        readIdx = (readIdx + 1)% BUFFER_SIZE;
        pthread_cond_signal(&roomAvailable);
        pthread_mutex_unlock(&mutex);

        // simulating a complex operation with 1us average computation time
        // wait_us(1);
        random_wait_ns(2000);
        clock_gettime(CLOCK_REALTIME, &receiveTimes[item]);
     }
   return NULL;
}


static void *producer(void *arg)
{
    int item = 0, i;
    // SET Affinity
    pthread_t thread_id = pthread_self();
    cpu_set_t *cpuset = (cpu_set_t *)arg;
    
    int status;
    status = pthread_setaffinity_np(thread_id, sizeof(cpu_set_t), cpuset);
    if (status) {
        perror("pthread_setaffinity_np");
        exit(EXIT_FAILURE);
    }
    
    // CHECK CPU LOCATION //
    status = pthread_getaffinity_np(thread_id, sizeof(cpu_set_t), cpuset);
    if (status) {
        perror("pthread_getaffinity_np");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(& mutex);
    printf("PRODUCER deployed in CPU:");
    for (int j = 0; j < CPU_SETSIZE; j++)
        if (CPU_ISSET(j, cpuset)) printf(" %d", j);
    printf("\n");
    pthread_mutex_unlock(&mutex);

    // PRODUCER CODE //
    for(i = 0; i < HISTORY_LEN; i++)
    {
        clock_gettime(CLOCK_REALTIME, &sendTimes[item]);
        pthread_mutex_lock(& mutex);
        while( (writeIdx+1)%BUFFER_SIZE == readIdx ) {
            pthread_cond_wait(&roomAvailable, &mutex);
        }
        buffer[writeIdx] = item;
        writeIdx = (writeIdx + 1)% BUFFER_SIZE;
        pthread_cond_signal(&dataAvailable);
        pthread_mutex_unlock(& mutex);
        item++;
        if(PRODUCER_DELAY) usleep(PRODUCER_DELAY);
    }
    finished = 1; // lock is not acutally needed because it is only read by consumer
    pthread_mutex_lock(& mutex);
    pthread_cond_broadcast(&dataAvailable);
    pthread_mutex_unlock(& mutex);
    return NULL;
}



int main(int argc , char *args[])
{
    pthread_t threads[ MAX_THREADS];
    int nConsumers, producerAffinity;
    int i;
    struct timespec t_start, t_end;
    if(argc != 3)
    {
        printf("Usage: prodcons <numConsumers> <producer_CPU_id>\n");
        exit(0);
    }
    sscanf(args[1], "%d", &nConsumers);
    sscanf(args[2], "%d", &producerAffinity);
 
    // cpu_set_t: This data set is a bitset where each bit represents a CPU.
    cpu_set_t producer_set, consumer_set;
    
    // CPU_ZERO: This macro initializes the CPU set set to be the empty set.
    CPU_ZERO(&producer_set);
    CPU_ZERO(&consumer_set);
    
    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    if(producerAffinity >= number_of_processors)
    {
        printf("Producer processor id must be in the range: [0-%d]\n", number_of_processors-1);
        exit(EXIT_FAILURE);
    }

    // CPU_SET: This macro adds cpu to the CPU set set.
    CPU_SET(producerAffinity, &producer_set);
    for(i=0; i<number_of_processors; ++i)
        if( !CPU_ISSET(i, &producer_set) )
            CPU_SET(i, &consumer_set);

    pthread_mutex_init(& mutex , NULL);
    pthread_cond_init(& dataAvailable , NULL);
    pthread_cond_init(& roomAvailable , NULL);
    clock_gettime(CLOCK_REALTIME, &t_start);
    pthread_create(&threads[0], NULL , producer , &producer_set); // send cpuset as param
    for(i = 0; i < nConsumers; i++) {
        pthread_create(& threads[i+1], NULL , consumer , &consumer_set);
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

