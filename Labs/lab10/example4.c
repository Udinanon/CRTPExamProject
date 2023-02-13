#include <stdio.h>
#include <omp.h>

void function1() {
    printf("function one\n");
    int i,n=10;
    #pragma omp for nowait
    for(i=0; i<n; i++)
        printf("[%d] ",i);
    #pragma omp barrier
    printf("\n");
}

void function2() {
    #pragma omp single
    printf("function two\n");
}

int main(void)
{
    function1(); // sequential call

    #pragma omp parallel num_threads(2) 
    {
        function1();
        function2();
    }
    return 0;
}

