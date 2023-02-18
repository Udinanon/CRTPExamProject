#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

int sd;

int setup_client() {
  char hostname[100] = "127.0.0.1";
  int port = 11111;
  struct sockaddr_in sin;
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
  /* connect the socket to the port and host
     specified in struct sockaddr_in */
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    perror("connect");
    exit(1);
  }
}

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

char* receive_string(){
  unsigned int netLen;
  int len;
  char* message;
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
  message[len] = 0;
  return message;
}
/* Main client program. The IP address and the port number of
   the server are passed in the command line. After establishing
   a connection, the program will read commands from the terminal
   and send them to the server. The returned answer string is
   then printed. */
int main(int argc, char **argv) {
  int stopped = FALSE;
  char *command, *answer;
  int len;
  setup_client();
  char * text = "alive";
  send_string(text);
  answer = receive_string();
  printf("Received from Server: %s\n", answer);
  // Beginning of Clinet core code
  while (!stopped) {
    /* Get a string command from terminal */
    char* q = receive_string();
    printf("%s", q);
    char name[256];
    fgets(name, sizeof(name), stdin);
    send_string(name);
  }
  /* Close the socket */
  close(sd);
  return 0;
}
