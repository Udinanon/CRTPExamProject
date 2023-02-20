/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see https://linuxtv.org/docs.php for more information
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>  /* low-level i/o */
#include <getopt.h> /* getopt_long() */
#include <linux/videodev2.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// Utility and constants

void* receive_data();
void send_string(char*);

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int FRAME_width = 640;
static int FRAME_height = 480;

struct buffer {
  void *start;
  size_t length;
};

static char *dev_name;
static int fd = -1;
struct buffer *buffers;
static unsigned int n_buffers;
static int frame_count = 140;
static int currSd;

struct v4l2_format v4l_format;

enum v4l_example_config_enum {
  FORCE_FORMAT = 1 << 0,
  OUT_BUF = 1 << 1,
};
static int16_t v4l_example_config_flags = 0;

static inline int16_t v4l_example_config_test_flags(int16_t flags) {
  return (v4l_example_config_flags & flags) > 0;
}

static void errno_exit(const char *s) {
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg) {
  int r;

  do {
    r = ioctl(fh, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}

// Program Core

typedef char PIXEL;
int frame_n = 0;

static void process_image(const void *p) {
  // p is a generic pointer to where the image resides, either directly as memory or as a mmap pointer or a userpointer
  if (frame_n % 5 == 0){
  getchar();
  }
  char* name;
  asprintf(&name, "frame_%d", frame_n);
  printf("Sending frame %d named %s\n", frame_n, name);
  send_string(name);

  int len = v4l_format.fmt.pix.sizeimage;
  printf("Local Pointer: %p\n", p);
  unsigned int netLen = htonl(len);
  if (send(currSd, &netLen, sizeof(netLen), 0) == -1)
    perror("ERROR SENDING MSG LEN");
  /* Send answer characters */
  if (send(currSd, (uint8_t *)p, len, 0) == -1)
    perror("ERROR SENDING MSG");
  if (v4l_format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) {
      FILE *file = fopen("output.jpeg", "wb");
      fwrite((uint8_t *)p, v4l_format.fmt.pix.sizeimage, 1, file);  // size is obtained from the query_buffer function
    }
  frame_n++;
}

static int read_frame(void) {
  struct v4l2_buffer buf;


  CLEAR(buf);

  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
    switch (errno) {
      case EAGAIN:
        return 0;
      default:
        errno_exit("VIDIOC_DQBUF");
    }
  }

  assert(buf.index < n_buffers);

  process_image(buffers[buf.index].start);

  if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
    errno_exit("VIDIOC_QBUF");

  return 1;
}

static unsigned mainloop_quit = 0;

static void mainloop(void) {
  unsigned int count;
  count = frame_count;

  while ((count-- > 0) && !mainloop_quit) {
    for (;;) {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      /* Timeout. */
      tv.tv_sec = 5;
      tv.tv_usec = 0;

      r = select(fd + 1, &fds, NULL, NULL, &tv);

      /*if (getchar() == "s") {  // stop command
        mainloop_quit = 1;
        break;
      }*/

      if (-1 == r) {
        if (EINTR == errno)
          continue;
        errno_exit("select");
      }
      if (0 == r) {
        fprintf(stderr, "select timeout\n");
        exit(EXIT_FAILURE);
      }

      if (read_frame())
        break;
      /* EAGAIN - continue select loop. */
    }
  }
}

// V4L2 setup and management functions

static void stop_capturing(void) {
  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
    errno_exit("VIDIOC_STREAMOFF");
}

static void start_capturing(void) {
  unsigned int i;
  enum v4l2_buf_type type;

  for (i = 0; i < n_buffers; ++i) {
    struct v4l2_buffer buf;

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
  }
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
    errno_exit("VIDIOC_STREAMON");

}

static void init_mmap(void) {
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno) {
      fprintf(stderr,
              "%s does not support "
              "memory mappingn",
              dev_name);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  if (req.count < 2) {
    fprintf(stderr, "Insufficient buffer memory on %s\n",
            dev_name);
    exit(EXIT_FAILURE);
  }

  buffers = calloc(req.count, sizeof(*buffers));

  if (!buffers) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers;

    if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
      errno_exit("VIDIOC_QUERYBUF");

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start =
        mmap(NULL /* start anywhere */,
             buf.length,
             PROT_READ | PROT_WRITE /* required */,
             MAP_SHARED /* recommended */,
             fd, buf.m.offset);

    if (MAP_FAILED == buffers[n_buffers].start)
      errno_exit("mmap");
  }
}


