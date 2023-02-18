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

We open a device, set it up via `ioctl` commands, and then we can start querying it for images. 

Here multiple options arise, from image formats to the approach we want to access memory.

For images we can use JPEG to avoid decoding issues

The topic of memory access is a bit more complex, if we use *streaming* we can access memory via a pointer, but i fear this might not work between processes or even threads, so we can't just send one via TCP/IP and hope it works. 
We'll have to either open it up or use another I/O method, maybe directly reading the data and passing it along

For our architecture we want:
 - producer getting images from webcam, passing them via sockets to a consumer
 - consumer receving them and saving them to disk

pretty simple in theory
