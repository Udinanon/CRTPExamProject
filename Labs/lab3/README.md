# LAB3 - WORKING WITH DEVICES

In this Lab we will learn how to grab images from an external webcam device.

We will access the video grabber through the linux video4linux2: a middleware framework embedded into the GNU-linux kernel that interacts with actual device drivers of the camera.

**PREREQUISITES**

This software will exploits some libraries, mainly for the representation of the acquired frames as video output.

* v4l2 infrastructure ( embedded any recent in kernel )
* v4l2-tools ( just for commandline device information )
* aalib-devel ( print the picture in current console as ASCII-art )
* ncurses ( libraries for manipulating console as a output-frame )
* sdl2 ( libraries for rendering video output )
* sdl2-image ( sdl2 addons to manipulate images and decode MJPEG )


**CONTENT**
```
.
├── FindCenter.c                ( program to find the center of a given cirle radius in images )
├── GradientNonOptimized.c      ( Sobel filter non optimized )
├── GradientOptimized.c         ( Sobel filter optimized - ref. lab4 )
├── image_process.h             ( header for the function declarations of Sobel and FindCenter )
├── Makefile                    ( Makefile )
├── README.md
├── render_sdl2.c               ( sdl2 render utilities )
├── render_sdl2.h               ( sdl2 render utilities )
└── v4l_example.c               ( main application )
```


**Build the program**

`make all`

**Run program**

```
Usage: ./v4l_example [options]Version 1.3
Options:
-d | --device name   Video device name [/dev/video0]
-h | --help          Print this message
-m | --mmap          Use memory mapped buffers [default]
-r | --read          Use read() calls
-u | --userp         Use application allocated buffers
-o | --output        Outputs stream to stdout
-a | --aalib         Outputs stream to aalib
-s | --sdl2          Outputs stream to sdl2
-O | --opti          Select optimized Sobel transform
-f | --format        Force format to 640x480 YUYV
-c | --count         Number of frames to grab [140]
```


For instance to run the application with sdl2 output just enter the following command:

`./v4l_example -m -s`
