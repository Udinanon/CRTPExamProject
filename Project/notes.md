# Makefiles
https://opensource.com/article/18/8/what-how-makefile

```makefile
target: prerequisites
    recipie
```

calling a target checks for its prerequisites, which can be files or other targets
recipes are basically shell commands
@ prefix supresses rinting the command
first target is the default one, usually called `all`

# V4L2

We open a device, set it up via `ioctl` commands and then we can start querying it for images. 

Here multiple options arise, from image formats to the appraoch we want to access memory.

FOr images we can use YUYV cause it's simple and we'll probably be able to use it directly, and we shuld have some examples already available. Otherwise i tink the Motion Jpeg is alos quite simple.

The topic of memory access is a bit more complex, if we use *streaming* we can access memory via a poionter, but i fear this might not work between threads or even processes, so we can0t just send one via TCP/PI and hope it works. We'll have to either open it up or use another I/O method, aybe directly reading the data and passing it along
