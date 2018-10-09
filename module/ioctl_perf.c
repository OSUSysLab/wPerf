#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/export.h>
#include <linux/swap.h> /* for totalram_pages */
#include <linux/kprobes.h> 
#include <linux/bootmem.h>
#include <linux/interrupt.h>

#include <linux/slab.h>
#include <linux/file.h> 
#include <linux/syscalls.h> 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/fs.h> // required for various structures related to files liked fops. 
#include <linux/semaphore.h>
#include <linux/cdev.h> 
#include <linux/vmalloc.h>
#include <linux/futex.h>
#include <linux/stop_machine.h>
#include <linux/genhd.h>
#include <linux/uio.h>
#include "ioctl_perf.h"

#define BMAXBYTE 500
#define BSIZEBYTE (BMAXBYTE-50)
#define BMAX (BMAXBYTE * 1024 * 1024)
#define BSIZE (BSIZEBYTE * 1024 * 1024)

static int Major;
volatile int tag = -1;
volatile int step = -1;

struct task_struct *p = NULL;

unsigned long pid = -1;
unsigned long gdbpid = -1;
int ret;

struct task_struct *ptmp;
struct task_struct *ttmp;
struct task_struct *t;

char *switch_result;
char *switch_result_bak;
char *switch_result_tmp;
char *futex_result;
char *futex_result_bak;
char *futex_result_tmp;
char *state_result;
char *state_result_bak;
char *state_result_tmp;

char *wait_result;
char *wait_result_bak;
char *wait_result_tmp;

//New Added for missing event solution
volatile int ipnum = 0;
volatile int fnum = 0;
volatile int wnum = 0;
long switch_pos = 0;
long state_pos = 0;
long futex_pos = 0;
long wait_pos = 0;
int state_total = 0;
int futex_total = 0;
long tmpip;

//For softirq events
u64 stime[32];

spinlock_t switch_lock;
spinlock_t state_lock;
spinlock_t futex_lock;
spinlock_t wait_lock;

struct wperf_struct *perf_struct;

struct task_struct *(*fang_curr_task)(int);
int (*fang_get_futex_key)(u32 __user *, int, union futex_key *, int);
struct futex_hash_bucket *(*fang_hash_futex)(union futex_key *);
static const struct futex_q futex_q_init = {
	/* list gets initialized in queue_me()*/
	.key = FUTEX_KEY_INIT,
	.bitset = FUTEX_BITSET_MATCH_ANY
};
int (*fang_futex_wait_setup)(u32 __user *, u32, unsigned int, struct futex_q *, struct futex_hash_bucket **);
long (fang_futex_wait_restart)(struct restart_block *);
//For kernel version larger than 4.8
//#ifdef NEWKERNEL
//u64 (*local_clock)(void);
//#endif

long firstsec, firstusec, lastsec, lastusec;

//Fang newly added
volatile int softirq[NR_CPUS];
volatile int softirq1[NR_CPUS];
volatile short hardirq[NR_CPUS][600];
volatile short hardirq_pos[NR_CPUS];
volatile int irq[NR_CPUS];
volatile int target[PIDNUM];
volatile int aiotag[NR_CPUS];
long netsend[65535];
//long futextag[65535];
//long futexsum[65535];
//long futexstart[65535];
struct fang_result spin_result[NR_CPUS];

//Only for test
//volatile short dump_check[100000];
//volatile short dump_cpu_check[32];

volatile long futex_to = 0;
volatile long futex_noto = 0;

//static long disk_work = 0;

int dnum;
char dname[20][20];
long disk_work[20];

