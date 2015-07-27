#!/bin/bash

for d in */ ;
do
    echo Entering directory $d
    pushd $d
    if [ -f test.sh ];
    then
        ./test.sh
        scons -uc
    fi
    popd
done
