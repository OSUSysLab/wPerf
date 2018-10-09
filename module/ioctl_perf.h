#include <linux/ioctl.h>

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
#define IOCTL_SPINLOCK _IOWR(IOC_MAGIC, 19, unsigned long) 
#define IOCTL_ADDUDS _IOWR(IOC_MAGIC, 20, unsigned long) 
#define IOCTL_COPYWAIT _IOWR(IOC_MAGIC, 21, unsigned long) 
#define IOCTL_ADDWAIT _IOWR(IOC_MAGIC, 22, unsigned long) 
#define IOCTL_TIME _IOWR(IOC_MAGIC, 23, unsigned long) 


int readTarget(void);

struct socket;

struct msghdr {
	void		*msg_name;	/* ptr to socket address structure */
	int		msg_namelen;	/* size of socket address structure */
	struct iov_iter	msg_iter;	/* data */
	void		*msg_control;	/* ancillary data */
	__kernel_size_t	msg_controllen;	/* ancillary data buffer length */
	unsigned int	msg_flags;	/* flags on received message */
	struct kiocb	*msg_iocb;	/* ptr to iocb for async requests */
};

struct futex_q {
	struct plist_node list;

	struct task_struct *task;
	spinlock_t *lock_ptr;
	union futex_key key;
	struct futex_pi_state *pi_state;
	struct rt_mutex_waiter *rt_waiter;
	union futex_key *requeue_pi_key;
	u32 bitset;
};

struct futex_hash_bucket {
	atomic_t waiters;
	spinlock_t lock;
	struct plist_head chain;
} ____cacheline_aligned_in_smp;

struct uds_spin_res {
	long addr;
	int type;
};

struct fang_spin_uds {
	u64 ts;
	int pid;
	long lock;
	short type;
} __attribute__((packed));

struct fang_uds {
	u64 ts;
	int pid;
	short type;
} __attribute__((packed));


struct fang_result {
	short type;
	u64 ts;
	short core;
	int pid1;
	int pid2;
	short irq;
	short pid1state;
	short pid2state;
	u64 perfts;
} __attribute__((packed));

struct futex_result {
	int pid;
	u64 time;
}__attribute__((packed));

struct softirq_result {
	char type;
	u64 stime;
	u64 etime;
	short core;
}__attribute__((packed));

typedef struct NESTIRQ {
	int hardirq;
	struct NESTIRQ* next;
} nestirq;

//Fang newly added
#define TASKLET_HI -1
#define TIMER -2
#define NET_TX -3
#define NET_RX -4
#define BLK_DONE -5
#define BLK_IOPOLL -6
#define TASKLET -7
#define SCHED -8
#define HRTIMER -9
#define RCU -10
#define APICTIMER -11
#define IRQ -12
#define KSOFTIRQ -13
#define RANDOM -14
#define HARDIRQ -15
#define SOFTIRQ -16

#define CORENUM 100
#define PIDNUM 500
