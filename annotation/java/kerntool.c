#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include <linux/unistd.h>
#include <linux/ioctl.h>
#include <sys/syscall.h>
#include <sys/fcntl.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>      

#include <jni.h>

#include "edu_osu_cse_ops_UDS.h"
#include "kerntool.h"


int fd;
pthread_key_t fdkey;
pthread_key_t bufferkey;
pthread_key_t poskey;
pthread_key_t lasttimekey;
pthread_key_t tidkey;

static __inline__ unsigned long long fangtime(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

JNIEXPORT void JNICALL Java_edu_osu_cse_ops_UDS_init (JNIEnv *env, jclass cls) {
	pthread_key_create(&fdkey, NULL);
	pthread_key_create(&bufferkey, NULL);
	pthread_key_create(&poskey, NULL);
	pthread_key_create(&tidkey, NULL);
	pthread_key_create(&lasttimekey, NULL);
}
 

JNIEXPORT void JNICALL Java_edu_osu_cse_ops_UDS_add (JNIEnv *env, jclass cls, jlong address, jint type) {
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
  ur.ts = fangtime();
  ur.pid = *tid;

  write(*fd, &ur, sizeof(uds_res));
  
  return;
}