static __inline__ unsigned long long tsc_now(void)
{
	  unsigned hi, lo;
	    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	      return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

inline u64 fang_clock(void) {
	struct timespec64 now;
	u64 ret = 0;

	//getnstimeofday64(&now);
	//get_monotonic_boottime(&now);

	//ret = now.tv_sec*1000000000+now.tv_nsec;

	ret = tsc_now();
	//printk(KERN_INFO "current time = %lu\n", ret);
	return ret;
}

inline u64 wperfclock(void) {
	//struct timespec64 now;
	//u64 ret = 0;

	//getrawmonotonic(&now);
	//ret = now.tv_sec*1000000000+now.tv_nsec;

	//return ret;
	return local_clock();
}

//We'll check target ID & jbd thread & ksoftirq threads
int containOfPID(int core, int pid) {
	int i = 0;
	for (i = 0; i < PIDNUM; i++) {
		if (target[i] == 0) break;
		if (target[i] == pid)
			return 1;
	}
	//if (target[0] == pid) {
	//	return 1;
	//}
	//if (target[1] == pid) {
	//	return 1;
	//}
	//if (target[2+core] == pid) {
	//	return 1;
	//}
	return 0;
}

__visible __notrace_funcgraph struct task_struct * j___switch_to(struct task_struct *prev_p, struct task_struct *next_p) {
	struct timeval start;
	struct fang_result fresult;
	struct futex_result furesult;
	char *chartmp;
	int cpu = 0;
	u64 ts;

	if (step == 1) {
		cpu = get_cpu();
		if (containOfPID(cpu, prev_p->tgid)||containOfPID(cpu, next_p->tgid)) {
				ts = fang_clock();
				fresult.type = 0;
				fresult.ts = ts;
				fresult.perfts = wperfclock();
				fresult.core = cpu;
				fresult.pid1 = prev_p->pid;
				fresult.pid2 = next_p->pid;
				
				if (in_irq()) {
					//fresult.irq = hardirq[cpu][hardirq_pos[cpu]];
					fresult.irq = HARDIRQ;
				}
				else if (in_serving_softirq()) {
					fresult.irq = softirq[cpu];
				}
				else {
					fresult.irq = 0;
				}
				fresult.pid1state = prev_p->state;
				fresult.pid2state = next_p->state;

				//if (prev_p->pid == 18183) {
				//	printk(KERN_INFO "pid1 = %d, pid2 = %d, pid1state = %d, pid2state = %d\n", prev_p->pid, next_p->pid, prev_p->state, next_p->state);
				//}

				spin_lock(&switch_lock);
				chartmp = (char *)(switch_result + switch_pos);
				memcpy(chartmp, &fresult, sizeof(fresult));
				switch_pos+=sizeof(fresult);
				spin_unlock(&switch_lock);

				//if (futextag[next_p->pid] == 1) {
				//	futexstart[next_p->pid] = ts;
				//	//printk(KERN_INFO "pid=%d, ts=%ld\n", next_p->pid, futexstart[prev_p->pid]);
				//}
				//if (futextag[prev_p->pid] == 1 && futexstart[prev_p->pid] > 0) {
				//	futexsum[prev_p->pid] += ts - futexstart[prev_p->pid];

				//	if (prev_p->state>0) {
				//		spin_lock(&state_lock);
				//		chartmp = (char *)(state_result + state_pos);
				//		furesult.pid = prev_p->pid;
				//		furesult.time = futexsum[prev_p->pid];
				//		memcpy(chartmp, &furesult, sizeof(furesult));
				//		state_pos+=sizeof(furesult);
				//		futexsum[prev_p->pid] = 0;
				//		futextag[prev_p->pid] = 0; 
				//		futexstart[prev_p->pid] = 0;
				//		spin_unlock(&state_lock);
				//	}
				//}
		}
	}
	jprobe_return();
	return NULL;
}

/* We need this jprobe by both steps
 * First, we need to record the time entry in the first step.
 * Second, we need to record the last state for GDB stack.
 * */ 

static int j_try_to_wake_up(struct task_struct *p, unsigned int state, int wake_flags) {
	char tmp[200];
	struct timeval start;
	struct fang_result fresult;
	char *chartmp = NULL;
	int cpu = 0;
	int i = 0;
	u64 ts;

	if (step == 1) {
		//if (p->pid==p->tgid+8) {
		//	printk(KERN_INFO "Thread %d wakes up IO thread\n", current->pid);
		//	dump_stack();
		//	if (aiotag[cpu] == 1) {
		//		aiotag = 0;
		//		printk(KERN_INFO "Thread %d is waken up by aio_complete\n", p->pid);
		//	}
		//}
		//if (p->tgid == target[1]) {
		//	if (in_serving_softirq()) {
		//		printk(KERN_INFO "It's in softirq context\n");
		//	}
		//	else {
		//		printk(KERN_INFO "It's NOT in softirq context\n");
		//		dump_stack();
		//		printk(KERN_INFO "------------------------\n");
		//	}
		//}
		//if (current->pid == 4145 && p->pid == 4144) dump_stack();
		//if (p->pid == 4145) dump_stack();
		cpu = get_cpu();
		//if (containOfPID(cpu, p->tgid) ) {
		//if (current->pid == 2381) dump_stack();
		if (containOfPID(cpu, p->tgid)) {
		  // Check if in hardirq context
		  //if (in_irq() && hardirq_pos[cpu]<=0) {
			//	printk(KERN_INFO "[cpu %d][hardirq_pos = %d] hardirq missing, %d wakes up %d\n", cpu, hardirq_pos[cpu], current->pid, p->pid);
			//	dump_stack();
			//}
			//else if (in_serving_softirq() && softirq1[cpu] == 0) {
			//	printk(KERN_INFO "softirq missing, %d wakes up %d\n", current->pid, p->pid);
			//	dump_stack();
			//}
			
			//if (current->pid == target[cpu+2] && p->pid == 376) {
			//	dump_stack();
			//}

			if ((p->state & state)) {
				//if (current->pid != 0 && current->tgid != target[0] && p->pid == 1935) {
					//printk(KERN_INFO "%d [parent %d] wakes up %d\n", current->pid, current->tgid, p->pid);
				//}
				//if (aiotag[cpu] == 1) {
				//	aiotag[p->pid] = 1;
				//	aiotag[cpu] = 0;
				//}

				ts = fang_clock();

				fresult.type = 1;
				fresult.ts = ts;
				fresult.perfts = wperfclock();
				fresult.core = cpu;
				fresult.pid1 = current->pid;
				fresult.pid2 = p->pid;
				if (in_irq()) {
					fresult.irq = HARDIRQ;
					//dump_check[p->pid] ++;
					//if (dump_check[p->pid] == 10) {
					//	dump_cpu_check[cpu] = p->pid;
					//	dump_check[p->pid]=0;
					//}
				}
				else if (in_serving_softirq()) {
					fresult.irq = softirq[cpu];
					//if (softirq[cpu] == -2 && p->pid == 4145) dump_stack();
				}
				else {
					fresult.irq = 0;
					// Added for futex Result
					//futextag[p->pid] = 1;
				}

				fresult.pid1state = current->state;
				fresult.pid2state = p->state;

				spin_lock(&switch_lock);
				chartmp = (char *)(switch_result + switch_pos);
				memcpy(chartmp, &fresult, sizeof(fresult));
				switch_pos+=sizeof(fresult);
				spin_unlock(&switch_lock);
			}
		}
	}

	jprobe_return();
	return 0;
}

static void j_tasklet_hi_action(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = TASKLET_HI;
	}
	jprobe_return();
}

static void j_run_timer_softirq(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = TIMER;
	}
	jprobe_return();
}

static void j_net_tx_action(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = NET_TX;
	}
	jprobe_return();
}

static void j_net_rx_action(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = NET_RX;
		//if (cpu == 7) {
		//	printk(KERN_INFO "[%d]softirq[cpu] = %d\n", cpu, softirq[cpu]);
		//}
	}
	jprobe_return();
}

static void j_blk_done_softirq(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = BLK_DONE;
	}
	jprobe_return();
}

static void j_blk_iopoll_softirq(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = BLK_IOPOLL;
	}
	jprobe_return();
}

static void j_tasklet_action(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = TASKLET;
	}
	jprobe_return();
}

static void j_run_rebalance_domains(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = SCHED;
	}
	jprobe_return();
}

static void j_run_hrtimer_softirq(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = HRTIMER;
	}
	jprobe_return();
}

static void j_rcu_process_callbacks(struct softirq_action *a) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = RCU;
	}
	jprobe_return();
}

//void j_hrtimer_interrupt(void) {
void j_local_apic_timer_interrupt(void) {
	int cpu = 0;
	int vector = 0;
	if (step == 1) {
		// Get information for do_IRQ
		vector = APICTIMER;
		cpu = get_cpu();
		
		hardirq[cpu][hardirq_pos[cpu]] = vector;
		hardirq_pos[cpu]+=1;
		//printk(KERN_INFO "[%d] local_apic_timer starts\n", cpu);
		//printk(KERN_INFO "hardirq[%d] = %d, hardirq_pos[%d] = %d\n", cpu, hardirq[cpu][hardirq_pos[cpu]], cpu, hardirq_pos[cpu]);
	}
	jprobe_return();
}

