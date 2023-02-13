#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h> // waitpid
#include <string.h>

#define BUFFER_SIZE 1024
#define MAX_PROCESSES 10
#define HISTORY_LEN 5000

#define PORT 1234



static struct timespec sendTimes[HISTORY_LEN];
static struct timespec receiveTimes[HISTORY_LEN];
int pos = 0;

static inline double get_us(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)*1E6 + (end.tv_nsec - start.tv_nsec)/1E3;
}

static inline double get_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)*1E3 + (end.tv_nsec - start.tv_nsec)/1E6;
}

static void random_wait_ns(int max_ns) {
    static struct timespec waitTime;
    int ns = rand() % max_ns;
    waitTime.tv_sec = 0;
    waitTime.tv_nsec = ns;
    nanosleep(&waitTime, NULL);
}

static void wait_us(int us) {
    static struct timespec waitTime;
    us %= 1000;
    waitTime.tv_sec = 0;
    waitTime.tv_nsec = us*1000;
    nanosleep(&waitTime, NULL);
}


static int consumer()
{
    char buffer[BUFFER_SIZE];
    int sockfd;
    int client_number = -1;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_aton("127.0.0.1", &servaddr.sin_addr);
    //servaddr.sin_addr.s_addr = INADDR_ANY;

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 5000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    int n, len;
    sendto(sockfd, "HELO", 4, MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                   sizeof(servaddr));
    printf("CONSUMER: Sent HELLO\n");
    struct timespec send_time, receive_time;
    while(1) {
        n = recv(sockfd, (char *)buffer, BUFFER_SIZE*sizeof(int), MSG_WAITALL);
        //if(n > 0) printf("CONSUMER: received message %d\n", n);
        if(n==4 && strncmp(buffer,"STOP",4) == 0)
        {
            printf("CONSUMER: Received STOP\n");
            break;
        }
        if(n==sizeof(struct timespec)) { 
            memcpy(&send_time, buffer, sizeof(struct timespec));
            clock_gettime(CLOCK_REALTIME, &receive_time);
            sendTimes[pos] = send_time;
            receiveTimes[pos] = receive_time;
            pos++;
        }
        
    }
    close(sockfd);
    return 0;
}

static int bindProducer()
{
   struct sockaddr_in servaddr;
   int sockfd;
   if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
       
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 5000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // Filling server information
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
       
    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, 
            sizeof(servaddr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

static int producer(int sockfd, int expectedClients)
{
    int active_packet_id[MAX_PROCESSES];
    int readIdx = 0;
    int writeIdx = 0;
    int last_pos = 0;
    
    struct sockaddr_in cliaddr;
    struct sockaddr_in active_clients[MAX_PROCESSES];
    int active_clients_status[MAX_PROCESSES];
    int active_clients_len = 0;
    int next_client_id = 0;

       
    // Creating socket file descriptor
        
    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(active_clients_status, 0, sizeof(active_clients_status));
    int id, n, len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    printf("Waiting clients....\n");
    while(active_clients_len < expectedClients)
    {
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 
                   MSG_DONTWAIT, ( struct sockaddr *) &cliaddr,
                    &len);
        if(n>0) {
                printf("hello %d LenAddr: %d, Port: %d Addr: %s\n", active_clients_len, len,  cliaddr.sin_port, inet_ntoa(cliaddr.sin_addr));
           // HELLO FROM CLIENT
            if( n==4 && strncmp(buffer,"HELO",4)==0) {
                active_clients[active_clients_len] = cliaddr;
                 active_clients_len++;
            }
        }
    }
    printf("All Clients Connected\n");
    while(last_pos < HISTORY_LEN || active_clients_len < expectedClients) {
        if(last_pos < HISTORY_LEN && active_clients_len) {
            cliaddr = active_clients[next_client_id++];
            next_client_id %= active_clients_len;
            struct timespec send_time;
            clock_gettime(CLOCK_REALTIME, &send_time);
            sendto(sockfd, (char *)&send_time, sizeof(struct timespec),
                   MSG_CONFIRM, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
           // printf("PRODUCER: sent message %d\n", last_pos);
            last_pos++;
        }
        usleep(10);
    }
    printf("stopping %d clients\n", active_clients_len);
    for(int i=0; i<active_clients_len; ++i) {
        sendto(sockfd, "STOP", 4, MSG_CONFIRM, (struct sockaddr *)&active_clients[i], sizeof(active_clients[i]));
    }
    return 0;
}





int main(int argc , char *args[])
{
    int nConsumers;
    int i, id;
    int sockfd;
    pid_t pids[MAX_PROCESSES];
    struct timespec t_start, t_end;
    if(argc != 2)
    {
        printf("Usage: prodcons <numConsumers >\n");
        exit(0);
    }
    sscanf(args[1], "%d", &nConsumers);
    if(nConsumers > MAX_PROCESSES)
    {
        printf("Maximun allowed number of consumers: %d\n", MAX_PROCESSES);
        exit(0);
    }
    
    sockfd = bindProducer();
    clock_gettime(CLOCK_REALTIME, &t_start);
    /* Spawn child processes */
    for(id = 0; id < nConsumers; ++id)
    {
        pids[id] = fork();
        if(pids[id] == 0)
        {
          consumer();
          double avg = 0, dev = 0;
          for(i = 0; i < pos; i++) {
              avg += get_ms(sendTimes[i], receiveTimes[i]);
          }
          avg /= pos;
          for(i = 0; i < pos; i++) {
              dev += pow(get_ms(sendTimes[i], receiveTimes[i]), 2);
          }
          dev /= pos;
          dev = sqrt(dev);
          printf("Average Communication time: %.2f ms (Std. Dev: %.2f ms)\n", avg, dev);
  
          exit(0);
        }
    }
    // PARENT PROCESS
    producer(sockfd, nConsumers);
    printf("USCITO DA PRODUCER\n");
    for(id = 0; id < nConsumers; id++)
    {
        /* Wait child process termination */
        waitpid(pids[id], NULL, 0);
        
    }

    clock_gettime(CLOCK_REALTIME, &t_end);
    printf("Overall Communication time: %.2f ms\n", get_ms(t_start, t_end));
   return 0;
}

