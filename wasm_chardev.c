/**
 * @file kprobe_chardev.c
 * @author Faisal Abdelmonem (fts@alumni.cmu.edu)
 * @brief 
 * @version 0.1
 * @date 2025-02-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
// #include <linux/list.h>
#include <linux/kthread.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/cdev.h>
#include <linux/slab.h>

#include "wasm_chardev.h"
#include "chardev_config.h"

/* Struct to track loaded WASM binaries */
struct wasm_instance {
    int id;
    char* pathname;
    void* binary_data;
    size_t binary_size;
    bool active;
    struct list_head list; // Linked list node to link instances
};

volatile char report_buffer[REPORT_BUFFER_SIZE];
volatile size_t report_size = 0;
static LIST_HEAD(wasm_list);
// struct wasm_instance* global_inst;
volatile int cur_id = 0;
static int next_id = 1;

long wasm_write(struct wasm_load_request* req) {
    struct wasm_instance *inst;
    pr_info("device_write() with length: %d\n", req->size);
    if (!req->size) {
        return -EINVAL;
    }

    inst = kmalloc(sizeof(*inst), GFP_KERNEL);
    if (!inst) {
        return -ENOMEM;
    }

    
    inst->pathname = kmalloc(MAX_WASM_PATH, GFP_KERNEL);
    if (!inst->pathname) {
        pr_info("could not allocate for path\n");
        kfree(inst);
        inst = NULL;
        return -ENOMEM;
    }
    
    strncpy(inst->pathname, req->path, MAX_WASM_PATH);
    
    inst->binary_data = kmalloc(req->size, GFP_KERNEL);
    if (!inst->binary_data) {
        pr_info("could not allocate for wasm binary\n");
        kfree(inst->pathname);
        kfree(inst);
        inst = NULL;
        return -ENOMEM;
    }

    if (copy_from_user(inst->binary_data, req->binary, req->size)) {
        return -EFAULT;
    }

    inst->binary_size = req->size;
    inst->id = next_id++;
    cur_id = inst->id;
    inst->active = true;

    pr_info("chrdev: set the len to %d\n", inst->binary_size);
    INIT_LIST_HEAD(&inst->list);
    list_add(&inst->list, &wasm_list);
    
    comp = INPUT;
    wake_up(&wq);

    printk(KERN_INFO "WASM binary loaded with ID: %d\n", inst->id);
    return inst->id;  // Return the descriptor (binary ID)
}

static ssize_t wasm_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {

    wait_event(wq, report_size != 0);
    pr_info("offset is: %d and report_size is %d\n", *offset, report_size);
    if (*offset >= report_size) {
        return 0;
    }

    size_t bytes_to_copy = min(len, report_size - *offset);
    
    if (copy_to_user(buf, report_buffer + *offset, bytes_to_copy)) {
        return -EFAULT;
    }

    memset(report_buffer, 0, sizeof(report_buffer));
    report_size = 0;

    *offset += bytes_to_copy;
    return bytes_to_copy;
}

int get_cur_id(void) {
    return cur_id;
}

int get_binary_size(int id) {
    struct wasm_instance* inst;
    list_for_each_entry(inst, &wasm_list, list) {
        if (inst->id == id) {
            return inst->binary_size;
        }
    }
    pr_info("No instance found\n");
    return -1;
}

void* get_binary(int id) {
    struct wasm_instance* inst;
    list_for_each_entry(inst, &wasm_list, list) {
        if (inst->id == id) {
            return inst->binary_data;
        }
    }
    pr_info("No instance found\n");
    return NULL;
}

bool is_active(int id) {
    struct wasm_instance* inst;
    list_for_each_entry(inst, &wasm_list, list) {
        if (inst->id == id) {
            return inst->active;
        }
    }
    pr_info("No instance found when checking active\n");
    return false;
}

void set_message(const char* message) {
    size_t len = strlen(message);
    if (len >= sizeof(report_buffer)) {
        pr_err("wasm-kernel: Message is too large for the buffer\n");
        return;
    }

    // Safely copy the message to the report buffer (with length checking)
    strncpy(report_buffer, message, sizeof(report_buffer) - 1);
    report_buffer[sizeof(report_buffer) - 1] = '\0';  // Null-terminate the string
    report_size = len;   
}

