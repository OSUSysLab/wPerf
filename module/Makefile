obj-m = ioctl_perf.o

NEWKERNEL := $(shell uname -r | awk -F"." '{if (int($$1) >= 4 && int($$2) > 10) print "1"}')
KVERSION = $(shell uname -r)

ifdef NEWKERNEL 
ccflags-y	:= -DNEWKERNEL -Wno-unused-function -Wno-unused-variable -Wframe-larger-than=16782
else
ccflags-y	:= -Wno-unused-function -Wno-unused-variable -Wframe-larger-than=16782
endif

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
