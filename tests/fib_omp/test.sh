#!/bin/bash

cat test-info.txt
echo Rebuilding test code quietly ...
scons -cu -Q --quiet &> /dev/null && scons -u -Q --quiet &> /dev/null
echo Running test ...
num_trials=1
if [ $# -gt 0 ];
then num_trials=$1
fi
> test-result.txt
for i in `seq 1 $num_trials`;
do
    echo Trial $i:
    MIR_CONF="--single-parallel-block" ./test-opt.out | tee -a test-result.txt
    if [ ${PIPESTATUS[0]} -ne 0 ];
    then echo Test FAILED.
    fi
done
