/* 
 * chardev.h - the header file with the ioctl definitions. 
 * 
 * The declarations here have to be in a header file, because they need 
 * to be known both to the kernel module (in chardev2.c) and the process 
 * calling ioctl() (in userspace_ioctl.c). 
 */ 

 

#ifndef _CHARDEV_CONFIG_H_
#define _CHARDEV_CONFIG_H_

 

#include <linux/ioctl.h> 

#define SUCCESS 0 

/* The major device number. We can not rely on dynamic registration 
 * any more, because ioctls need to know it. 
 */ 
#define MAJOR_NUM 100 

#define BUFFER_LEN 1024

#define MAX_WASM_PATH 256

struct wasm_load_request {
    char path[MAX_WASM_PATH];
    size_t size;
    unsigned char* binary;  // or pointer if doing multi-step
};

struct wasm_request {
    int id;
    char buffer[BUFFER_LEN];
};

/* Set the message of the device driver */ 
// #define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *) 
/* _IOW means that we are creating an ioctl command number for passing 
 * information from a user process to the kernel module. 
 * 
 * The first arguments, MAJOR_NUM, is the major device number we are using. 
 * 
 * The second argument is the number of the command (there could be several 
 * with different meanings). 
 * 
 * The third argument is the type we want to get from the process to the 
 * kernel. 
 */ 

 

/* Get the message of the device driver */ 
// #define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *) 
/* This IOCTL is used for output, to get the message of the device driver. 
 * However, we still need the buffer to place the message in to be input, 
 * as it is allocated by the process. 
 */ 

/* Get the n'th byte of the message */ 
// #define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int) 
/* The IOCTL is used for both input and output. It receives from the user 
 * a number, n, and returns message[n]. 
 */ 

#define IOCTL_LOAD _IOW(MAJOR_NUM, 0, struct wasm_load_request *)
#define IOCTL_LIST _IOR(MAJOR_NUM, 1, char*)
#define IOCTL_INFO _IOR(MAJOR_NUM, 2, char*)
#define IOCTL_ACTIVATE  _IOW(MAJOR_NUM, 3, int)
#define IOCTL_DEACTIVATE _IOW(MAJOR_NUM, 4, int)
#define IOCTL_UNLOAD _IOW(MAJOR_NUM, 5, int)
#define IOCTL_REPORT _IOW(MAJOR_NUM, 6, int)

/* The name of the device file */ 
#define DEVICE_NAME "wasm_tty" 

#define DEVICE_PATH "/dev/wasm_tty" 


#endif
