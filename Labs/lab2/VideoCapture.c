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
#define FALSE 0
#define TRUE 1
#define CHECK_IOCTL_STATUS(message) \
if(status == -1)                    \
{                                   \
    perror(message);                \
    exit(EXIT_FAILURE);             \
}


static int xioctl(int fh, unsigned long int request, void *arg)
{
	int r;
	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);
	return r;
}

int main (int argc, char *argv[])
{
    int fd, idx, status;
    int pixelformat;
    int imageSize;
    int width, height;
    int yuyvFound;

    struct v4l2_capability cap;       //Query Capability structure
    struct v4l2_fmtdesc fmt;          //Query Format Description structure
    struct v4l2_format format;        //Query Format structure
    struct v4l2_requestbuffers reqBuf;//Buffer request structure
    struct v4l2_buffer buf;           //Buffer setup structure

    enum v4l2_buf_type bufType;       //Used to enqueue buffers

    typedef struct {                  //Buffer descriptors
        void *start;
        size_t length;
    } bufferDsc;

    bufferDsc *buffers;

    fd_set fds;                     //Select descriptors
    struct timeval tv;              //Timeout specification structure

/* Step 1: Open the device */
    fd = open("/dev/video0", O_RDWR);

/* Step 2: Check streaming capability */
    status = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    CHECK_IOCTL_STATUS("Error querying capability")
    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("Streaming NOT supported\n");
        exit(EXIT_FAILURE);
    }

/* Step 3: Check supported formats */
    yuyvFound = FALSE;
    for(idx = 0; idx < MAX_FORMAT; idx++)
    {
        fmt.index = idx;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        status = ioctl(fd, VIDIOC_ENUM_FMT, &fmt);
        if(status != 0) break;
        if(fmt.pixelformat == V4L2_PIX_FMT_YUYV)
        {
            yuyvFound = TRUE;
            break;
        }
    }
    if(!yuyvFound)
    {
        printf("YUYV format not supported\n");
        exit(EXIT_FAILURE);
    }

/* Step 4: Read current format definition */
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = ioctl(fd, VIDIOC_G_FMT, &format);
    CHECK_IOCTL_STATUS("Error Querying Format")

/* Step 5: Set format fields to desired values: YUYV coding,
    480 lines, 640 pixels per line */
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

/* Step 6: Write desired format and check actual image size */
    status = ioctl(fd, VIDIOC_S_FMT, &format);
    CHECK_IOCTL_STATUS("Error Setting Format");
    width = format.fmt.pix.width;                       //Image Width
    height = format.fmt.pix.height;                     //Image Height
    //Total image size in bytes
    imageSize = (unsigned int)format.fmt.pix.sizeimage;

/* Step 7: request for allocation of 4 frame buffers by the driver */
    reqBuf.count = 4;
    reqBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqBuf.memory = V4L2_MEMORY_MMAP;
    status = ioctl(fd, VIDIOC_REQBUFS, &reqBuf);
    CHECK_IOCTL_STATUS("Error requesting buffers")
/* Check the number of returned buffers. It must be at least 2 */
    if(reqBuf.count < 2)
    {
        printf("Insufficient buffers\n");
        exit(EXIT_FAILURE);
    }

/* Step 8: Allocate a descriptor for each buffer and request its
   address to the driver. The start address in user space and the
   size of the buffers are recorded in the buffers descriptors. */
    buffers = calloc(reqBuf.count, sizeof(bufferDsc));
    
    for(idx = 0; idx < reqBuf.count; idx++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
/* Get the start address in the driver space of buffer idx */
        status = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        CHECK_IOCTL_STATUS("Error querying buffers")
/* Prepare the buffer descriptor with the address in user space
   returned by mmap() */
        buffers[idx].length = buf.length;
        buffers[idx].start = mmap(NULL, buf.length,
            PROT_READ | PROT_WRITE,MAP_SHARED,
            fd, buf.m.offset);
        if(buffers[idx].start == MAP_FAILED)
        {
            perror("Error mapping memory");
            exit(EXIT_FAILURE);
        }
    }

/* Step 9: request the driver to enqueue all the buffers
   in a circular list */
    for(idx = 0; idx < reqBuf.count; idx++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
        status = ioctl(fd, VIDIOC_QBUF, &buf);
        CHECK_IOCTL_STATUS("Error enqueuing buffers")
    }

/* Step 10: start streaming */
    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = ioctl(fd, VIDIOC_STREAMON, &bufType);
    CHECK_IOCTL_STATUS("Error starting streaming")

/* Step 11: wait for a buffer ready */
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    for(;;)
    {
        // status = select(1, &fds, NULL, NULL, &tv);
        // if(status == -1)
        // {
        //     perror("Error in Select");
        //     exit(EXIT_FAILURE);
        // }
/* Step 12: Dequeue buffer */
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        status = xioctl(fd, VIDIOC_DQBUF, &buf);
        CHECK_IOCTL_STATUS("Error dequeuing buffer")
        static int frame = 0;
        if ( frame % 10 == 0)
/* Step 13: Do image processing */
            // processImage( buffers[buf.index].start, width, height, imagesize);
            printf("frame .. %d\n", frame);
        frame++;

/* Step 14: Enqueue used buffer */
        status = ioctl(fd, VIDIOC_QBUF, &buf);
        CHECK_IOCTL_STATUS("Error enqueuing buffer")
    }
}
