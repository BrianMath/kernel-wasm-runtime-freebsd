/**
 * @file wasm_chardev.h
 * @brief 
 * @version 0.1
 * @date 2025-02-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _WASM_CHARDEV_H_
#define _WASM_CHARDEV_H_ 

#include <linux/fs.h> 
#include <linux/atomic.h> 

#define SUCCESS 0 

#define DEVICE_NAME "wasm_tty"

#define REPORT_BUFFER_SIZE 128

enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
};

enum { 
    WAITING = 0, 
    INPUT,
    STOP,
    REPORT
};

static struct class *cls;

extern int comp; // 0: read (I/O) 1: input, 2: interp
extern wait_queue_head_t wq;

int get_cur_id(void);
int get_binary_size(int);
void* get_binary(int);
bool is_active(int);

void set_message(const char* buffer);

int chardev_init(void);
void chardev_exit(void);

#endif