static void init_device(void) {
  struct v4l2_capability cap;
  struct v4l2_format fmt;
  unsigned int min;

  if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
    if (EINVAL == errno) {
      fprintf(stderr, "%s is no V4L2 device\n",
              dev_name);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_QUERYCAP");
    }
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "%s is no video capture device\n",
            dev_name);
    exit(EXIT_FAILURE);
  }


  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    fprintf(stderr, "%s does not support streaming i/o\n",
            dev_name);
    exit(EXIT_FAILURE);
  }

  /* Select video input, video standard and tune here. */

  // need to read more on internal V4L2 formats and data
  // https://www.kernel.org/doc/html/v4.8/media/uapi/v4l/pixfmt-yuyv.html
  struct v4l2_fmtdesc fmtDesc;  // Query Format Description structure
  int status, jpegFound;
  for (int idx = 0; idx < 100; idx++) {
    printf("%d ", idx);
    fmtDesc.index = idx;
    fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = ioctl(fd, VIDIOC_ENUM_FMT, &fmtDesc);
    printf("%s\n", fmtDesc.description);
    if (status != 0)
      break;
    if (fmtDesc.pixelformat == V4L2_PIX_FMT_MJPEG){
      jpegFound = 1;
    }
  }

  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (jpegFound)  // use use JPEG format if it's available
  {
    fmt.fmt.pix.width = FRAME_width;
    fmt.fmt.pix.height = FRAME_height;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // YUV422
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
      errno_exit("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */
  } else {
    errno_exit("JPEG_FORMAT_UNAVAILABLE\n");
  }

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  init_mmap();


  FRAME_width = fmt.fmt.pix.width;
  FRAME_height = fmt.fmt.pix.height;
  printf("[%dx%d]\n", FRAME_width, FRAME_height);

  switch (fmt.fmt.pix.pixelformat) {
    case V4L2_PIX_FMT_MJPEG:
      break;

    default:
      perror("Format of the camera is not supported\n");
      errno_exit("VIDIOC_S_FMT");
      break;
  }


  v4l_format = fmt;
}

static void uninit_device(void) {
  unsigned int i;

  for (i = 0; i < n_buffers; ++i)
    if (-1 == munmap(buffers[i].start, buffers[i].length))
      errno_exit("munmap");

  free(buffers);
}

static void open_device(void) {
  struct stat st;

  if (-1 == stat(dev_name, &st)) {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n",
            dev_name, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (!S_ISCHR(st.st_mode)) {
    fprintf(stderr, "%s is no devicen", dev_name);
    exit(EXIT_FAILURE);
  }

  fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

  if (-1 == fd) {
    fprintf(stderr, "Cannot open '%s': %d, %s\n",
            dev_name, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void close_device(void) {
  if (-1 == close(fd))
    errno_exit("close");

  fd = -1;
}

// HHTP Server
static int currSd;  // identifier for connection used by server

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
  if (receive(currSd, (char *)&netLen, sizeof(netLen))) {
    perror("recv");
    exit(0);
  }
  /* Convert from Network byte order */
  len = ntohl(netLen);
  /* Allocate and receive the answer */
  message = malloc(len + 1);
  if (receive(currSd, message, len)) {
    perror("recv");
    exit(1);
  }
  return message;
}

void send_string(char *string) {
  /* Send the answer back */
  int len = strlen(string);
  /* Convert to network byte order */
  unsigned int netLen = htonl(len);
  /* Send answer character length */
  if (send(currSd, &netLen, sizeof(netLen), 0) == -1)
    perror("ERROR SENDING MSG LEN");
  /* Send answer characters */
  char string2[len];
  strcpy(string2, string);
  if (send(currSd, string2, len, 0) == -1)
    perror("ERROR SENDING MSG");
}

void setup_server() {
  int sAddrLen, sd;
  int port = 11111;
  int len;
  unsigned int netLen;
  struct sockaddr_in sin, retSin;

  /* Create a new socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
  if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    perror("bind");
    exit(1);
  }
  /* Set the maximum queue length for clients requesting connection to 1*/
  if (listen(sd, 1) == -1) {
    perror("listen");
    exit(1);
  }
  sAddrLen = sizeof(retSin);
  /* Accept and serve the incoming connection */
  printf("Waiting for client connection on port %d\n", port);
  if ((currSd =
           accept(sd, (struct sockaddr *)&retSin, &sAddrLen)) == -1) {
    perror("accept");
    exit(1);
  }
  /* When execution reaches this point a client established the connection.
      The returned socket (currSd) is used to communicate with the client */
  printf("Connection received from %s\n", inet_ntoa(retSin.sin_addr));
  char* message = receive_data();
  printf("Client says: %s\n", message);
  char* video_info;
  asprintf(&video_info, "[%dx%d]\n", FRAME_width, FRAME_height);
  send_string(video_info);

  len = sizeof(v4l_format.fmt.pix.sizeimage);
  netLen = htonl(len);
  if (send(currSd, &netLen, sizeof(netLen), 0) == -1)
    perror("ERROR SENDING MSG LEN");
  /* Send answer characters */
  if (send(currSd, &v4l_format.fmt.pix.sizeimage, len, 0) == -1)
    perror("ERROR SENDING MSG");

  free(video_info);
}

int main() {
  dev_name = "/dev/video0";
  
  open_device();
  init_device();
  setup_server();
  start_capturing();
  mainloop();
  stop_capturing();
  uninit_device();
  close_device();
  return 0;
}