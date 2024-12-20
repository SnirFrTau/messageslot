#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <errno.h>

#include "message_slot.h"


// -----------------------------------------------------------------------------

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Snir Fridman");

// -----------------------------------------------------------------------------

struct node {
    struct node *next;
    int chid;
};

static struct node lists[256];

void search_node(struct file *fdesc, int s_chid) {
    // Searches for the correct node and adjusts fdesc accordingly
    struct inode *finode = fget(fdesc)->f_inode;
    int minor = iminor(finode);
    struct node *head = lists[minor];

    if (head != NULL) {
        if (head->chid == s_chid) {
	   return;
        }
    }
    while (head->next != NULL) {
        head = head->next;
        fdesc->f_pos = fdesc->f_pos + BUF_LEN;
        if (head->chid == s_chid) {
	    return;
        }
    }
    // If we reached here, it means that s_chid could not be found.
    head->next = kmalloc(sizeof(struct node));
    *(head->next) = {NULL, s_chid};
    fdesc-> f_pos = f_desc->f_pos + BUF_LEN;
}

struct chardev_info {
    // The reason for this struct is to make sure the lock isn't moved
    spinlock_t lock;
}; 

static struct chardev_info device_info;
static int major;
static char slot_buff[BUF_LEN];
static int dev_open_flag = 0; // Marks if the device is open
static unsigned int channel_bmap[1024 * 1024];

// -----------------------------------------------------------------------------

static int device_open(struct inode *inode, struct file *file) {
    unsigned long flags;

    spin_lock_irqsave(&device_info.lock, flags);
    if (dev_open_flag == 1) {
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }

    dev_open_flag++;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
    unsigned long flags;

    spin_lock_irqsave(&device_info.lock, flags);
    dev_open_flag--;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

static ssize_t device_read(struct file *fp, char __user *msg, 
                           size_t length, loff_t *offset) {
    ssize_t i;
    for (i = 0; i < length && i < BUF_LEN; i++) {
        put_user(slot_buff[i], &msg[i]);
    }

    return i;
}

static ssize_t device_write(struct file *fp, const char __user *msg, 
                            size_t length, loff_t *offset) {
    // Check message 0 < length <= BUF_LEN
    if (length <= 0 || length > 128) {
        errno = EMSGSIZE;
        return -1;
    }

    ssize_t i;
    for (i = 0; i < length && i < BUF_LEN; i++) {
        get_user(slot_buff[i], &msg[i]);
    }

    return i;
}

static int device_ioctl(struct file *fp, unsigned int ctrl, 
                            unsigned long cmd) {
    if (cmd != MSG_SLOT_COMMAND) {
        errno = EINVAL;
        return -1;
    }
    
    // cmd is valid
    if (ctrl == 0) {
        errno = EINVAL;
        return -1;
    }
    else {
        search_node(fp, ctrl);
    }
    return 0;
}

// -----------------------------------------------------------------------------

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .llseek = NULL,
    .flush = NULL,
    .release = device_release
};

// -----------------------------------------------------------------------------

// Loader
static int __init mod_init(void) {
    if ((major = register_chrdev(MAJOR_NUM, "message_slot", &fops)) < 0) {
        printk(KERN_ALERT"%s registration failed for %d", DEVICE_FILE_NAME, major);
        return major;
    }
    // Getting here means the character device registered successfully.

    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);

    printk("Testing of major %d\n", major);
    return 0;
}

// Cleanup
static void __exit mod_cleanup(void) {
    // TODO: free all used-up memory!

    printk("Cleanup of major %d\n", major);
}

module_init(mod_init);
module_exit(mod_cleanup);
