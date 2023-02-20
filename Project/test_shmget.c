#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct SharedObj {
  char value[100];
  int num;
  int newId;
} SharedObj;

int main() {
  int memId;
  memId = shmget(IPC_PRIVATE, sizeof(SharedObj), SHM_R | SHM_W);
  SharedObj* obj = (SharedObj*)shmat(memId, NULL, 0);
  obj->num = 100;
  char name[] = "When knowledge is not liberating, the dream of the oppressed is to become the oppressor";
  memId = shmget(IPC_PRIVATE, sizeof(name), SHM_R | SHM_W);
  //char sharedName[100] = (char*)shmat(memId, NULL, 0);
  strcpy(obj->value, name);
  //= sharedName;
  //printf("name pointer %p, size %d, content %s\n", sharedName, sizeof(sharedName), sharedName);
  printf("SharedObj Pointer %p, value %d, name %s\n", obj, obj->num, obj->value);

  pid_t pids[3];
  pids[0] = fork();
  if (pids[0] == 0) {
    // Client
    printf("First process, PID: %d\n", pids[0]);
    //printf("name pointer %p, size %d, content %s\n", sharedName, sizeof(sharedName), sharedName);
    printf("SharedObj Pointer %p, value %d, name %s\n", obj, obj->num, obj->value);
    obj->value[3] = 'f'; 
    printf("SharedObj Pointer %p, value %d, name %s\n", obj, obj->num, obj->value);
    int id = shmget(IPC_PRIVATE, sizeof(SharedObj), SHM_R | SHM_W);
    obj->newId = id;
    SharedObj* obj2 = shmat(obj->newId, NULL, 0);
    obj2->num = 101;
    printf("NewID %d, pointer %p,  num %d\n", obj->newId, obj2, obj2->num);
    

    exit(0);
  }
  /* Launch consumer processes */
  pids[1] = fork();
  if (pids[1] == 0) {
    printf("second process, PID: %d\n", pids[1]);
    sleep(3);
    //printf("name pointer %p, size %d, content %s\n", sharedName, sizeof(sharedName), sharedName);
    printf("SharedObj Pointer %p, value %d, name %s\n", obj, obj->num, obj->value);
    printf("NewId %d\n", obj->newId);
    SharedObj* obj2 = shmat(obj->newId, NULL, 0);
    printf("NewID %d, pointer %p,  num %d\n", obj->newId, obj2, obj2->num);

    exit(0);
  }
  /* Wait process termination */
  for (int i = 0; i <= 2; i++) {
    waitpid(pids[i], NULL, 0);
  }
  return 0;
}