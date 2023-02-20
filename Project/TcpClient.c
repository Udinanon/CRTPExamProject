#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>

#define FALSE 0
#define TRUE 1

#define BUFFER_SIZE 8

// Funcion to setup TCP Server 
int sd, imageSize;
struct sockaddr_in sin;

void* receive_data();
void send_string(char* string);

int setup_client() {
  char hostname[100] = "127.0.0.1";
  int port = 11111;
  struct hostent *hp;
  printf("Connecting to %s:%d\n", hostname, port);

  /* Resolve the passed name and store the resulting long representation
     in the struct hostent variable */
  if ((hp = gethostbyname(hostname)) == 0) {
    perror("gethostbyname");
    exit(1);
  }
  /* fill in the socket structure with host information */
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
  sin.sin_port = htons(port);
  /* create a new socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  return 0;
}

void connect_server(){
  /* connect the socket to the port and host
   specified in struct sockaddr_in */
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    perror("connect");
    exit(1);
  }
  char *text = "alive";
  send_string(text);

  char *answer;
  answer = receive_data();
  printf("Received from Server: %s\n", answer);

  imageSize = *(int *)receive_data();
  printf("Received imagesize: %d\n", imageSize);
}

// Send routine to quickly pass strings
void send_string(char* string) {
  /* Send the answer back */
  int len = strlen(string);
  /* Convert to network byte order */
  unsigned int netLen = htonl(len);
  /* Send answer character length */
  if (send(sd, &netLen, sizeof(netLen), 0) == -1)
    perror("ERROR SENDING MSG LEN");
  /* Send answer characters */
  char string2[len];
  strcpy(string2, string); 
  if (send(sd, string2, len, 0) == -1)
    perror("ERROR SENDING MSG");
}

/* Receive routine: use recv to receive from socket and manage
   the fact that recv may return after having read less bytes than
   the passed buffer size
   In most cases recv will read ALL requested bytes, and the loop body
   will be executed once. This is not however guaranteed and must
   be handled by the user program. The routine returns 0 upon
   successful completion, -1 otherwise */
static int receive(int sd, char *retBuf, int size) {
  int totSize, currSize;
  totSize = 0;
  while (totSize < size) {
    currSize = recv(sd, &retBuf[totSize], size - totSize, 0);
    if (currSize <= 0)
      /* An error occurred */
      return -1;
    totSize += currSize;
  }
  return 0;
}

void* receive_data(){
  unsigned int netLen;
  int len;
  void* message;
  if (receive(sd, (char *)&netLen, sizeof(netLen))) {
    perror("recv");
    exit(0);
  }
  /* Convert from Network byte order */
  len = ntohl(netLen);
  /* Allocate and receive the answer */
  message = malloc(len + 1);
  if (receive(sd, message, len)) {
    perror("recv");
    exit(1);
  }
  //message[len] = 0;
  return message;
}

// Thread and buffer functions and objects

typedef struct Image {
  void *data;
  unsigned int datasize;
  char filename[100];
} Image;

typedef struct BufferData {
  int readIdx;
  int writeIdx;
  Image buffer[BUFFER_SIZE];
  sem_t mutexSem;
  sem_t dataAvailableSem;
  sem_t roomAvailableSem;
} BufferData;


BufferData* sharedBuf;

void file_writer(){
  Image image;
  printf("file_writer here!\n");
  while (1) {
    /* Wait for availability of at least one data slot */
    sem_wait(&sharedBuf->dataAvailableSem);
    /* Enter critical section */
    sem_wait(&sharedBuf->mutexSem);

    //copy shared Image object
    memcpy(&image, &sharedBuf->buffer[sharedBuf->readIdx], sizeof(Image));
    // allocate memory for frame data in process
    image.data = malloc(image.datasize);
    // copy frame data
    memcpy(image.data, sharedBuf->buffer[sharedBuf->readIdx].data, image.datasize);
    /* Update read index */
    sharedBuf->readIdx = (sharedBuf->readIdx + 1) % BUFFER_SIZE;
    printf("Got new image to write\n");
    printf("Image %p, Size %d, name %s\n", image.data, image.datasize, image.filename);
    /* Signal that a new empty slot is available */
    sem_post(&sharedBuf->roomAvailableSem);
    /* Exit critical section */
    sem_post(&sharedBuf->mutexSem);
    printf("WRiter reliquishes critial section\n");
    
    // write image data to disk
    char ext[] = ".jpeg";
    strcat(image.filename, ext); 
    //asprintf(filename, "%s.jpg", image.filename);
    printf("%s\n", image.filename);
    FILE *file = fopen(image.filename, "wb");
   // FILE *file = fopen("test.jpg", "wb");
    printf("%s\n", image.filename);
    fwrite(image.data, image.datasize, 1, file);
    // free frame data memory
    printf("%s\n", image.filename);
  }
  printf("Filewriter done!\n");
}

