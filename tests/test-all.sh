#!/bin/bash

for d in */ ;
do
    echo Entering directory $d
    pushd $d > /dev/null
    if [ -f test.sh ];
    then
        ./test.sh
    fi
    popd > /dev/null
done
