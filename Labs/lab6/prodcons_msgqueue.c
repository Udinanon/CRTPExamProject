#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAX_PROCESSES 256
/* The type of message */
#define PRODCONS_TYPE 1
/* Message structure definition */
struct msgbuf {
  long mtype;
  int item;
};
/* Message queue id */
int msgId;

/* Consumer routine */
static void consumer()
{
  int retSize;
  struct msgbuf msg;
  int item;
  while(1)
  {
/* Receive the message. msgrcv returns the size of the received message */
    retSize =  msgrcv(msgId, &msg, sizeof(int), PRODCONS_TYPE, 0);
    if(retSize == -1) //If Message reception failed
    {
      perror("error msgrcv");
      exit(0);
    }
    item = msg.item;
/* Consume data item */
    // ...
  }
}
/* Consumer routine */
static void producer()
{
  int item = 0;
  struct msgbuf msg;
  msg.mtype = PRODCONS_TYPE;
  while(1)
  {
/* produce data item */
    // ...
    msg.item = item;
    msgsnd(msgId, &msg, sizeof(int), 0);
  }
}
/* Main program. The number of consumer
   is passed as argument */
int main(int argc, char *args[])
{
  int i, nConsumers;
  pid_t pids[MAX_PROCESSES];
  if(argc != 2)
  {
    printf("Usage: prodcons  <nConsumers>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nConsumers);
/* Initialize message queue */
  msgId = msgget(IPC_PRIVATE, 0666);
  if(msgId == -1)
  {
    perror("msgget");
    exit(0);
  }
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