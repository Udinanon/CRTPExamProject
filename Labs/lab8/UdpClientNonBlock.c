// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <fcntl.h> 
#include <assert.h>
#include <errno.h> // errno and EAGAIN

#define PORT	8080
#define MAXLINE 1024



// Driver code
int main() {
	int sockfd;
	char buffer[MAXLINE];
	char *hello = "Hello from client";
	struct sockaddr_in	 servaddr;

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	int flags = fcntl(sockfd,F_GETFL,0);
	assert(flags != -1);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	memset(&servaddr, 0, sizeof(servaddr));
	
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = INADDR_ANY;
	
	int n, len;
	
	sendto(sockfd, (const char *)hello, strlen(hello),
		MSG_CONFIRM, (const struct sockaddr *) &servaddr,
			sizeof(servaddr));
	printf("Hello message sent.\n");
		
	// this should go through without waiting for reply
	do {
		n = recvfrom(sockfd, (char *)buffer, MAXLINE,
				0, (struct sockaddr *) &servaddr,
				&len); 
		// n = recvfrom(sockfd, (char *)buffer, MAXLINE,
		// 		MSG_DONTWAIT, (struct sockaddr *) &servaddr,
		// 		&len); 
	}
	while(n<0 && errno == EAGAIN);
	
	buffer[n] = '\0';
	printf("Server : %s\n", buffer);

	close(sockfd);
	return 0;
}
