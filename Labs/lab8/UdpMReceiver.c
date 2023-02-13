#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 4444
#define GROUP "225.0.0.37"

/* Maximum dimension of the receiver buffer */
#define BUFSIZE 256

/* Receiver main program. No arguments are passed in the command line. */
int main(int argc, char *argv[])
{
  struct sockaddr_in addr;
  int sd, nbytes,addrLen;
  struct ip_mreq mreq;
  char msgBuf[BUFSIZE];

/* Create a UDP socket */
  if ((sd=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    exit(0);
  }

  int optval = 1; // true
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
   { 
      perror("options");
      exit(1);
   }
/* Set up receiver address. Same as in the TCP/IP example. */
  memset(&addr,0,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);
/* Bind to receiver address */
  if (bind(sd,(struct sockaddr *) &addr,sizeof(addr)) < 0)
  {
    perror("bind");
    exit(0);
  }
/* Use setsockopt() to request that the receiver join a multicast group */
  mreq.imr_multiaddr.s_addr = inet_addr(GROUP);
  mreq.imr_interface.s_addr = INADDR_ANY;
  if (setsockopt(sd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)
  {
    perror("setsockopt");
    exit(0);
  }
/* Now the receiver belongs to the multicast group:
   start accepting datagrams in a loop */
  for(;;)
  {
    addrLen = sizeof(addr);
/* Receive the datagram. The sender address is returned in addr */
    if ((nbytes = recvfrom(sd, msgBuf, BUFSIZE, 0,
     (struct sockaddr *) &addr,&addrLen)) < 0)
    {
      perror("recvfrom");
      exit(0);
    }
/* Insert terminator */
    msgBuf[nbytes] = 0;
    printf("%s\n", msgBuf);
  }
  
  return 0;
}
