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
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

#include "message_slot.h"


// -----------------------------------------------------------------------------

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Snir Fridman");

// -----------------------------------------------------------------------------

struct node {
    struct node *next;
    int chid;
    size_t len;
    char *slot_buff;
};

static struct node *lists[MINOR_COUNT];

struct node *search_node(struct file *fdesc, int s_chid) {
    // Searches for the correct node and adjusts fdesc accordingly
    struct inode *finode = fdesc->f_inode;
    int minor = iminor(finode);
    struct node *head = lists[minor];
    if (head) {
        if (head->chid == s_chid) {
	   return head;
        }
	while (head->next) {
            head = head->next;
            fdesc->f_pos = fdesc->f_pos + BUF_LEN;
            if (head->chid == s_chid) {
	        return head;
            }
	}

	// If we reached here, it means that s_chid could not be found.
        head->next = kmalloc(sizeof(struct node), GFP_USER);
	if (!(head->next)) {
            return NULL;
        }
        *(head->next) = (struct node) {NULL, s_chid};
        fdesc-> f_pos = fdesc->f_pos + BUF_LEN;
	return head->next;
    }
    else {
        head = kmalloc(sizeof(struct node), GFP_USER);
	*head = (struct node) {NULL, s_chid, 0};
	lists[minor] = head;
	return head;
    }
}

struct node *buffer_ptr(struct file *fdesc, loff_t off) {
    // Returns a pointer to the node with the buffer associated with an offset.
    unsigned int ind;
    struct inode *finode = fdesc->f_inode;
    int minor = iminor(finode);
    struct node *head = lists[minor];
    
    for (ind = off / 128; ind > 0; ind--) {
        if (!head)
	    return NULL;
	head = head->next;
    }
    if(!head)
        return NULL;
    return head;
}

void free_list(struct node *head) {
    if (head) {
        free_list(head->next);
	kfree(head);
    }
}

// -----------------------------------------------------------------------------
  
struct chardev_info {
    // The reason for this struct is to make sure the lock isn't moved
    spinlock_t lock;
}; 

static struct chardev_info device_info;
static int dev_open_flag = 0; // Marks if the device is open

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

static ssize_t device_read(struct file *filp, char __user *msg, 
                           size_t length, loff_t *offset) {
    struct node *nd = buffer_ptr(filp, *offset);
    ssize_t i;
    
    for (i = 0; i < length && i < nd->len; i++) {
        put_user(nd->slot_buff[i], &msg[i]);
    }

    return i;
}

static ssize_t device_write(struct file *filp, const char __user *msg, 
                            size_t length, loff_t *offset) {
    char *tmp, *safety;
    struct node *nd;
    ssize_t i;
    
    // Check message 0 < length <= BUF_LEN
    if (length <= 0 || length > BUF_LEN) {
        return -EMSGSIZE;
    }
    nd = buffer_ptr(filp, *offset);

    safety = kmalloc(length * sizeof(char), GFP_ATOMIC);
    if (!safety) {
      return -ENOMEM;
    }
    
    // Copy to safety buffer
    for (i = 0; i < length && i < BUF_LEN; i++) {
        get_user(safety[i], &msg[i]);
    }
    
    // Writing successful, so we can pass the pointer
    tmp = nd->slot_buff;
    nd->slot_buff = safety;
    nd->len = length;
    kfree(tmp);

    return i;
}

static long int device_ioctl(struct file *fp, unsigned int cmd, 
                            unsigned long ctrl) {
    if (cmd != MSG_SLOT_COMMAND) {
        return -EINVAL;
    }
    
    // cmd is valid
    // This is hardcoded to 3 to not conflict with the kernel
    if (ctrl == 0) {
        return -EINVAL;
    }
    else {
        if (search_node(fp, ctrl)) {
	    return 0;
	}
    }
    return -EINVAL;
}

// -----------------------------------------------------------------------------

static struct file_operations fops = {
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
    int res;
    if ((res = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &fops)) < 0) {
        printk(KERN_ALERT"%s registration failed with status %d", DEVICE_FILE_NAME, -res);
        return res;
    }
    // Getting here means the character device registered successfully.

    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);
    return 0;
}

// Cleanup
static void __exit mod_cleanup(void) {
    int i;
    for (i = 0; i < MINOR_COUNT; i++) {
        free_list(lists[i]);
    }

    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(mod_init);
module_exit(mod_cleanup);
