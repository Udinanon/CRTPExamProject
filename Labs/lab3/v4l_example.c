/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see https://linuxtv.org/docs.php for more information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h> /* getopt_long() */

#include <fcntl.h> /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <stdint.h>

#include <linux/videodev2.h>

#include <aalib.h>
#include "render_sdl2.h"

#include "image_process.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int FRAME_width = 640;
static int FRAME_height = 480;
aa_context *FRAME_context = NULL;

enum io_method
{
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

struct buffer
{
    void *start;
    size_t length;
};

static char *dev_name;
static enum io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer *buffers;
static unsigned int n_buffers;
static int frame_count = 140;

struct v4l2_format v4l_format;


static uint8_t *grey_buffer1 = NULL;
static uint8_t *grey_buffer2 = NULL;
static uint8_t *grey_buffer3 = NULL;


enum v4l_example_config_enum
{
    FORCE_FORMAT = 1 << 0,
    OUT_BUF      = 1 << 1,
    AA_BUF       = 1 << 2,
    SDL_BUF      = 1 << 3,
    SOBEL_OPTI   = 1 << 4
};
static int16_t v4l_example_config_flags = 0;

static inline int16_t v4l_example_config_test_flags(int16_t flags)
{
    return (v4l_example_config_flags & flags) > 0;
}

static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do
    {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

aa_context *aa_create_framebuffer()
{
    int i;
    aa_context *context; /* The information about currently initialized device. */
    aa_palette palette;  /* Emulated palette (optional) */
                         // char *framebuffer;

    aa_defparams.minwidth = 80;
    aa_defparams.minheight = 40;

    // aa_defrenderparams.bright = 0.5;
    // aa_defrenderparams.gamma = 1;
    // aa_defrenderparams.contrast = 60;

    /* Initialize output driver. */
    context = aa_init(&curses_d, &aa_defparams, 0);
    if (context == NULL)
    {
        printf("Failed to initialize aalib\n");
        exit(1);
    }
    printf("aa img: [%dx%d]\n", aa_imgwidth(context), aa_imgheight(context));

    return context;
}

typedef char PIXEL;

static void process_image(const void *p, int size_bytes)
{

    uint8_t *buf0 = malloc(sizeof(char) * v4l_format.fmt.pix.sizeimage * 3);
    uint8_t *buf1 = grey_buffer1;
    uint8_t *buf2 = grey_buffer2;
    uint8_t *buf3 = grey_buffer3;

    int img_size = FRAME_height * FRAME_width;

    // if (v4l_format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
    //         printf("image format is YUYV\n");
    // }
    if (v4l_format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
    {
        size_bytes = decode_sdl2_mjpeg_frame((uint8_t *)p, buf0, size_bytes);
        size_bytes = RGB24_to_GREY(buf0, buf1, img_size);
    }

    // YUV422_to_grey(p, (unsigned char *)buf1);
    if(v4l_example_config_test_flags(SOBEL_OPTI))
        makeBorderOptimized(buf1, buf2, FRAME_width, FRAME_height, 150);
    else
        makeBorderNonOptimized(buf1, buf2, FRAME_width, FRAME_height, 150);

    int radius = 20;
    int retX, retY, retMax;

    findCenter(radius, buf2, FRAME_width, FRAME_height, &retX, &retY, &retMax, buf3);
    printf("c:[%d,%d] ~ %d      \r", retX, retY, retMax);

    if (v4l_example_config_test_flags(OUT_BUF))
        fwrite(p, size_bytes, 1, stdout);

    if (v4l_example_config_test_flags(AA_BUF))
    {
        unsigned char *src = (unsigned char *)buf1;
        unsigned char *dst = (unsigned char *)aa_image(FRAME_context);

        aa_context *ctx = FRAME_context;

        // aa_scrwidth()
        int size_x = aa_imgwidth(ctx);
        int size_y = aa_imgheight(ctx);

        // memcpy(bitmap,src,size);
        for (int y = 0; y < size_y; ++y)
        {
            memcpy(&dst[y * size_x], &src[y * FRAME_width], size_x);
        }

        aa_render(ctx, &aa_defrenderparams, 0, 0, aa_imgwidth(ctx), aa_imgheight(ctx));
        aa_flush(FRAME_context);
    }

    if (v4l_example_config_test_flags(SDL_BUF))
    {
        GREY_to_RGB24(buf2, buf0, img_size);
        int pitch = v4l_format.fmt.pix.width * sizeof(char) * 3;
        render_sdl2_frame(buf0, pitch);
    }

    if (!v4l_example_config_test_flags(SDL_BUF | AA_BUF))
    {
        fflush(stderr);
        fprintf(stderr, ".");
        fflush(stdout);
    }

    free(buf0);
}

static int read_frame(void)
{
    struct v4l2_buffer buf;
    unsigned int i;

    switch (io)
    {
    case IO_METHOD_READ:
        if (-1 == read(fd, buffers[0].start, buffers[0].length))
        {
            switch (errno)
            {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("read");
            }
        }

        process_image(buffers[0].start, buffers[0].length);
        break;

    case IO_METHOD_MMAP:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF");
            }
        }

        assert(buf.index < n_buffers);

        process_image(buffers[buf.index].start, buf.bytesused);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
        break;

    case IO_METHOD_USERPTR:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF");
            }
        }

