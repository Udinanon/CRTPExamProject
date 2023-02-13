#include <stdio.h>
#include <omp.h>
#include <unistd.h>

int fibonacci(int x) {
    if(x<2) { usleep(500); return 1; }
    int n1,n2;
    #pragma omp task shared(n1)
    n1 = fibonacci(x-1);
    #pragma omp task shared(n2)
    n2 = fibonacci(x-2);
    #pragma omp taskwait
    return n1+n2;
}




int main(void) {

    printf("\nTasks are created, one per execution unit in a parallel section\n[ ");
    #pragma omp parallel
    {
        {   // a set of N tasks to print A
            int id = omp_get_team_num();
            printf(" A%d ", id);
        }

        {   // a set of N tasks to print B
            int id = omp_get_team_num();
            printf(" B%d ", id);
        }
    }
    printf("]\n");


    printf("\nExample of creating two tasks\n[ ");
    #pragma omp parallel
    #pragma omp single
    {
        #pragma omp task
        {
            for(int i=0; i<10; i++)
            { usleep(100); printf("+"); }
        }
        #pragma omp task
        {
            for(int i=0; i<10; i++)
            { usleep(100); printf("o"); }
        }
    }
    printf(" ]\n");

    printf("\nExample of fibonacci using tasks\n[ ");
    #pragma omp parallel
    #pragma omp single
    {
        printf("fibonacci(10) = %d", fibonacci(10));
    }
    printf(" ]\n");


    return 0;
}



