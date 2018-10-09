#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <linux/ioctl.h>
#include <sys/fcntl.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>      
#include <unistd.h>     

#define IOC_MAGIC 'f'

#define IOCTL_STATE_BEGIN _IOWR(IOC_MAGIC,0, unsigned long) 
#define IOCTL_PID _IOWR(IOC_MAGIC, 1, unsigned long) 
#define IOCTL_JPROBE _IOWR(IOC_MAGIC, 2, unsigned long) 
#define IOCTL_UNJPROBE _IOWR(IOC_MAGIC, 3, unsigned long) 
#define IOCTL_COPYSWITCH _IOWR(IOC_MAGIC, 4, unsigned long) 
#define IOCTL_COPYSTATE _IOWR(IOC_MAGIC, 5, unsigned long) 
#define IOCTL_COPYFUTEX _IOWR(IOC_MAGIC, 6, unsigned long) 
#define IOCTL_JSWITCH _IOWR(IOC_MAGIC, 7, unsigned long) 
#define IOCTL_INIT _IOWR(IOC_MAGIC, 8, unsigned long) 
#define IOCTL_GDBPID _IOWR(IOC_MAGIC, 9, unsigned long) 
#define IOCTL_STATE_END _IOWR(IOC_MAGIC, 10, unsigned long) 
#define IOCTL_FUTEX _IOWR(IOC_MAGIC, 11, unsigned long) 
#define IOCTL_COPYBUFFER _IOWR(IOC_MAGIC, 12, unsigned long) 
#define IOCTL_MEMINIT _IOWR(IOC_MAGIC, 13, unsigned long) 
#define IOCTL_GETENTRY _IOWR(IOC_MAGIC, 14, unsigned long) 
#define IOCTL_STEP1_BEGIN _IOWR(IOC_MAGIC, 15, unsigned long)
#define IOCTL_STEP1_END _IOWR(IOC_MAGIC, 16, unsigned long) 
#define IOCTL_USER_STACK _IOWR(IOC_MAGIC, 17, unsigned long) 
#define IOCTL_DNAME _IOWR(IOC_MAGIC, 18, unsigned long) 
#define IOCTL_COPYWAIT _IOWR(IOC_MAGIC, 21, unsigned long) 
#define IOCTL_ADDWAIT _IOWR(IOC_MAGIC, 22, unsigned long) 
#define IOCTL_TIME _IOWR(IOC_MAGIC, 23, unsigned long) 