void client(){

  // Beginning of Client core code
  int stopped = FALSE;
  
  char* filename;
  void* image_data;
  printf("Client starting!\n");
  
  while (!stopped) {

    // receive all data from server
    filename = receive_data();
    printf("Filename received: %s\n", filename);
    image_data = malloc(imageSize);
    image_data = receive_data();
    printf("Received from Server: %p\n", image_data);

    /* Wait for availability of at least one empty slot */
    sem_wait(&sharedBuf->roomAvailableSem);
    /* Enter critical section */
    sem_wait(&sharedBuf->mutexSem);
    FILE *file = fopen("test.jpg", "wb");
    
    fwrite(image_data, imageSize, 1, file);
    fclose(file);
    /* Write data item */
    Image *bufferImg = &sharedBuf->buffer[sharedBuf->writeIdx];
     memcpy(bufferImg->data, image_data, imageSize);
    printf("Saved image %p to buffer!\n", image_data);
    strcpy(bufferImg->filename, filename);
    bufferImg->datasize = imageSize;
    /* Update write index */
    sharedBuf->writeIdx = (sharedBuf->writeIdx + 1) % BUFFER_SIZE;
    /* Signal that a new data slot is available */
    sem_post(&sharedBuf->dataAvailableSem);
    /* Exit critical section */
    sem_post(&sharedBuf->mutexSem);
  }
  printf("Client process done!\n");
  /* Close the socket */
  close(sd);
}


int memId;
void create_shared_memory_segment(){
  //Create shared memory segment for the buffer object
  memId = shmget(IPC_PRIVATE, sizeof(BufferData), 0666 | IPC_CREAT);
  if (memId == -1) {
    perror("Error in shmget");
    exit(0);
  }
  sharedBuf = (BufferData* )shmat(memId, NULL, 0);
  if (sharedBuf == (void *)-1) {
    perror("Error in shmat");
    exit(0);
  }
  for (int i = 0; i < BUFFER_SIZE; i++){
    // create shared memory segment for images in buffer
    memId = shmget(IPC_PRIVATE, imageSize, 0666 | IPC_CREAT);
    if (memId == -1) {
      perror("Error in shmget");
      exit(0);
    }
    sharedBuf->buffer[i].data = shmat(memId, NULL, 0);
    if (sharedBuf->buffer[i].data == (void *)-1) {
      perror("Error in shmat");
      exit(0);
    }
  }
}

void cleanup_shared_memory(){
  //TODO
}

int main(int argc, char **argv) {
  setup_client(); // the TCP server is setup for connection

  pid_t pids[3];
  connect_server(); // connects to server, gives us info such as imageSize
  create_shared_memory_segment();


  /* Initialize buffer indexes */
  sharedBuf->readIdx = 0;
  sharedBuf->writeIdx = 0;
  /* Initialize semaphores. Initial value is 1 for mutexSem,
     0 for dataAvailableSem (no filled slots initially available)
     and BUFFER_SIZE for roomAvailableSem (all slots are
     initially free). The second argument specifies
     that the semaphore is shared among processes */
  sem_init(&sharedBuf->mutexSem, 1, 1);
  sem_init(&sharedBuf->dataAvailableSem, 1, 0);
  sem_init(&sharedBuf->roomAvailableSem, 1, BUFFER_SIZE);

  /* Launch producer process */
  pids[0] = fork();
  if (pids[0] == 0) {
    // Client

    client();
    exit(0);
  }
/* Launch consumer processes */
  pids[1] = fork();
  if (pids[1] == 0) {
    file_writer();
    exit(0);
  }
  /* Wait process termination */
  for (int i = 0; i <= 2; i++) {
    waitpid(pids[i], NULL, 0);
  }
  //cleanup_shared_memory();
  return 0;
}
