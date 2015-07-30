#!/bin/bash

sched_policies="central central-stack ws ws-de numa"

cat test-info.txt
scons -cu -Q --quiet &> /dev/null && scons -u -Q --quiet &> /dev/null
echo -n Running test ...
num_trials=1
if [ $# -gt 0 ];
then num_trials=$1
fi
> test-result.txt
for i in `seq 1 $num_trials`;
do
    echo -n "  trial $i ..."
    for p in $sched_policies;
    do
        MIR_CONF="-s $p" ./test-opt.out >> test-result.txt
        if [ $? -ne 0 ];
        then cat test-result.txt
             echo Test FAILED.
             exit 1
        fi
        MIR_CONF="-s $p -w 1 --stack-size=32" ./test-opt.out >> test-result.txt
        if [ $? -ne 0 ];
        then cat test-result.txt
             echo Test FAILED.
             exit 1
        fi
    done
done
echo "  Passed"