unsigned int j_do_IRQ(struct pt_regs *regs) {
	int cpu = 0;
	int vector = 0;
	if (step == 1) {
		// Get information for do_IRQ
		vector = ~regs->orig_ax;
		cpu = get_cpu();

		hardirq[cpu][hardirq_pos[cpu]] = vector;
		hardirq_pos[cpu]+=1;
		//printk(KERN_INFO "[%d] local_apic_timer starts\n", cpu);
		//printk(KERN_INFO "hardirq[%d] = %d, hardirq_pos[%d] = %d\n", cpu, hardirq[cpu][hardirq_pos[cpu]], cpu, hardirq_pos[cpu]);
	}
	jprobe_return();
	return 0;
}

static void j_aio_complete(struct kiocb *kiocb, long res, long res2) {
	int cpu = get_cpu();
	aiotag[cpu] = 1;
	jprobe_return();
	return;
}

static void j_wakeup_softirqd(void) {
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = KSOFTIRQ;
	}
	jprobe_return();
}

static struct jprobe jp1 = {
        .entry                  = j___switch_to,
        .kp = {
                .symbol_name    = "__switch_to",
        },
};

static struct jprobe jp2 = {
        .entry                  = j_try_to_wake_up,
        .kp = {
                .symbol_name    = "try_to_wake_up",
        },
};

static struct jprobe jp3 = {
        .entry                  = j_tasklet_hi_action,
        .kp = {
                .symbol_name    = "tasklet_hi_action",
        },
};

static struct jprobe jp4 = {
        .entry                  = j_run_timer_softirq,
        .kp = {
                .symbol_name    = "run_timer_softirq",
        },
};

static struct jprobe jp5 = {
        .entry                  = j_net_tx_action,
        .kp = {
                .symbol_name    = "net_tx_action",
        },
};

static struct jprobe jp6 = {
        .entry                  = j_net_rx_action,
        .kp = {
                .symbol_name    = "net_rx_action",
        },
};

static struct jprobe jp7 = {
        .entry                  = j_blk_done_softirq,
        .kp = {
                .symbol_name    = "blk_done_softirq",
        },
};

static struct jprobe jp8 = {
        .entry                  = j_blk_iopoll_softirq,
        .kp = {
                .symbol_name    = "irq_poll_softirq",
        },
};

static struct jprobe jp9 = {
        .entry                  = j_tasklet_action,
        .kp = {
                .symbol_name    = "tasklet_action",
        },
};

static struct jprobe jp10 = {
        .entry                  = j_run_rebalance_domains,
        .kp = {
                .symbol_name    = "run_rebalance_domains",
        },
};

static struct jprobe jp11 = {
        .entry                  = j_run_hrtimer_softirq,
        .kp = {
                .symbol_name    = "run_hrtimer_softirq",
        },
};

static struct jprobe jp12 = {
        .entry                  = j_rcu_process_callbacks,
        .kp = {
                .symbol_name    = "rcu_process_callbacks",
        },
};

// For Local APIC Timer Interrupt
static struct jprobe jp13 = {
        .entry                  = j_local_apic_timer_interrupt,
        .kp = {
                .symbol_name    = "local_apic_timer_interrupt",
        },
};

// For HardIRQ
static struct jprobe jp14 = {
        .entry                  = j_do_IRQ,
        .kp = {
                .symbol_name    = "do_IRQ",
        },
};

//static struct jprobe jp15 = {
//        .entry                  = j_aio_complete,
//        .kp = {
//                .symbol_name    = "aio_complete",
//        },
//};
// For ksoftirq waking up
//static struct jprobe jp15 = {
//        .entry                  = j_wakeup_softirqd,
//        .kp = {
//                .symbol_name    = "wakeup_softirqd",
//        },
//};


// Add it temporarily for do_softirq
void j___do_softirq(void) {
	int cpu = 0;
	u64 ts;
	char *chartmp;
	struct softirq_result sr;
	if (step == 1) {
		cpu = get_cpu();
		ts = fang_clock();
		spin_lock(&futex_lock);
		stime[cpu] = ts;
		spin_unlock(&futex_lock);
	}
	jprobe_return();
}

static struct jprobe jp16 = {
        .entry                  = j___do_softirq,
        .kp = {
                .symbol_name    = "__do_softirq",
        },
};

void j_add_interrupt_randomness(int irq, int irq_flags) {
	int cpu = 0;
	int vector = 0;
	if (step == 1) {
		cpu = get_cpu();

		hardirq[cpu][hardirq_pos[cpu]] = RANDOM;
		hardirq_pos[cpu]+=1;
		//printk(KERN_INFO "[%d] local_apic_timer starts\n", cpu);
		//printk(KERN_INFO "hardirq[%d] = %d, hardirq_pos[%d] = %d\n", cpu, hardirq[cpu][hardirq_pos[cpu]], cpu, hardirq_pos[cpu]);
	}
	jprobe_return();
	return;
}

// Special case for add_interrupt_randomness
static struct jprobe jp17 = {
        .entry                  = j_add_interrupt_randomness,
        .kp = {
                .symbol_name    = "add_interrupt_randomness",
        },
};


inline int compareDisk(const char *dname_tmp) {
	int i = 0;
	for (; i < dnum; i++) {
		if (strcmp(dname_tmp, dname[i]) == 0) {
			return i;
		}
	}
	return -1;
}

#ifdef NEWKERNEL
static void j_part_round_stats(struct request_queue *q, int cpu, struct hd_struct *part) {
	jprobe_return();
}
#else
static void old_part_round_stats_single(struct hd_struct *part, unsigned long now) {
	int inflight;
	const char *dname_tmp = NULL;
	struct device *ddev = NULL;
	int pos = 0;

	ddev = part_to_dev(part);
	dname_tmp = dev_name(ddev);

	pos = compareDisk(dname_tmp);

	if (pos >= 0) {
		if (now == part->stamp) {
		}
		else {
			inflight = part_in_flight(part);
			if (inflight) {
				disk_work[pos] += now - part->stamp;
			}
		}
	}
}

static void j_part_round_stats(int cpu, struct hd_struct *part) {
	unsigned long now = jiffies;

	if (step == 1) {
		if (part->partno) {
			old_part_round_stats_single(&part_to_disk(part)->part0, now);
		}
		old_part_round_stats_single(part, now);
	}
	jprobe_return();
}
#endif
/*
static void j_part_round_stats_single(int cpu, struct hd_struct *part, unsigned long now) {
	int inflight;
	const char *dname_tmp = NULL;
	struct device *ddev = NULL;
	int pos = 0;

	if (step == 1) {
		ddev = part_to_dev(part);
		dname_tmp = dev_name(ddev);

		pos = compareDisk(dname_tmp);
		if (pos >= 0) {
			if (now == part->stamp) {
			}
			else {
				#ifndef NEWKERNEL
				inflight = part_in_flight(part);
				#endif
				if (inflight) {
					disk_work[pos] += now - part->stamp;
					//spin_lock(&futex_lock);
					//sr.type = 1;
					//sr.stime = part->stamp;
					//sr.etime = now;
					//sr.core = cpu;
					//chartmp = (char *)(futex_result + futex_pos);
					//memcpy(chartmp, &sr, sizeof(sr));
					//futex_pos+=sizeof(sr);
					//spin_unlock(&futex_lock);
				}
			}
		}
	}
	jprobe_return();
}
*/
static struct jprobe jp18 = {
        .entry                  = j_part_round_stats,
        .kp = {
                .symbol_name    = "part_round_stats",
        },
};

