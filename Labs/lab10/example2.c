#define _GNU_SOURCE // sched_getcpu(3) is glibc-specific (see the man page)

#include <stdio.h>
#include <sched.h>
#include <omp.h>

int main(int argc, char **argv)
{
    int a[100000];
    int printed_once = 0;

    #pragma omp parallel for private(printed_once)
    for (int i = 0; i < 100000; i++) {
        if (!printed_once) {
            int thread_num = omp_get_thread_num();
            int cpu_num = sched_getcpu();
            printf("cpu: %d - thread: %d - i=%d\n", cpu_num, thread_num, i);
            printed_once = 1;
        }
        a[i] = 2 * i;
    }
    return 0;
}
