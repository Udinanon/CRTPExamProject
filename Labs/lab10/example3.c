#define _GNU_SOURCE // sched_getcpu(3) is glibc-specific (see the man page)

#include <stdio.h>
#include <sched.h>
#include <omp.h>

int main(int argc, char **argv)
{
    int a[100000];
    int printed_once = 0;

    printf("Threads available: %d\n", omp_get_max_threads());
    #pragma omp parallel private(printed_once)
    // this create a team of all available cores that expands in parallel tasks
    // in the following
    {
        #pragma omp for nowait
        // parallelize this for loop with all avaliable threads
        for (int i = 0; i < 100000; i++) {
            if (!printed_once) {
                int thread_num = omp_get_thread_num();
                int team_num = omp_get_team_num();
                int cpu_num = sched_getcpu();
                printf("cpu: %d - team: %d - thread: %d - i=%d\n", 
                       cpu_num, team_num, thread_num, i);
                printed_once = 1;
            }
            a[i] = 2 * i;
        }
        #pragma omp barrier
        // after the for has finished one of the threads executes this code.
        #pragma omp single
        {
            int thread_num = omp_get_thread_num();
            int team_num = omp_get_team_num();
            int cpu_num = sched_getcpu();
            printf("cpu: %d - team: %d - thread: %d - single process\n", 
                   cpu_num, thread_num);
        }

    }
    return 0;
}
