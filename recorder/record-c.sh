#/bin/bash
# Arg1: Thread Name
# Arg2: Test Time
# Arg3: Disk Name
echo '0. Prepare perf probe and start module'
# Prepare perf probe
sudo rm /tmp/wperf-*
sudo perf probe 'try_to_wake_up pid=p->pid s1=p->state s2=state'
sudo dmesg -c
cd ../module
sh 1prepare.sh $1 $3
cd -

echo '1. Get threads information'
pid=$(pgrep $1)
echo "pid:" ${pid}
echo "time:" ${2}

cp /tmp/target target.tinfo
pgrep ksoftirq > soft.tinfo
#Ignore multiple theads

echo '2. Start module to record kernel events & record I/O'
nohup sudo ./recorder 0 $3 > /dev/null 2>&1 &

perflist=$(tr '\n' , < /tmp/perf_target | sed 's/,$//g')
echo $perflist
nohup sudo perf record -F $4 -e 'probe:try_to_wake_up' -e sched:sched_switch --call-graph dwarf -o presult -p ${perflist} sleep $2 > /dev/null 2>&1 &

hname=$(hostname)
ifstat -i $5 > nstat &
iostat $3 -d 1 > iostat &

sleep $2
sudo pkill recorder 
sudo pkill ifstat
sudo pkill iostat 

echo '3. Start gdb to record the function name of all threads'
sudo gdb -p $pid < gdb.script > gresult 2>&1

echo '4. Copy all the files to result directory.'
time=$(date +%Y-%m-%d-%H-%M)
mkdir $1-${2}-${time}
sudo mv target.tinfo $1-${2}-${time}/
sudo mv soft.tinfo $1-${2}-${time}/
sudo mv result* $1-${2}-${time}/
sudo mv presult $1-${2}-${time}/
sudo mv gresult $1-${2}-${time}/
sudo dmesg -c > $1-${2}-${time}/kmesg
awk '{if (NF==2) {a+=$2;b+=1;}} END{print "Network:", a/b}' nstat >> $1-${2}-${time}/kmesg

sudo mv nstat $1-${2}-${time}/
sudo mv iostat $1-${2}-${time}/

sh cpufreq.sh > cpufreq
sudo mv cpufreq $1-${2}-${time}/

sudo rm $1-${2}-${time}/result_fake
sudo cat /tmp/wperf-* > $1-${2}-${time}/result_fake   

echo '5. Finish'