static void j_futex_wait_queue_me(struct futex_hash_bucket *hb, struct futex_q *q,
				struct hrtimer_sleeper *timeout) {
	struct fang_uds fuds;
	char *chartmp = NULL;
	if (step == 1) {
		if (current->tgid==target[0]) {
			fuds.ts = fang_clock();
			fuds.pid = current->pid;
			fuds.type = 1;

			spin_lock(&wait_lock);
			chartmp = (char *)(wait_result + wait_pos);
			memcpy(chartmp, &fuds, sizeof(fuds));
			wait_pos+=sizeof(fuds);
			spin_unlock(&wait_lock);
		}
	}
	jprobe_return();
}

static struct jprobe jp19 = {
        .entry                  = j_futex_wait_queue_me,
        .kp = {
                .symbol_name    = "futex_wait_queue_me",
        },
};

static void j_journal_end_buffer_io_sync(struct buffer_head *bh, int uptodate) {
	printk(KERN_INFO "I/O sync is happended in softirq context? %s", in_serving_softirq()>0?"True":"False");
	jprobe_return();
}

static struct jprobe jp20 = {
        .entry                  = j_journal_end_buffer_io_sync,
        .kp = {
					.symbol_name    = "journal_end_buffer_io_sync",
				},
};

void j_wake_up_new_task(struct task_struct *p) {
	if (step == 1) { 
		if (p->tgid == target[0]) {
			u64 ts = fang_clock();
			int cpu = get_cpu();
			struct fang_result fresult;
			char *chartmp = NULL;
			fresult.type = 2;
			fresult.ts = ts;
			fresult.perfts = wperfclock();
			fresult.core = cpu;
			fresult.pid1 = current->pid;
			fresult.pid2 = p->pid;
			fresult.pid1state = current->state;
			fresult.pid2state = p->state;

			spin_lock(&switch_lock);
			chartmp = (char *)(switch_result + switch_pos);
			memcpy(chartmp, &fresult, sizeof(fresult));
			switch_pos+=sizeof(fresult);
			spin_unlock(&switch_lock);

			printk(KERN_INFO "[Create] Thread %d creates thread %d time %lld\n", current->pid, p->pid, ts);
		}
	}
	jprobe_return();
}

static struct jprobe jp21 = {
	.entry                  = j_wake_up_new_task,
	.kp = {
		.symbol_name    = "wake_up_new_task",
	},
};

void j_do_exit(long code) {
	if (step == 1) {
		if (current->tgid == target[0]) {
			u64 ts = fang_clock();
			int cpu = get_cpu();
			char *chartmp = NULL;
			struct fang_result fresult;
			fresult.type = 3;
			fresult.ts = ts;
			fresult.perfts = wperfclock();
			fresult.core = cpu;
			fresult.pid1 = current->pid;
			fresult.pid2 = 0;
			fresult.pid1state = current->state;
			fresult.pid2state = 0;

			spin_lock(&switch_lock);
			chartmp = (char *)(switch_result + switch_pos);
			memcpy(chartmp, &fresult, sizeof(fresult));
			switch_pos+=sizeof(fresult);
			spin_unlock(&switch_lock);

			printk(KERN_INFO "[Finish] Thread %d finishes time %lld\n", current->pid, ts);
		}
	}
	jprobe_return();
}

static struct jprobe jp22 = {
	.entry                  = j_do_exit,
	.kp = {
		.symbol_name    = "do_exit",
	},
};

int j_tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size) {
	if (step == 1) {
		if (containOfPID(0, current->tgid)) {
			netsend[current->pid] += (&msg->msg_iter)->count;
		}
	}
	jprobe_return();
	return 0;
}

static struct jprobe jp26 = {
	.entry                  = j_tcp_sendmsg,
	.kp = {
		.symbol_name    = "tcp_sendmsg",
	},
};


int j_udp_sendmsg(struct sock *sk, struct msghdr *msg, size_t len) {
	if (step == 1) {
		if (containOfPID(0, current->tgid)) {
			netsend[current->pid] += (&msg->msg_iter)->count;
		}
	}
	jprobe_return();
	return 0;
}

static struct jprobe jp27 = {
	.entry                  = j_udp_sendmsg,
	.kp = {
		.symbol_name    = "udp_sendmsg",
	},
};

int j_tcp_sendpage(struct sock *sk, struct page *page, int offset,
		 size_t size, int flags) {
	if (step == 1) {
		if (containOfPID(0, current->tgid)) {
			netsend[current->pid] += size;
		}
	}
	jprobe_return();
	return 0;
}

static struct jprobe jp28 = {
	.entry                  = j_tcp_sendpage,
	.kp = {
		.symbol_name    = "tcp_sendpage",
	},
};

int j_udp_sendpage(struct sock *sk, struct page *page, int offset,
		 size_t size, int flags) {
	if (step == 1) {
		if (containOfPID(0, current->tgid)) {
			netsend[current->pid] += size;
		}
	}
	jprobe_return();
	return 0;
}

static struct jprobe jp29 = {
	.entry                  = j_udp_sendpage,
	.kp = {
		.symbol_name    = "udp_sendpage",
	},
};


int j_sock_sendmsg(struct socket *sock, struct msghdr *msg) {
	//struct fang_uds fuds;
	//char *chartmp = NULL;
	if (step == 1) {
		if (containOfPID(0, current->tgid)) {
			netsend[current->pid] += (&msg->msg_iter)->count;

			//fuds.ts = fang_clock();
			//fuds.pid = current->pid;
			//fuds.type = 2;

			//spin_lock(&wait_lock);
			//chartmp = (char *)(wait_result + wait_pos);
			//memcpy(chartmp, &fuds, sizeof(fuds));
			//wait_pos+=sizeof(fuds);
			//spin_unlock(&wait_lock);
		}
	}
	jprobe_return();
	return 0;
}

static struct jprobe jp23 = {
	.entry                  = j_sock_sendmsg,
	.kp = {
		.symbol_name    = "sock_sendmsg",
	},
};

