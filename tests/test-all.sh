#!/bin/bash

num_trials=1
if [ $# -gt 0 ];
then num_trials=$1
fi
for d in */ ;
do
    echo Entering directory $d
    pushd $d > /dev/null
    if [ -f test.sh ];
    then
        ./test.sh $num_trials
    fi
    popd > /dev/null
done
