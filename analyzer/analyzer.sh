#!/bin/bash
echo "wPerf analzyer:"
echo -n "Data directory:"
read -r dir
echo -n "Programming language[c or java, default c]:"
read -r lang
echo -n "User annotation[fake(false wakeup), busy(spinlock), or no, default no]:"
read -r uanno
echo -n "Analyzer worker number[default 32]:"
read -r nworker
echo -n "Merge similarity for long term threads[default 0.45]"
read -r lsim

if [ "$dir" == "" ]; then
echo "No input directory."
exit
fi

if [ "$lang" == "" ]; then
lang="c"
fi

if [ "$uanno" == "" ]; then
uanno="no"
fi

if [ "$nworker" == "" ]; then
nworker=32
fi

if [ "$lsim" == "" ]; then
lsim=0.45
fi

rm $dir/waitfor $dir/waitfor.bak

echo '0. Backup old target.tinfo'
cp $dir/target.tinfo $dir/target.tinfo.bak
grep creates $dir/kmesg | awk '{if (NF>9) {print $8} else {print $7}}' | sort | uniq >> $dir/target.tinfo

echo '1. Generate perf function stacks'
cd $dir
#sudo cp perf-*.map /tmp/
sudo perf script -i presult > presult.out
echo "1 2 pid= 3" >> presult.out
cd -

echo '2. Generate breakdown file and wait-for info'
make
wMem=$(free -mh | grep Mem | awk '{print tolower($2)}')
time java -cp ".:/tmp/uds.jar" -Xms${wMem} Analyzer $dir/result_switch $dir/result_softirq $dir/result_fake $dir/target.tinfo $dir/presult.out $dir/cpufreq $dir/waitfor.bak $dir/breakdown $uanno $nworker
echo "1 2 pid= 3" >> presult.final
mv presult.final $dir/
cp $dir/waitfor.bak $dir/waitfor

echo '3. Generate the merge info'
if [ $lang == "java" ]
then
python change-jstack.py $dir/gresult > $dir-gresult
mv $dir-gresult $dir/gresult
fi

python ./merge.py $dir/presult.out $lsim > $dir/merge.tinfo

echo '4. Start process short-term processes'
cp short.py $dir/
cd $dir
python short.py > waitfor-1
python short.py 1 >> merge.tinfo
mv waitfor-1 waitfor
cp target.tinfo.bak target.tinfo
cd -
