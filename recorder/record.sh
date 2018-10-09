#!/bin/bash
echo "wPerf recorder:"
echo -n "Thread name[part is find, but still need unique]:"
read -r tname
echo -n "Programming language[c or java, default c]:"
read -r lang
echo -n "Profile length[default 90]:"
read -r length 
echo -n "Perf frequency[default 100]:"
read -r pfreq
echo -n "Disk[default sda]:"
read -r pdisk
echo -n "NIC[default eth0]:"
read -r pnic

if [ "$tname" == "" ]; then
echo "No thread name."
exit
fi

if [ "$lang" == "" ]; then
lang="c"
fi

if [ "$length" == "" ]; then
length=90
fi

if [ "$pfreq" == "" ]; then
pfreq=100
fi

if [ "$pdisk" == "" ]; then
pdisk="sda"
fi

if [ "$pnic" == "" ]; then
pnic="eth0"
fi

echo $lang
if [ "$lang" == "c" ]; then
./record-c.sh ${tname} ${length} ${pdisk} ${pfreq} ${pnic}
else
./record-java.sh ${tname} ${length} ${pdisk} ${pfreq} ${pnic}
fi
