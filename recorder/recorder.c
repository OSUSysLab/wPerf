#include "recorder.h"

int i,fd;
clock_t clk1,clk2;
struct timespec tbuf1;
struct timespec tbuf2;
double rt;
FILE *fp, *fp1, *fp2, *fp3;
char *mem;
long mem_pos = 0;

void term(int signum)
{
	ioctl(fd,IOCTL_INIT, -1);

	// Final Copy 
	mem_pos = ioctl(fd,IOCTL_COPYSWITCH, mem);
	fwrite(mem, 1, mem_pos, fp);

	mem_pos = ioctl(fd,IOCTL_COPYFUTEX, mem);
	fwrite(mem, 1, mem_pos, fp1);

	mem_pos = ioctl(fd,IOCTL_COPYSTATE, mem);
	//printf("[1] COPYSTATE mem_pos = %d\n", mem_pos);
	fwrite(mem, 1, mem_pos, fp2);

	mem_pos = ioctl(fd,IOCTL_COPYWAIT, mem);
	fwrite(mem, 1, mem_pos, fp3);

	fclose(fp);
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	close(fd);
	exit(0);
}

int main (int argc, char *argv[]) {
	int i = 0;
	int j = 0;

	mem = (char*)malloc(500*1024*1024*sizeof(char));
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);

	char *dname = (char*)calloc(20,sizeof(char));

	fd = open("/dev/wperf", O_RDWR);

	if (fd == -1)
	{
		printf("Error in opening file \n");
		exit(-1);
	}

	fp = fopen("result_switch","w");
	fp1 = fopen("result_softirq","w");
	fp2 = fopen("result_fake","w");
	fp3 = fopen("result_wait","w");
		
	if (atoi(argv[1])==0) {
		ioctl(fd,IOCTL_INIT, 0);
		ioctl(fd,IOCTL_PID, 0);

		for ( j = 2; j < argc; j++) {
			strcpy(dname, argv[j]);
			ioctl(fd, IOCTL_DNAME, dname);
		}	

		while(i<36000) {
			sleep(1);
			mem_pos = ioctl(fd,IOCTL_COPYSWITCH, mem);
			fwrite(mem, 1, mem_pos, fp);
			mem_pos = ioctl(fd,IOCTL_COPYFUTEX, mem);
			fwrite(mem, 1, mem_pos, fp1);
			mem_pos = ioctl(fd,IOCTL_COPYSTATE, mem);
			//printf("[0] COPYSTATE mem_pos = %d\n", mem_pos);
			fwrite(mem, 1, mem_pos, fp2);
			mem_pos = ioctl(fd,IOCTL_COPYWAIT, mem);
			fwrite(mem, 1, mem_pos, fp3);
			i++;
		}
	}

	ioctl(fd,IOCTL_INIT, -1);
	//fclose(fp);
	close(fd);
	return 0;
}
