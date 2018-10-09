rm /tmp/target
rm /tmp/perf_target

pid=$(sudo jps | grep $1 | awk '{print $1}')
ps -T -p ${pid} | awk '{if (NR>1) print $2}' > /tmp/target
echo ${pid} >> /tmp/perf_target

#echo $pid > /tmp/target

for i in $2 $3 $4 $5 $6 $7
do
pgrep $i >> /tmp/target
pgrep $i >> /tmp/perf_target
done

pgrep ksoftirq >> /tmp/target
pgrep kworker >> /tmp/target

pgrep ksoftirq >> /tmp/perf_target
pgrep kworker >> /tmp/perf_target

make
exist=$(sudo lsmod | grep ioctl_perf | awk '{print $1}')
if [ ! "$exist" ]
then
sudo insmod ioctl_perf.ko
if [ ! -f /dev/wperf ]
then
sudo mknod /dev/wperf c 239 0
fi
sudo chmod 666 /dev/wperf
else
echo "Module has been installed."
fi
