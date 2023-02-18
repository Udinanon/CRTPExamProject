#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <asm/unistd.h>
#include <poll.h>
#define MAX_FORMAT 100

static int receive(int sd, char *retBuf, int size)
{
    int totSize, currSize;
    totSize = 0;
    while (totSize < size)
    {
        currSize = recv(sd, &retBuf[totSize], size - totSize, 0);
        if (currSize <= 0)
            /* An error occurred */
            return -1;
        totSize += currSize;
    }
    return 0;
}

/* Handle an established  connection
   routine receive is listed in the previous example */
static void handleConnection(int currSd)
{
    unsigned int netLen;
    int len;
    int exit_status = 0;
    char *command, *answer;
    int counter = 2;
    while (counter > 0)
    {
        /* Get the command string length
           If receive fails, the client most likely exited */
        if (receive(currSd, (char *)&netLen, sizeof(netLen)))
            break;
        /* Convert from network byte order */
        len = ntohl(netLen);
        command = malloc(len + 1);
        /* Get the command and write terminator */
        receive(currSd, command, len);
        command[len] = 0;
        /* Execute the command and get the answer character string */
        if (strcmp(command, "help") == 0)
            answer = strdup(
                "server is active.\n\n"
                "    commands:\n"
                "       help: print this help\n"
                "       quit: stop client connection\n"
                "       stop: force stop server connection\n");
        else if (strcmp(command, "stop") == 0)
        {
            answer = strdup("closing server connection");
            exit_status = 1;
        }
        else if (strcmp(command, "hello") == 0)
        {
            printf("Client says hello\n");
            answer = strdup("Server says hello");
        }
        else
            answer = strdup("invalid command (try help).");
        /* Send the answer back */
        len = strlen(answer);
        /* Convert to network byte order */
        netLen = htonl(len);
        /* Send answer character length */
        if (send(currSd, &netLen, sizeof(netLen), 0) == -1)
            break;
        /* Send answer characters */
        if (send(currSd, answer, len, 0) == -1)
            break;
        free(command);
        free(answer);
        counter--;
        if (exit_status)
        {
            break;
        }
    }
    /* The loop is most likely exited when the connection is terminated */
    //printf("Connection terminated\n");
    //close(currSd);
}

int setup_server()
{
    int sd, currSd;
    int sAddrLen;
    int port = 11111;
    int len;
    unsigned int netLen;
    char *command, *answer;
    struct sockaddr_in sin, retSin;

    /* Create a new socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /* set socket options REUSE ADDRESS */
    int reuse = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEPORT) failed");
#endif
    /* Initialize the address (struct sokaddr_in) fields */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    /* Bind the socket to the specified port number */
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("bind");
        exit(1);
    }
    /* Set the maximum queue length for clients requesting connection to 5 */
    if (listen(sd, 5) == -1)
    {
        perror("listen");
        exit(1);
    }
    sAddrLen = sizeof(retSin);
    /* Accept and serve all incoming connections in a loop */

    if ((currSd =
                accept(sd, (struct sockaddr *)&retSin, &sAddrLen)) == -1)
    {
        perror("accept");
        exit(1);
    }
    /* When execution reaches this point a client established the connection.
        The returned socket (currSd) is used to communicate with the client */
    printf("Connection received from %s\n", inet_ntoa(retSin.sin_addr));
    return currSd;
}