static long wasm_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    
    int id;
    struct wasm_instance *inst;
    struct wasm_request* req;

    if (cmd == IOCTL_LOAD) {
        struct wasm_load_request* load_req = kmalloc(sizeof(struct wasm_load_request), GFP_KERNEL);
        if (!load_req) {
            return -ENOMEM;
        }
        if (copy_from_user(load_req, (struct wasm_load_request __user*)arg, sizeof(*load_req))) {
            return -EFAULT;
        }
        
        pr_info("wasm-kernel: Loading module from path: %s, size: %zu\n", load_req->path, load_req->size);
        return wasm_write(load_req);
    } else if (cmd == IOCTL_LIST) {
        char tmp_buf[BUFFER_LEN];
        size_t offset = 0;
        list_for_each_entry(inst, &wasm_list, list) {
            int written = snprintf(tmp_buf + offset, sizeof(tmp_buf) - offset,
                                   "Path: %s, ID: %d\n", inst->pathname, inst->id);
            if (written < 0 || written >= (int)(sizeof(tmp_buf) - offset)) {
                // buffer overflow, stop adding more
                break;
            }
            offset += written;
        }
        if (copy_to_user((char __user *)arg, tmp_buf, strlen(tmp_buf) + 1)) {
            return -EFAULT;
        }
        return 0;
    } else if (cmd == IOCTL_INFO) {
        req = kmalloc(sizeof(struct wasm_request), GFP_KERNEL);
        if (!req) {
            return -ENOMEM;
        }
        if (copy_from_user(req, (struct wasm_info_request __user*)arg, sizeof(*req))) {
            return -EFAULT;
        }
        id = req->id;
    } else {
        id = (int)arg;
    }

    cur_id = id;
    
    list_for_each_entry(inst, &wasm_list, list) {
        if (inst->id == id) {
            switch (cmd) {
                case IOCTL_ACTIVATE: {
                    printk(KERN_INFO "Activating WASM binary ID: %d\n", id);
                    inst->active = true;
                    break;
                }
                
                case IOCTL_DEACTIVATE: {
                    printk(KERN_INFO "Deactivating WASM binary ID: %d\n", id);
                    inst->active = false;
                    break;
                }
                
                case IOCTL_UNLOAD: {
                    printk(KERN_INFO "Unloading WASM binary ID: %d\n", id);
                    // list_del(&inst->list);
                    // kfree(inst->binary_data);
                    // kfree(inst);
                    // inst = NULL;
                    break;
                }
                
                case IOCTL_REPORT: {
                    memset(report_buffer, 0, sizeof(report_buffer));
                    report_size = 0;
                    comp = REPORT;
                    wake_up(&wq);
                    break;
                }

                case IOCTL_INFO: {
                    snprintf(req->buffer, BUFFER_LEN,
                             "ID: %d\nPath: %s\nActive: %s\n",
                             inst->id,
                             inst->pathname,
                             inst->active ? "yes" : "no");
                
                    if (copy_to_user((void __user *)arg, req, sizeof(*req))) {
                        return -EFAULT;
                    }
                
                    return 0;
                }
                default:
                    return -EINVAL;
            }
            return 0;
        }
    }
    return -EINVAL;
}

static int wasm_open(struct inode *inode, struct file *file) {
    pr_info("=====================================================");
    pr_info("device_open(%p)\n", file); 

    try_module_get(THIS_MODULE);

    return SUCCESS;
}

static int wasm_release(struct inode *inode, struct file *file) {
    pr_info("device_release(%p,%p)\n", inode, file); 
    module_put(THIS_MODULE); 

    return SUCCESS; 
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    // .write = wasm_write,
    .read = wasm_read,
    .unlocked_ioctl = wasm_ioctl,
    .open = wasm_open,
    .release = wasm_release,
};


int chardev_init(void) {
    /* Register the character device (atleast try) */ 
    int ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

    if (ret_val < 0) { 
        pr_alert("registering the character device failed with %d\n", ret_val); 
        return ret_val; 
    } 

    cls = class_create(DEVICE_NAME); 
    device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_NAME); 

    pr_info("Device created on %s\n", DEVICE_PATH);

    return 0; 
}

void chardev_exit(void) {
    // struct wasm_instance *inst, *tmp;
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

    // if (global_inst) {
    //     kfree(global_inst->binary_data);
    //     kfree(global_inst);
    //     global_inst = NULL;
    // }

    struct wasm_instance* inst;
    struct wasm_instance* tmp;
    
    list_for_each_entry_safe(inst, tmp, &wasm_list, list) {
        list_del(&inst->list);
        kfree(inst->binary_data);
        kfree(inst);
    }

    device_destroy(cls, MKDEV(MAJOR_NUM, 0)); 
    class_destroy(cls); 
    printk(KERN_INFO "wasm_loader module unloaded\n");
 
}
