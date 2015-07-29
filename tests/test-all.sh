#!/bin/bash

exitcode=0
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
        if [ $? -ne 0 ];
        then
            exitcode=1
        fi
    fi
    popd > /dev/null
done

if [ $exitcode -eq 0 ];
then
    echo "Testsuite PASSED."
else
    echo "Testsuite FAILED."
fi

exit $exitcode
