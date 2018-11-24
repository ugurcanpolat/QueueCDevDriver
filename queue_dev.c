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

#include <linux/string.h> // strcpy(), strcat()

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
MODULE_LICENSE("Dual BSD/GPL");

struct queue_dev {
    struct queue* data;
    unsigned int number_of_characters;
    struct semaphore sem;
    struct cdev cdev;
};

struct queue_dev *queue_devices;

int queue_trim(struct queue_dev *dev) {
    struct queue *ptr = dev->data;
    
    printk(KERN_NOTICE "Trim operation started.\n");
    
    if (ptr) {
		while(ptr->front) {
			kfree(dequeue(ptr));
		}
		kfree(ptr);
	}
        
    dev->data = NULL;
    dev->number_of_characters = 0;
    printk(KERN_NOTICE "Trim operation finished.\n");
    return 0;
}

int queue_open(struct inode *inode, struct file *filp) {
    struct queue_dev *dev;
    
    printk(KERN_NOTICE "Open operation started.\n");
    
    dev = container_of(inode->i_cdev, struct queue_dev, cdev);
    filp->private_data = dev;
    
    printk(KERN_NOTICE "Open operation finished.\n");
    
    return 0;
}

int queue_release(struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t queue_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos) {
    struct queue_dev *dev = filp->private_data;
    unsigned int number_of_characters = dev->number_of_characters + 2;
    ssize_t retval = 0;
    char* concatenated;
    struct node* temp;
    
    printk(KERN_NOTICE "Read operation started.\n");
    
    if (dev == queue_devices)
        return -EINVAL;
    
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    
    if (dev->data == NULL)
        goto out;
        
    if (*f_pos >= number_of_characters)
		goto out;
    
    concatenated = kmalloc(number_of_characters * sizeof(char), GFP_KERNEL);
    
    if (!concatenated)
        goto out;
    
    memset(concatenated, '\0', number_of_characters);
    
    for (temp = dev->data->front; temp; temp = temp->next) {
        strcat(concatenated, temp->data);
    }
    
    concatenated[number_of_characters-2] = '\n';
    concatenated[number_of_characters-1] = '\0';
    
    if (count > number_of_characters - *f_pos)
		count = number_of_characters - *f_pos;
    
    if (copy_to_user(buf, concatenated + *f_pos, count)) {
		kfree(concatenated);
        retval = -EFAULT;
        goto out;
    }
    
    kfree(concatenated);
    
    *f_pos += count;
    retval = count;
    
out:
    up(&dev->sem);
    printk(KERN_NOTICE "Read operation finished.\n");
    return retval;
}

ssize_t queue_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos) {
    struct queue_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;
    char* text;
    
    printk(KERN_NOTICE "Write operation started.\n");
    
    if (dev == queue_devices)
        return -EINVAL;
    
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
    
    text[count-1] = '\0';
    
    if (enqueue(dev->data, text)) {
		kfree(text);
        retval = -EFAULT;
        goto out;
    }
    
    dev->number_of_characters += count - 1;
    retval = count;
    
    printk(KERN_NOTICE "The string \"%s\" added to queue.\n", text);
    kfree(text);

out:
    up(&dev->sem);
    printk(KERN_NOTICE "Write operation finished. Char count: %u\n", dev->number_of_characters);
    return retval;
}

long queue_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    int err = 0;
    int retval = 0;
    char* text = NULL;
    struct queue_dev* dev;
    
    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != QUEUE_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > QUEUE_IOC_MAXNR) return -ENOTTY;
    
    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;
    
    if (cmd == QUEUE_POP) {
        if (! capable (CAP_SYS_ADMIN))
            return -EPERM;
        
        printk(KERN_NOTICE "IOCTL Pop operation started.\n");
        dev = filp->private_data;
        
        if (dev == queue_devices) {
            int i;
            for (i = 1; i < queue_nr_devs; i++) {
                if ((queue_devices+i)->data->front) {
                    text = dequeue((queue_devices+i)->data);
                    (queue_devices+i)->number_of_characters -= strlen(text);
                    printk(KERN_NOTICE "String \"%s\" is popped from queue%d.\n", text, i);
                    break;
                }
            }
            
        } else {
            text = dequeue(dev->data);
            dev->number_of_characters -= strlen(text);
            printk(KERN_NOTICE "String \"%s\" is popped.\n", text);
        }
        
        retval = __put_user(text, (char __user **)arg);
        kfree(text);
    }
    else
        return -ENOTTY;
    return retval;
}

struct file_operations queue_fops = {
    .owner =    THIS_MODULE,
    .read =     queue_read,
    .write =    queue_write,
    .unlocked_ioctl =  queue_ioctl,
    .open =     queue_open,
    .release =  queue_release,
};

void queue_cleanup_module(void) {
    int i;
    dev_t devno = MKDEV(queue_major, queue_minor);
    
    printk(KERN_NOTICE "Module cleanup started.\n");
    
    if (queue_devices) {
        for (i = 0; i < queue_nr_devs; i++) {
            queue_trim(queue_devices + i);
            cdev_del(&queue_devices[i].cdev);
        }
        kfree(queue_devices);
    }
    
    unregister_chrdev_region(devno, queue_nr_devs);
    printk(KERN_NOTICE "Module cleanup finished.\n");
}

int queue_init_module(void) {
    int result, i;
    int err;
    dev_t devno = 0;
    struct queue_dev *dev;
    
    printk(KERN_NOTICE "Module initialization started.\n");
    
    if (queue_major) {
        devno = MKDEV(queue_major, queue_minor);
        result = register_chrdev_region(devno, queue_nr_devs, "queue");
    } else {
        result = alloc_chrdev_region(&devno, queue_minor, queue_nr_devs,
                                     "queue");
        queue_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "queue: can't get major %d\n", queue_major);
        return result;
    }
    
    queue_devices = kmalloc(queue_nr_devs * sizeof(struct queue_dev),
                            GFP_KERNEL);
    if (!queue_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(queue_devices, 0, queue_nr_devs * sizeof(struct queue_dev));
    
    /* Initialize each device. */
    for (i = 0; i < queue_nr_devs; i++) {
        dev = &queue_devices[i];
        sema_init(&dev->sem,1);
        devno = MKDEV(queue_major, queue_minor + i);
        cdev_init(&dev->cdev, &queue_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &queue_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding queue%d\n", err, i);
        printk(KERN_NOTICE "Device: queue%d initialized.\n", i);
    }
    
    printk(KERN_NOTICE "Module initialization finished.\n");
    return 0; /* succeed */
    
fail:
    queue_cleanup_module();
    return result;
}

module_init(queue_init_module);
module_exit(queue_cleanup_module);
