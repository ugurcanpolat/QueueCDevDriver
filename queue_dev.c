#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>    /* printk() */
#include <linux/slab.h>        /* kmalloc() */
#include <linux/fs.h>        /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <include/linux/string.h> // strcpy(), strcat()

#include <asm/switch_to.h>        /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_*_user */

#include "queue.h"
#include "queue_ioctl.h"

#define QUEUE_MAJOR 0
#define QUEUE_NR_DEVS 4

int queue_major = QUEUE_MAJOR;
int queue_minor = 0;
int queue_nr_devs = QUEUE_NR_DEVS;

module_param(queue_major, int, S_IRUGO);
module_param(queue_minor, int, S_IRUGO);
module_param(queue_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Musa Anıl Dogan, Ugurcan Polat, Umut Cem Avin");

struct queue_dev {
    Queue* data;
    unsigned int number_of_characters;
    struct semaphore sem;
    struct cdev cdev;
};

struct queue_dev *queue_devices;

int queue_trim(struct queue_dev *dev) {
    struct Queue *ptr = dev->data;
    
    while(ptr->front) {
        kfree(dequeue(ptr->data));
    }
    
    kfree(ptr);
    
    dev->data = NULL;
    dev->number_of_characters = 0;
    return 0;
}

int queue_open(struct inode *inode, struct file *filp) {
    struct queue_dev *dev;
    
    dev = container_of(inode->i_cdev, struct queue_dev, cdev);
    filp->private_data = dev;
    
    /* trim the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        queue_trim(dev);
        up(&dev->sem);
    }
    return 0;
}

int queue_release(struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t queue_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos) {
    struct queue_dev *dev = filp->private_data;
    unsigned int number_of_characters = dev->number_of_characters + 1;
    ssize_t retval = 0;
    char* concatenated, temp;
    
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    if (dev->data == NULL)
        goto out;
    
    concatenated = kmalloc(number_of_characters * sizeof(char), GFP_KERNEL);
    memset(concatenated, '\0', number_of_characters);
    
    for (temp = dev->data->front; temp; temp = temp->next) {
        strcat(concatenated, temp->data);
    }
    
    count = number_of_characters;
    
    if (copy_to_user(buf, concatenated, count)) {
        retval = -EFAULT;
        goto out;
    }
    
    kfree(concatenated);
    
    retval = count;
    
out:
    up(&dev->sem);
    return retval;
}

ssize_t queue_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos) {
    struct queue_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;
    char* text;
    
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    if (!dev->data) {
        dev->data = new_queue();
        if (!dev->data)
            goto out;
    }
    
    text = kmalloc(count * sizeof(char), GFP_KERNEL);
    if (!text)
        goto out;
    
    if (copy_from_user(text, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    
    if (enqueue(dev->data, text)) {
        retval = -EFAULT;
        goto out;
    }
    
    dev->number_of_characters += count;
    retval = count;
out:
    up(&dev->sem);
    return retval;
}
