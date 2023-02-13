#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* Port number used in the application */
#define PORT 4444
/* Multicast address */
#define GROUP "225.0.0.37"
/* Sender main program: get the string from the command argument */
main(int argc, char *argv[])
{
  struct sockaddr_in addr;
  int sd;
  char *message;
/* Get message string */
  if(argc < 2)
  {
    printf("Usage: sendUdp <message>\n");
    exit(0);
  }
  message = argv[1];
/* Create the socket. The second argument specifies that
   this is an UDP socket */
  if ((sd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
  {
    perror("socket");
    exit(0);
  }
/* Set up destination address: same as TCP/IP example */
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(GROUP);
  addr.sin_port=htons(PORT);
/* Send the message */
  if (sendto(sd,message,strlen(message),0,
	     (struct sockaddr *) &addr, sizeof(addr)) < 0)
  {
    perror("sendto");
    exit(0);
  }
/* Close the socket */
  close(sd);
}