static long j_futex_wait_restart(struct restart_block *restart) {
	if (step == 1) {
		printk(KERN_INFO "Core %d Thread %d futex_wait_restart Time %llu\n", get_cpu(), current->pid, fang_clock());
	}
	jprobe_return();
	return 0;
}

long j_do_futex(u32 __user *uaddr, int op, u32 val, ktime_t *timeout,
		u32 __user *uaddr2, u32 val2, u32 val3) {
//static int j_futex_wake(u32 __user *uaddr, unsigned int flags, int nr_wake, u32 bitset) {
	struct fang_uds fuds;
	char *chartmp = NULL;
	if (step == 1) {
		if (current->tgid==target[0]) {
			fuds.ts = fang_clock();
			fuds.pid = current->pid;
			fuds.type = 1;

			spin_lock(&wait_lock);
			chartmp = (char *)(wait_result + wait_pos);
			memcpy(chartmp, &fuds, sizeof(fuds));
			wait_pos+=sizeof(fuds);
			spin_unlock(&wait_lock);
		}
	}
	jprobe_return();
	return 0;
}

static struct jprobe jp24 = {
	//.entry                  = j_futex_wake,
	.entry                  = j_do_futex,
	.kp = {
		.symbol_name    = "do_futex",
		//.addr						= (kprobe_opcode_t *) kallsyms_lookup_name("futex_wait_restart"),
	},
};

static void j___lock_sock(struct sock *sk) {
	struct fang_uds fuds;
	char *chartmp = NULL;
	if (step == 1) {
		if (current->tgid==target[0]) {
			fuds.ts = fang_clock();
			fuds.pid = current->pid;
			fuds.type = 3;

			spin_lock(&wait_lock);
			chartmp = (char *)(wait_result + wait_pos);
			memcpy(chartmp, &fuds, sizeof(fuds));
			wait_pos+=sizeof(fuds);
			spin_unlock(&wait_lock);
		}
	}
	jprobe_return();
}
static struct jprobe jp25 = {
	//.entry                  = j_futex_wake,
	.entry                  = j___lock_sock,
	.kp = {
		.symbol_name    = "__lock_sock",
		//.addr						= (kprobe_opcode_t *) kallsyms_lookup_name("futex_wait_restart"),
	},
};




int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Inside open %llu\n", fang_clock());
	return 0;
}

int release(struct inode *inode, struct file *filp) {
	int i = 0;
	printk (KERN_INFO "Inside close %llu\n", fang_clock());
	for (i = 0; i < dnum; i++) {
		printk(KERN_INFO "Disk %s work time: %ld\n", dname[i], disk_work[i]);
	}
	for (i = 0; i < 65535; i++) {
		if (netsend[i] != 0) {
			printk(KERN_INFO "Thread %d sends bytes: %ld\n",i, netsend[i]);
		}
	}

//	printk(KERN_INFO "futex with timeout: %ld\n", futex_to);
//	printk(KERN_INFO "futex without timeout: %ld\n", futex_noto);

	return 0;
}

static int tasklet_hi_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret3 = {
	.handler		= tasklet_hi_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int timer_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret4 = {
	.handler		= timer_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int net_tx_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret5 = {
	.handler		= net_tx_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int net_rx_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
		//if (cpu == 7) {
		//	printk(KERN_INFO "[%d]softirq[cpu] = %d\n", cpu, softirq[cpu]);
		//}
	}
	return 0;
}

static struct kretprobe kret6 = {
	.handler		= net_rx_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int blk_done_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret7 = {
	.handler		= blk_done_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int blk_iopoll_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret8 = {
	.handler		= blk_iopoll_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int tasklet_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret9 = {
	.handler		= tasklet_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int sched_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret10 = {
	.handler		= sched_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int hrtimer_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret11 = {
	.handler		= hrtimer_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int rcu_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

static struct kretprobe kret12 = {
	.handler		= rcu_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int apictimer_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		if (hardirq_pos[cpu] != 0) hardirq_pos[cpu]-=1;
		//printk(KERN_INFO "[%d] local_apic_timer ends\n", cpu);
		//printk(KERN_INFO "hardirq_pos[%d] = %d\n", cpu, hardirq_pos[cpu]);
	}
	return 0;
}

static struct kretprobe kret13 = {
	.handler		= apictimer_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= 1024,
};

static int IRQ_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		if (hardirq_pos[cpu] != 0) hardirq_pos[cpu]-=1;
		//printk(KERN_INFO "[%d] local_apic_timer ends\n", cpu);
		//printk(KERN_INFO "hardirq_pos[%d] = %d\n", cpu, hardirq_pos[cpu]);
	}
	return 0;
}

static struct kretprobe kret14 = {
	.handler		= IRQ_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= 1024,
};

static int KSOFTIRQ_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		softirq[cpu] = 0;
	}
	return 0;
}

//static struct kretprobe kret15 = {
//	.handler		= KSOFTIRQ_return,
//	.maxactive		= NR_CPUS,
//	//.maxactive		= MAX_CPU_NR,
//};

static int SOFTIRQ_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	u64 ts;
	char *chartmp;
	struct softirq_result sr;
	if (step == 1) {
		cpu = get_cpu();
		ts = fang_clock();
		spin_lock(&futex_lock);
		sr.type = 0;
		sr.stime = stime[cpu];
		sr.etime = ts;
		sr.core = cpu;
		chartmp = (char *)(futex_result + futex_pos);
		memcpy(chartmp, &sr, sizeof(sr));
		futex_pos+=sizeof(sr);
		spin_unlock(&futex_lock);
	}
	return 0;
}

static struct kretprobe kret16 = {
	.handler		= SOFTIRQ_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

// Special for random generator in do_IRQ 
static int RANDOM_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu = get_cpu();
		if (hardirq_pos[cpu] != 0) hardirq_pos[cpu]-=1;
	}
	return 0;
}

static struct kretprobe kret17 = {
	.handler		= RANDOM_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int switch_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int cpu = 0;
	if (step == 1) {
		cpu=get_cpu();
		//if (dump_cpu_check[cpu]>0) {
			//printk(KERN_INFO "threadID = %d\n", dump_cpu_check[cpu]);
			//dump_stack();
			//dump_cpu_check[cpu]=0;
		//}
	}
	return 0;
}

