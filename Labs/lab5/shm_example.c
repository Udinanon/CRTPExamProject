#include <sys/ipc.h>
#include <sys/shm.h>
# include <sys/types.h>

/* The numeric identifier of the shared memory segment
   the same value must be used by all processes sharing the segment */
#define MY_SHARED_ID 1
 // ...
key_t key;        //Identifier to be passed to shmget()
int memId;        //The id returned by shmget() to be passed to shmat()
void *startAddr;  //The start address of the shared memory segment
 // ...
/* Creation of the key. Routine ftok() function uses the identity
   of the file path passed as first argument (here /tmp is used, but it
   may refer to any existing file in the system) and the least
   significant 8 bits of the second argument */
key_t key = ftok("/tmp", MY_SHARED_ID);

/* First try to create a new memory segment. Flags define exclusive
   creation, i.e. if the shared memory segment already exists, shmget()
   returns with an error */
  memId = shmget(key, size, IPC_CREAT | IPC_EXCL);
  if(memId == -1)
/* Exclusive creation failed, the segment has been already created by
   another process */
  {
/* shmget() is called again without the CREATE option */
    memId = shmget(key, size, 0);
  }
/* If memId == -1 here, an error occurred in the creation of
   the shared memory segment */
  if(memId != -1)
  {
  /* Routine shmat() maps the shared memory segment to a range
     of virtual addresses */
    startAddr = (char *)shmat(memId, NULL, 0666);
  /* From now, memory region pointed by startAddr
     is the shared segment */
// ...
