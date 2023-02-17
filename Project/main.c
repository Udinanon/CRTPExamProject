#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <asm/unistd.h>
#include <poll.h>
#define MAX_FORMAT 100

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
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

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
        FILE* file = fopen("output.yuy", "wb");
        fwrite(buffers[buf.index].start, imageSize, 1, file); // size is obtained from the query_buffer function
        frame++;

        /* Step 14: Enqueue used buffer */
        status = ioctl(fd, VIDIOC_QBUF, &buf);
        if (status == -1)
        {
        perror("Error enqueuing buffer\n");
        exit(EXIT_FAILURE);
        }
    }
}