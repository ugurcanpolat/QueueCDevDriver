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

#include <asm/switch_to.h>        /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_*_user */

#include "queue.h"
#include "queue_ioctl.h"

#define QUEUE_MAJOR 0
#define QUEUE_NR_DEVS 4
#define QUEUE_QUANTUM 4000
#define QUEUE_QSET 1000

int queue_major = QUEUE_MAJOR;
int queue_minor = 0;
int queue_nr_devs = QUEUE_NR_DEVS;
int queue_quantum = QUEUE_QUANTUM;
int queue_qset = QUEUE_QSET;

module_param(queue_major, int, S_IRUGO);
module_param(queue_minor, int, S_IRUGO);
module_param(queue_nr_devs, int, S_IRUGO);
module_param(queue_quantum, int, S_IRUGO);
module_param(queue_qset, int, S_IRUGO);

MODULE_AUTHOR("Musa Anıl Dogan, Ugurcan Polat, Umut Cem Avin");

struct qset {
    Queue* data;
    struct qset* next;
}

struct queue_dev {
    struct qset *data;
    int quantum;
    int qset;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct queue_dev *queue_devices;

