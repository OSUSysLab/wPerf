sudo rm /tmp/target
sudo rm /tmp/perf_target

for i in $1 $2 $3 $4 $5
do
pgrep $i -w >> /tmp/target
pgrep $i >> /tmp/perf_target
done

pgrep ksoftirq >> /tmp/target
pgrep kworker >> /tmp/target

pgrep ksoftirq >> /tmp/perf_target
pgrep kworker >> /tmp/perf_target

exist=$(sudo lsmod | grep ioctl_perf | awk '{print $1}')
if [ ! "$exist" ]
then
make
sudo insmod ioctl_perf.ko
sudo mknod /dev/wperf c 239 0
sudo chmod 666 /dev/wperf
else
echo "Module has been installed."
fi