static struct kretprobe kret18 = {
	.handler		= switch_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

static int futex_wait_return(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct fang_uds fuds;
	char *chartmp = NULL;
	if (step == 1) {
		//int retval = regs_return_value(regs);
		//int cpu = get_cpu();
		if (current->tgid==target[0]) {
			fuds.ts = fang_clock();
			fuds.pid = current->pid;
			fuds.type = 4;

			spin_lock(&wait_lock);
			chartmp = (char *)(wait_result + wait_pos);
			memcpy(chartmp, &fuds, sizeof(fuds));
			wait_pos+=sizeof(fuds);
			spin_unlock(&wait_lock);
		}

		/*
		if (containOfPID(cpu, current->tgid) && retval < 0) {

			//printk(KERN_INFO "Core %d Thread %d RetValue %d Time %llu\n", get_cpu(), current->pid, retval, fang_clock());
			fuds.ts = fang_clock();
			fuds.pid = current->pid;
			fuds.type = (short)retval;

			spin_lock(&wait_lock);
			chartmp = (char *)(wait_result + wait_pos);
			memcpy(chartmp, &fuds, sizeof(fuds));
			wait_pos+=sizeof(fuds);
			spin_unlock(&wait_lock);
		}
		*/
	}
	return 0;
}

static struct kretprobe kret19 = {
	.handler		= futex_wait_return,
	.maxactive		= NR_CPUS,
	//.maxactive		= MAX_CPU_NR,
};

long ioctl_funcs(struct file *filp,unsigned int cmd, unsigned long arg)
{

	unsigned long ret = 0;
	int tres;
	char tmp[200];
	char *chartmp = NULL;
	struct timeval start;
	struct fang_uds fuds;
	struct fang_spin_uds spin_uds;
	struct uds_spin_res spin_res;
	int i = 0, cpu = 0;

	switch(cmd) {
		case IOCTL_ADDWAIT:
			if (step == 1) {
				fuds.ts = fang_clock();
				fuds.pid = current->pid;
				fuds.type = (short)arg;
				spin_lock(&wait_lock);
				chartmp = (char *)(wait_result + wait_pos);
				memcpy(chartmp, &fuds, sizeof(fuds));
				wait_pos+=sizeof(fuds);
				spin_unlock(&wait_lock);
			}
			break;
		case IOCTL_ADDUDS:
			if (step == 1) {
				spin_uds.ts = fang_clock();
				spin_uds.pid = current->pid;
				copy_from_user(&spin_res, (void*)arg, 12);
				spin_uds.lock = spin_res.addr;
				spin_uds.type = spin_res.type;
				//fuds.ts = fang_clock();
				//fuds.pid = current->pid;
				//fuds.type = (short)arg;
				spin_lock(&state_lock);
				//chartmp = (char *)(state_result + state_pos);
				chartmp = (char *)(state_result + state_pos);
				memcpy(chartmp, &spin_uds, sizeof(spin_uds));
				state_pos+=sizeof(spin_uds);
				spin_unlock(&state_lock);
			}
			break;
		case IOCTL_PID:
			//printk(KERN_INFO "Setting PID = %d\n", (int)pid);
			//for_each_process(p) {
			//	if (task_pid_nr(p)==pid) break;
			//}
			//target[0] = pid;
			switch_pos = 0;
			futex_pos = 0;
			state_pos = 0;
			wait_pos = 0;

			for (i=0;i<20;i++){
				disk_work[i] = 0;
			}

			for (i=0; i<65535; i++) {
				netsend[i] = 0;
				//futextag[i] = 0;
				//futexstart[i] = 0;
				//futexsum[i] = 0;
			}

			for (i = 0; i < PIDNUM; i++) {
				target[i] = 0;
			}

			dnum = 0;

			futex_to = 0;
			futex_noto = 0;

			readTarget();

			step = 1;
			break;

		case IOCTL_INIT:
			step = -1;
			break;

		case IOCTL_COPYSWITCH:
			spin_lock(&switch_lock);
			ret = switch_pos;
			switch_pos = 0;
			switch_result_tmp  = switch_result;
			switch_result = switch_result_bak;
			spin_unlock(&switch_lock);
			switch_result_bak = switch_result_tmp;
			tres = copy_to_user((void*)arg, switch_result_bak, ret);
			break;

		case IOCTL_COPYWAIT:
			spin_lock(&wait_lock);
			ret = wait_pos;
			wait_pos = 0;
			wait_result_tmp  = wait_result;
			wait_result = wait_result_bak;
			spin_unlock(&wait_lock);
			wait_result_bak = wait_result_tmp;
			tres = copy_to_user((void*)arg, wait_result_bak, ret);
			break;

		case IOCTL_STATE_BEGIN: 
			sprintf(tmp, "Sample %d\n", state_total);
			spin_lock(&switch_lock);
			chartmp = (char *)(switch_result + switch_pos);
			memcpy(chartmp, tmp, strlen(tmp));
			switch_pos+=strlen(tmp);
			spin_unlock(&switch_lock);
			step = 2;
			state_total++;
			break;

		case IOCTL_STATE_END:
			step = -1;

			if (switch_pos > BSIZE) {
				ret = 1;
			}
			else {
				ret = 0;
			}
			//ret = 1;
			break;
		case IOCTL_FUTEX:
			spin_lock(&futex_lock);
			tag = 0;
			fnum = 0;
			wnum = 0;
			chartmp = (char *)(futex_result + futex_pos);
			memcpy(chartmp, tmp, strlen(tmp));
			futex_pos+=strlen(tmp);
			futex_total++;
			spin_unlock(&futex_lock);
			break;

		case IOCTL_JPROBE:
			break;

		case IOCTL_UNJPROBE:
			break;
		case IOCTL_JSWITCH:
			break;
		case IOCTL_COPYSTATE:
			spin_lock(&state_lock);
			ret = state_pos;
			state_pos = 0;
			state_result_tmp  = state_result;
			state_result = state_result_bak;
			spin_unlock(&state_lock);
			state_result_bak = state_result_tmp;
			tres = copy_to_user((void*)arg, state_result_bak, ret);
			//printk(KERN_INFO "state_pos = %d\n", state_pos);
			break;
		case IOCTL_COPYFUTEX:
			spin_lock(&futex_lock);
			ret = futex_pos;
			futex_pos = 0;
			futex_result_tmp  = futex_result;
			futex_result = futex_result_bak;
			spin_unlock(&futex_lock);
			futex_result_bak = futex_result_tmp;
			tres = copy_to_user((void*)arg, futex_result_bak, ret);
			break;
		case IOCTL_COPYBUFFER:
			break;
		case IOCTL_GETENTRY:
			spin_lock(&switch_lock);
			ret = copy_to_user((void*)arg, switch_result, switch_pos);
			ret = switch_pos;
			switch_pos = 0;
			spin_unlock(&switch_lock);
			break;
		case IOCTL_STEP1_BEGIN:
			switch_pos = 0;
			state_pos = 0;
			futex_pos = 0;
			wait_pos = 0;
			spin_lock(&switch_lock);
			t = p;
			ptmp = p;
			do {
				do_gettimeofday(&start);
			} while_each_thread(ptmp, t);

			printk(KERN_INFO "start time: sec = %ld usec = %ld\n", start.tv_sec, start.tv_usec);
			//step = 1;
			spin_unlock(&switch_lock);
			break;
		case IOCTL_STEP1_END:
			spin_lock(&switch_lock);
			do_gettimeofday(&start);
			lastsec = start.tv_sec;
			lastusec = start.tv_usec;
			printk(KERN_INFO "end time: sec = %ld usec = %ld\n", lastsec, lastusec);
			step = -1;
			spin_unlock(&switch_lock);
			break;
		case IOCTL_USER_STACK:
			//for_each_process(p) {
			//	if (task_pid_nr(p)==pid) break;
			//}
			//struct pt_regs *regs = task_pt_regs(p);
			//printk(KERN_INFO "IP=%ld, BP=%ld\n", regs->ip, regs->bp);
			//long currbp = regs->bp;
			//int currpos = 0;
			//while(currbp!=0 || currpos > 10) {
			//	copy_from_user(&currbp, currbp, 8);
			//	printk(KERN_INFO "Next BP = %ld", currbp);
			//}
			break;
		case IOCTL_DNAME:
			tres = copy_from_user(&dname[dnum], (void*)arg, 20);
			printk(KERN_INFO "Monitored Disk: %s\n", dname[dnum]);
			dnum++;
			break;
		case IOCTL_SPINLOCK:
			cpu = get_cpu();
			spin_result[cpu].type = (short)arg;
			spin_result[cpu].core = cpu;
			spin_result[cpu].ts = fang_clock();
			spin_result[cpu].pid1 = current->pid;
			spin_lock(&switch_lock);
			chartmp = (char *)(switch_result + switch_pos);
			memcpy(chartmp, &spin_result[cpu], sizeof(spin_result[cpu]));
			switch_pos+=sizeof(spin_result[cpu]);
			spin_unlock(&switch_lock);
			break;
		case IOCTL_TIME:
			printk(KERN_INFO "now = %llu\n", tsc_now());
			break;
	} 
	return ret;
}

struct file_operations fops = {
				open:   open,
				unlocked_ioctl: ioctl_funcs,
				release: release
};

struct cdev *kernel_cdev; 
dev_t dev_no, dev;

//Fang added
int readTarget(void) {
	mm_segment_t fs;
	struct file *filp = NULL;
	char buf[10240];
	int tmppos= 0;
	int pidpos = 0;
	char buftmp[64];
	char* buftmp1;
	int i = 0;

	fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open("/tmp/target", O_RDONLY, 0);
	if (filp==NULL) {
		printk(KERN_ERR "Error: something wrong with the pid target file!\n");
		return -1;
	}
#ifdef NEWKERNEL
	kernel_read(filp, &filp->f_pos, buf, 10240);
#else
	vfs_read(filp, buf, 10240, &filp->f_pos);
#endif
	//filp->f_op->read(filp, buf, 1024, &filp->f_pos);
	filp_close(filp,NULL);
	set_fs(fs);

	i=0;
	tmppos= 0;

	while(buf[i]) {
		if (buf[i]=='\n') {
			buftmp[tmppos]='\0';
			//printk(KERN_INFO "%d\n", simple_strtol(buftmp, &buftmp1, 10));
			target[pidpos] = simple_strtol(buftmp, &buftmp1, 10);
			pidpos++;
			tmppos = 0;
		}
		else {
			buftmp[tmppos] = buf[i];
			tmppos++;
		}
		i++;
	}
	return 0;
}

int char_arr_init (void) {
	int ret = 0;
	int i = 0;

	for (i= 0;i<PIDNUM;i++) {
		target[i] = 0;
	}

	for (i= 0;i<NR_CPUS;i++) {
		softirq[i] = 0;
		hardirq_pos[i] = 0;
		aiotag[i] = 0;
	}
	
	for (i=0; i<65535; i++) {
		netsend[i] = 0;
		//futextag[i] = 0;
		//futexstart[i] = 0;
		//futexsum[i] = 0;
	}

	readTarget();

	//Check dname
	//if (strcmp(dname, "") == 0) {
	//	printk(KERN_ERR "Error: no device name!\n");
	//	return 1;
	//}
	//else {
	//	printk(KERN_INFO "Device name: %s\n", dname);
	//}
	//Init the file read
	//fang_curr_task = (struct task_struct *(*)(int))0xffffffff870d0180;
	//fang_curr_task = (struct task_struct *(*)(int))kallsyms_lookup_name("curr_task");
	//fang_get_futex_key = (int (*)(u32 __user *, int, union futex_key *, int))kallsyms_lookup_name("get_futex_key");
	//fang_hash_futex = (struct futex_hash_bucket *(*)(union futex_key *))kallsyms_lookup_name("hash_futex");
	//fang_futex_wait_setup = (int (*)(u32 __user *, u32, unsigned int, struct futex_q *, struct futex_hash_bucket **))kallsyms_lookup_name("futex_wait_setup");
	//fang_futex_wait_restart = (long (*)(struct restart_block *)kallsyms_lookup_name("futex_wait_restart");
	//For kernel version larger than 4.8
	//#ifdef NEWKERNEL
	//local_clock = (u64 (*)(void))kallsyms_lookup_name("local_clock");
	//#endif

	//printk(KERN_INFO "curr_task = %lx\n", kallsyms_lookup_name("curr_task"));
	//printk(KERN_INFO "get_futex_key = %lx\n", kallsyms_lookup_name("get_futex_key"));
	//printk(KERN_INFO "hash_futex = %lx\n", kallsyms_lookup_name("hash_futex"));
	//printk(KERN_INFO "futex_wait_setup = %lx\n", kallsyms_lookup_name("futex_wait_setup"));

	kernel_cdev = cdev_alloc(); 
	kernel_cdev->ops = &fops;
	kernel_cdev->owner = THIS_MODULE;
	printk (" Inside init module\n");

	// New added
	dev = MKDEV(239, 0);
	ret = register_chrdev_region(dev, (unsigned int)1, "wperf");

	//ret = alloc_chrdev_region( &dev_no , 0, 1,"wperf");
	if (ret < 0) {
		printk("Major number allocation is failed\n");
		return ret; 
	}

	Major = MAJOR(dev);
	//dev = MKDEV(Major,0);
	printk (" The major number for your device is %d\n", Major);
	ret = cdev_add( kernel_cdev,dev,1);

	if(ret < 0 ) 
	{
		printk(KERN_INFO "Unable to allocate cdev");
		return ret;
	}

	state_result = (char*)vmalloc(BMAX*sizeof(char));
	state_result_bak = (char*)vmalloc(BMAX*sizeof(char));
	futex_result = (char*)vmalloc(BMAX*sizeof(char));
	futex_result_bak = (char*)vmalloc(BMAX*sizeof(char));
	switch_result = (char*)vmalloc(BMAX*sizeof(char));
	switch_result_bak = (char*)vmalloc(BMAX*sizeof(char));

	wait_result = (char*)vmalloc(BMAX*sizeof(char));
	wait_result_bak = (char*)vmalloc(BMAX*sizeof(char));

	spin_lock_init(&switch_lock);
	spin_lock_init(&state_lock);
	spin_lock_init(&futex_lock);
	spin_lock_init(&wait_lock);

	ret = register_jprobe(&jp1);
	if (ret < 0) {
		printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
		//return ret;
	}
	ret = register_jprobe(&jp2);
	if (ret < 0) {
		printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
		//return ret;
	}
	register_jprobe(&jp3);
	register_jprobe(&jp4);
	register_jprobe(&jp5);
	if (register_jprobe(&jp6)) {
		printk(KERN_INFO "register net_rx jprobe failed!\n");
	}
	register_jprobe(&jp7);
	register_jprobe(&jp8);
	register_jprobe(&jp9);
	register_jprobe(&jp10);
	//register_jprobe(&jp11);
	register_jprobe(&jp12);
	//if (register_jprobe(&jp13)) {
	//	printk(KERN_INFO "register apic jprobe failed!%d\n");
	//}

	//register_jprobe(&jp14);
	//register_jprobe(&jp15);
	register_jprobe(&jp16);
	//register_jprobe(&jp17);
	register_jprobe(&jp18);
	//register_jprobe(&jp19);
	//register_jprobe(&jp20);
	register_jprobe(&jp21);
	register_jprobe(&jp22);
	//register_jprobe(&jp23);
	//jp24.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("futex_wait_restart"),
	//register_jprobe(&jp24);
	//register_jprobe(&jp25);
	register_jprobe(&jp26);
	register_jprobe(&jp27);
	register_jprobe(&jp28);
	register_jprobe(&jp29);


	kret3.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("tasklet_hi_action");
	kret4.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("run_timer_softirq");
	kret5.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("net_tx_action");
	kret6.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("net_rx_action");
	kret7.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("blk_done_softirq");
	kret8.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("irq_poll_softirq");
	kret9.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("tasklet_action");
	kret10.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("run_rebalance_domains");
	kret11.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("run_hrtimer_softirq");
	kret12.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("rcu_process_callbacks");
	kret13.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("local_apic_timer_interrupt");
	kret14.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("do_IRQ");
//	kret15.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("wakeup_softirqd");
	kret16.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("__do_softirq");
	kret17.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("add_interrupt_randomness");
	//kret18.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("finish_task_switch");
	//kret19.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("futex_wait");
	//kret19.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("__lock_sock");

	register_kretprobe(&kret3);
	register_kretprobe(&kret4);
	register_kretprobe(&kret5);
	register_kretprobe(&kret6);
	register_kretprobe(&kret7);
	register_kretprobe(&kret8);
	register_kretprobe(&kret9);
	register_kretprobe(&kret10);
	//register_kretprobe(&kret11);
	register_kretprobe(&kret12);
	//register_kretprobe(&kret13);
	//register_kretprobe(&kret14);
	register_kretprobe(&kret16);
	//register_kretprobe(&kret17);
	//register_kretprobe(&kret18);
	//register_kretprobe(&kret19);

	return 0;
}

void char_arr_cleanup(void) {
	cdev_del(kernel_cdev);

	unregister_jprobe(&jp1);
	
	unregister_jprobe(&jp2);
	unregister_jprobe(&jp3);
	unregister_jprobe(&jp4);
	unregister_jprobe(&jp5);
	unregister_jprobe(&jp6);
	unregister_jprobe(&jp7);
	unregister_jprobe(&jp8);
	unregister_jprobe(&jp9);
	unregister_jprobe(&jp10);
	//unregister_jprobe(&jp11);
	unregister_jprobe(&jp12);
	//unregister_jprobe(&jp13);
	//unregister_jprobe(&jp14);
	//unregister_jprobe(&jp15);
	unregister_jprobe(&jp16);
	//unregister_jprobe(&jp17);
	unregister_jprobe(&jp18);
	//unregister_jprobe(&jp19);
	//unregister_jprobe(&jp20);
	unregister_jprobe(&jp21);
	unregister_jprobe(&jp22);
	//unregister_jprobe(&jp23);
	//unregister_jprobe(&jp24);
	//unregister_jprobe(&jp25);
	unregister_jprobe(&jp26);
	unregister_jprobe(&jp27);
	unregister_jprobe(&jp28);
	unregister_jprobe(&jp29);


	//unregister_kretprobe(&futex_return);
	//unregister_kretprobe(&sleep_return);
	//unregister_kretprobe(&signal_return);
	//unregister_kretprobe(&aio_return);

	unregister_kretprobe(&kret3);
	unregister_kretprobe(&kret4);
	unregister_kretprobe(&kret5);
	unregister_kretprobe(&kret6);
	unregister_kretprobe(&kret7);
	unregister_kretprobe(&kret8);
	unregister_kretprobe(&kret9);
	unregister_kretprobe(&kret10);
	//unregister_kretprobe(&kret11);
	unregister_kretprobe(&kret12);
	//unregister_kretprobe(&kret13);
	//unregister_kretprobe(&kret14);
	unregister_kretprobe(&kret16);
	//unregister_kretprobe(&kret17);
	//unregister_kretprobe(&kret18);
	//unregister_kretprobe(&kret19);

	/* nmissed > 0 suggests that maxactive was set too low. */
	//printk("Missed probing %d instances of %s\n",
	//		futex_return.nmissed, "do_futex return");

	cdev_del(kernel_cdev);
	unregister_chrdev_region(dev, 1);
	vfree(switch_result);
	vfree(switch_result_bak);
	vfree(state_result);
	vfree(state_result_bak);
	vfree(futex_result);
	vfree(futex_result_bak);

	vfree(wait_result);
	vfree(wait_result_bak);

	printk(KERN_INFO " Inside cleanup_module\n");
}
MODULE_LICENSE("GPL"); 
module_init(char_arr_init);
module_exit(char_arr_cleanup);
