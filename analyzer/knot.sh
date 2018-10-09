#!/bin/bash

echo "wPerf knot presenter:"
echo -n "Data directory:"
read -r dir
echo -n "Programming language[c or java, default c]:"
read -r lang
echo -n "Termination condition[default 0.2 (20% of total testing time)]:"
read -r tc
echo -n "Remove threshold[default 0.01 (1% of total testing time)]:"
read -r re 

if [ "$dir" == "" ]; then
echo "No input directory."
exit
fi

if [ "$lang" == "" ]; then
lang="c"
fi

if [ "$tc" == "" ]; then
tc=0.2
fi

if [ "$re" == "" ]; then
re=0.01
fi

python bottleneck.py $dir $lang $tc $re
