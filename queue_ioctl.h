#ifndef __QUEUE_H
#define __QUEUE_H

#include <linux/ioctl.h> / _IOR

#define QUEUE_IOC_MAGIC  'k'
#define QUEUE_POP    _IOR(QUEUE_IOC_MAGIC, 0, int)
#define QUEUE_IOC_MAXNR 0

#endif
