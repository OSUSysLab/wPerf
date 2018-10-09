pthread_key_t fdkey;
pthread_key_t bufferkey;
pthread_key_t poskey;
pthread_key_t lasttimekey;
pthread_key_t tidkey;

int pos = 0;
char *tmp;
unsigned long lastsync = 0;

int fpid[300];
char* fbuffer[300];

typedef struct udsResult {
    unsigned long ts;
    int pid;
    long address;
    short type;
} __attribute__((packed)) uds_res;

static __inline__ unsigned long long tsctime(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

void uds_add(void* address, int type) {
	uds_res ur;
	long prev;
	char fname[30];
	char *dest = NULL;
	char *tmp = NULL;

	int* fd;
	char *buffer = NULL;
	int* pos;
	int* tid;
	long* lasttime;

	fd = (int*)pthread_getspecific(fdkey);
	buffer = (char*)pthread_getspecific(bufferkey);
	pos = (int*)pthread_getspecific(poskey);
	tid = (int*)pthread_getspecific(tidkey);
	lasttime = (long*)pthread_getspecific(lasttimekey);
	if (fd == NULL) {
		tid = (int*)malloc(sizeof(int));
		*tid = (int)syscall(SYS_gettid);
		sprintf(fname, "/tmp/wperf-%d", *tid);

		fd = (int*)malloc(sizeof(int));
		*fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC,0666);

		pos = (int*)malloc(sizeof(int));
		*pos = 0;

		buffer = (char*)malloc(50*1024*1024*sizeof(char));

		lasttime = (long*)malloc(sizeof(long));
		*lasttime = 0;

		pthread_setspecific(fdkey, fd);
		pthread_setspecific(bufferkey, buffer);
		pthread_setspecific(poskey, pos);
		pthread_setspecific(lasttimekey, lasttime);
		pthread_setspecific(tidkey, tid);
	}

	ur.address = (long)address;
	ur.type = type;
	ur.ts = tsctime();
	ur.pid = *tid;

	tmp = buffer + *pos;
	memcpy(tmp, &ur, sizeof(ur));
	*pos+=sizeof(ur);

	if (*pos > 45*1024*1024 || ur.ts - *lasttime > 1e9) {
		write(*fd, buffer, *pos);
		*pos = 0;
		*lasttime = ur.ts;
	}

	return;
}
