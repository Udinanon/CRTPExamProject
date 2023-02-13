#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

#define BUFFER_SIZE 5
#define NITER 30

// Shared by the producer and consumer thread
int buffer[BUFFER_SIZE];
int in = 0;    //next free position 
int out = 0;   //first full position
int finished = 0;

// Forward declarations
void* producer();
void* consumer();

int main(int argc, char**argv){
   
   int nConsumers;
   if(argc != 2)
   {
       printf("Usage: prodcons <numConsumers >\n");
       exit(0);
   }
   sscanf(argv[1], "%d", &nConsumers);

   #pragma omp parallel
   #pragma omp single
   {
      #pragma omp task
      producer();
      
      for(int i=0; i<nConsumers; ++i) {
        #pragma omp task
        consumer();
      }
   }
   printf("\n");
   
   return 0;
}



void *producer(){
   for (int i=0; i< NITER; i++) {
   
      // Do nothing until a slot is available
      while (((in + 1) % BUFFER_SIZE) == out) 
         usleep(100);

      #pragma omp critical
      {
        buffer[in] = i;
        in = (in + 1) % BUFFER_SIZE;
      }
      printf("+");
      fflush(stdout);
   }
   finished = 1;
   return NULL;
}

void *consumer(){
   int next_consumed; 
   
   while( 1 ){ 
      // Do nothing if the buffer is empty and the producer did not finish yet
      while (!finished && in == out) 
         usleep(100);
      
      // If we finished and all elements has been consumed then exit
      if(finished && in==out) {
         return NULL;
      }

      #pragma omp critical
      {
        next_consumed = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
      }
      printf(" %d ",next_consumed); fflush(stdout);
      
   } 
   return NULL;
}