        for (i = 0; i < n_buffers; ++i)
            if (buf.m.userptr == (unsigned long)buffers[i].start && buf.length == buffers[i].length)
                break;

        assert(i < n_buffers);

        process_image((void *)buf.m.userptr, buf.bytesused);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
        break;
    }

    return 1;
}

static unsigned mainloop_quit = 0;
static int mainloop_quit_callback(void *data)
{
    mainloop_quit = 1;
    return mainloop_quit;
}

static void mainloop(void)
{
    unsigned int count;
    count = frame_count;

    if (v4l_example_config_test_flags(SDL_BUF))
    {
        render_set_event_callback(EV_QUIT, mainloop_quit_callback, NULL);
    }

    while ((count-- > 0) && !mainloop_quit)
    {
        for (;;)
        {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* Timeout. */
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);

            render_sdl2_dispatch_events();
            if (mainloop_quit)
                break;

            if (-1 == r)
            {
                if (EINTR == errno)
                    continue;
                errno_exit("select");
            }
            if (0 == r)
            {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

            if (read_frame())
                break;
            /* EAGAIN - continue select loop. */
        }
    }
}

static void stop_capturing(void)
{
    enum v4l2_buf_type type;

    switch (io)
    {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");
        break;
    }
}

static void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io)
    {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
        {
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
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
        {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)buffers[i].start;
            buf.length = buffers[i].length;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;
    }
}