int main(int argc, char* argv[]){

    /* Step 1: Open the device */
    printf("Opening Device\n");
    int fd = open("/dev/video0", O_RDWR);

    /* Step 2: Check streaming capability */
    printf("Checking I/O Capabilities\n");
    struct v4l2_capability cap; // Query Capability structure

    int status = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    // cap contains infomration about the camera device
    // driver card bus_info are all strings
    // capapbilities is a summed binary flags set defined in the linkx kernel, describing what it's capable of, as of https://www.linuxtv.org/downloads/v4l-dvb-apis-new/userspace-api/v4l/vidioc-querycap.html#vidioc-querycap
    if (status == -1){
        perror("Error querying capabilities\n");
        exit(EXIT_FAILURE);
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("Streaming NOT supported\n");
        exit(EXIT_FAILURE);
    }
    if (!(cap.capabilities & V4L2_CAP_READWRITE)){
        printf("No Read/Write access\n");
    }
    
    /* Step 3: Check supported formats */
    printf("Checking supported formats\n");
    struct v4l2_fmtdesc fmt; // Query Format Description structure

    int yuyvFound = 0;
    int jpegFound = 0;
    // need to read more on internal V4L2 formats and data
    // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/pixfmt-yuyv.html

    for (int idx = 0; idx < MAX_FORMAT; idx++){
        printf("%d ", idx);
        fmt.index = idx;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        status = ioctl(fd, VIDIOC_ENUM_FMT, &fmt);
        printf("%s\n", fmt.description);
        if (status != 0)
            break;
        if (fmt.pixelformat == V4L2_PIX_FMT_YUYV)
        {
            yuyvFound = 1;
        }
        if (fmt.pixelformat == V4L2_PIX_FMT_JPEG){
            jpegFound = 1;
            printf("Found JPEG\n");
        }
    }

    /* Step 4: Read current format definition */
    printf("Reading current Format settings\n");
    struct v4l2_format format; // Query Format structure

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = ioctl(fd, VIDIOC_G_FMT, &format);
    if (status == -1){
        perror("Error Querying Format");
        exit(EXIT_FAILURE);
    }

    /* Step 5: Set format fields to desired values: YUYV coding,
       480 lines, 640 pixels per line */
    printf("Setting format properties\n");
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;

    /* Step 6: Write desired format and check actual image size */
    printf("Setting the created format in V4L2\n");
    status = ioctl(fd, VIDIOC_S_FMT, &format);
    if (status == -1){
        perror("Error Setting Format\n");
        exit(EXIT_FAILURE);
    }
    int width = format.fmt.pix.width;   // Image Width
    int height = format.fmt.pix.height; // Image Height
    // Total image size in bytes
    int imageSize = (unsigned int)format.fmt.pix.sizeimage;
    printf("Image size: %d\n", imageSize);

    /* Step 7: request for allocation of 4 frame buffers by the driver */
    struct v4l2_requestbuffers reqBuf; // Buffer request structure

    printf("Request 4 image buffers\n");
    reqBuf.count = 4;
    reqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqBuf.memory = V4L2_MEMORY_MMAP;
    status = ioctl(fd, VIDIOC_REQBUFS, &reqBuf);
    if (status == -1)
    {
        perror("Error requesting buffers\n");
        exit(EXIT_FAILURE);
    }
    /* Check the number of returned buffers. It must be at least 2 */
    if (reqBuf.count < 2)
    {
        printf("Insufficient buffers\n");
        exit(EXIT_FAILURE);
    }

    struct v4l2_buffer buf; // Buffer setup structure
    printf("Allocating memory buffers\n");
    /* Step 8: Allocate a descriptor for each buffer and request its
   address to the driver. The start address in user space and the
   size of the buffers are recorded in the buffers descriptors. */

    typedef struct
    { // Buffer descriptors
        void *start;
        size_t length;
    } bufferDsc;

    bufferDsc *buffers;

    buffers = calloc(reqBuf.count, sizeof(bufferDsc));

    for (int idx = 0; idx < reqBuf.count; idx++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
        /* Get the start address in the driver space of buffer idx */
        status = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        if (status == -1)
        {
            perror("Error querying buffers\n");
            exit(EXIT_FAILURE);
        }
        /* Prepare the buffer descriptor with the address in user space
           returned by mmap() */
        buffers[idx].length = buf.length;
        buffers[idx].start = mmap(NULL, buf.length,
                                  PROT_READ | PROT_WRITE, MAP_SHARED,
                                  fd, buf.m.offset);
        if (buffers[idx].start == MAP_FAILED)
        {
            perror("Error mapping memory");
            exit(EXIT_FAILURE);
        }
    }

    /* Step 9: request the driver to enqueue all the buffers
   in a circular list */
    for (int idx = 0; idx < reqBuf.count; idx++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
        status = ioctl(fd, VIDIOC_QBUF, &buf);
        if (status == -1)
        {
            perror("Error enqueuing buffers\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Step 10: start streaming */
    enum v4l2_buf_type bufType; // Used to enqueue buffers

    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = ioctl(fd, VIDIOC_STREAMON, &bufType);
    if (status == -1)
    {
        perror("Error starting streaming\n");
        exit(EXIT_FAILURE);
    }
    printf("starting TCP Server\n");
    int curr_sd = setup_server();

    /* Step 11: wait for a buffer ready */
    fd_set fds; // Select descriptors
    struct timeval tv; // Timeout specification structure

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    for (;;)
    {

        /* Step 12: Dequeue buffer */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        do{
            status = ioctl(fd, VIDIOC_DQBUF, &buf);
        } while (-1 == status && EINTR == errno);
        if (status == -1)
        {
            perror("Error dequeuing buffer\n");
            exit(EXIT_FAILURE);
        }
        static int frame = 0;

        /* Step 13: Do image processing */
        // processImage( buffers[buf.index].start, width, height, imagesize);
        printf("frame .. %d\n", frame);
        FILE* file = fopen("output.jpeg", "wb");
        fwrite(buffers[buf.index].start, imageSize, 1, file); // size is obtained from the query_buffer function
        frame++;
        handleConnection(curr_sd);

        /* Step 14: Enqueue used buffer */
        status = ioctl(fd, VIDIOC_QBUF, &buf);
        if (status == -1)
        {
        perror("Error enqueuing buffer\n");
        exit(EXIT_FAILURE);
        }
    }
}   