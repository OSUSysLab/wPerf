#define IOC_MAGIC 'f'
#define IOCTL_ADDUDS _IOWR(IOC_MAGIC, 20, unsigned long)

typedef struct udsResult {
  unsigned long ts;
  int pid;
  long address;
  short type;
} __attribute__((packed)) uds_res;