static void init_read(unsigned int buffer_size)
{
    buffers = calloc(1, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc(buffer_size);

    if (!buffers[0].start)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
}

static void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support "
                            "memory mappingn",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
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

static void init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support "
                            "user pointer i/on",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    buffers = calloc(4, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers)
    {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = malloc(buffer_size);

        if (!buffers[n_buffers].start)
        {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    switch (io)
    {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE))
        {
            fprintf(stderr, "%s does not support read i/o\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            fprintf(stderr, "%s does not support streaming i/o\n",
                    dev_name);
            exit(EXIT_FAILURE);
        }
        break;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    }
    else
    {
        /* Errors ignored. */
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l_example_config_test_flags(FORCE_FORMAT))
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
    }
    else
    {
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
            errno_exit("VIDIOC_G_FMT");
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (io)
    {
    case IO_METHOD_READ:
        init_read(fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        init_mmap();
        break;

    case IO_METHOD_USERPTR:
        init_userp(fmt.fmt.pix.sizeimage);
        break;
    }

    FRAME_width = fmt.fmt.pix.width;
    FRAME_height = fmt.fmt.pix.height;
    printf("[%dx%d]\n", FRAME_width, FRAME_height);

    int xstep = 0, ystep = 0;

    switch (fmt.fmt.pix.pixelformat)
    {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_MJPEG:
        xstep = 2;
        ystep = 4;
        break;

    default:
        perror("Format of the camera is not supported\n");
        errno_exit("VIDIOC_S_FMT");
        break;
    }

    int grey_size = FRAME_width * FRAME_height * sizeof(char);

    /* ALLOCATE FLITER BUFFERS */
    grey_buffer1 = (uint8_t *)malloc(grey_size);
    grey_buffer2 = (uint8_t *)malloc(grey_size);
    grey_buffer3 = (uint8_t *)malloc(grey_size);

    if (v4l_example_config_test_flags(SDL_BUF))
        init_render_sdl2(FRAME_width, FRAME_height, 0);

    v4l_format = fmt;
}

static void uninit_device(void)
{
    unsigned int i;

    switch (io)
    {
    case IO_METHOD_READ:
        free(buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap(buffers[i].start, buffers[i].length))
                errno_exit("munmap");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free(buffers[i].start);
        break;
    }

    free(buffers);

    if (grey_buffer1)
        free(grey_buffer1);
    if (grey_buffer2)
        free(grey_buffer2);
    if (grey_buffer2)
        free(grey_buffer3);
    if (v4l_example_config_test_flags(SDL_BUF))
        render_sdl2_clean();
}

static void open_device(void)
{
    struct stat st;

    if (-1 == stat(dev_name, &st))
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no devicen", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void close_device(void)
{
    if (-1 == close(fd))
        errno_exit("close");

    fd = -1;
}

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Version 1.3\n"
            "Options:\n"
            "-d | --device name   Video device name [%s]\n"
            "-h | --help          Print this message\n"
            "-m | --mmap          Use memory mapped buffers [default]\n"
            "-r | --read          Use read() calls\n"
            "-u | --userp         Use application allocated buffers\n"
            "-o | --output        Outputs stream to stdout\n"
            "-a | --aalib         Outputs stream to aalib\n"
            "-s | --sdl2          Outputs stream to sdl2\n"
            "-O | --opti          Select optimized Sobel transform\n"
            "-f | --format        Force format to 640x480 YUYV\n"
            "-c | --count         Number of frames to grab [%i]\n"
            "\n",
            argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruoasOfc:";

static const struct option
    long_options[] = {
        {"device", required_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"mmap", no_argument, NULL, 'm'},
        {"read", no_argument, NULL, 'r'},
        {"userp", no_argument, NULL, 'u'},
        {"output", no_argument, NULL, 'o'},
        {"aalib", no_argument, NULL, 'a'},
        {"sdl2", no_argument, NULL, 's'},
        {"opti", no_argument, NULL, 'O'},
        {"format", no_argument, NULL, 'f'},
        {"count", required_argument, NULL, 'c'},
        {0, 0, 0}};

int main(int argc, char **argv)
{
    dev_name = "/dev/video0";

    for (;;)
    {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                        short_options, long_options, &idx);

        if (-1 == c)
            break;

        switch (c)
        {
        case 0: /* getopt_long() flag */
            break;

        case 'd':
            dev_name = optarg;
            break;

        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);

        case 'm':
            io = IO_METHOD_MMAP;
            break;

        case 'r':
            io = IO_METHOD_READ;
            break;

        case 'u':
            io = IO_METHOD_USERPTR;
            break;

        case 'o':
            v4l_example_config_flags |= OUT_BUF;
            break;

        case 'a':
            FRAME_context = aa_create_framebuffer();
            v4l_example_config_flags |= AA_BUF;
            break;

        case 's':
            // render_sdl2_clean();
            v4l_example_config_flags |= SDL_BUF;
            break;

        case 'O':
            v4l_example_config_flags |= SOBEL_OPTI;
            break;

        case 'f':
            v4l_example_config_flags |= FORCE_FORMAT;
            break;

        case 'c':
            errno = 0;
            frame_count = strtol(optarg, NULL, 0);
            if (errno)
                errno_exit(optarg);
            break;

        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    open_device();
    init_device();
    start_capturing();
    mainloop();
    stop_capturing();
    uninit_device();
    close_device();
    if (FRAME_context)
        aa_close(FRAME_context);
    fprintf(stderr, "\n");
    return 0;
}