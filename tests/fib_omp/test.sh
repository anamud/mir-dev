#!/bin/bash

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
    MIR_CONF="--single-parallel-block" ./test-opt.out >> test-result.txt
    if [ $? -ne 0 ];
    then cat test-result.txt
         echo Test FAILED.
         exit 1
    fi
    ./test-opt.out >> test-result.txt
    if [ $? -ne 0 ];
    then cat test-result.txt
         echo Test FAILED.
         exit 1
    fi
done
echo "  Passed"
