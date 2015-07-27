#!/bin/bash

cat test-info.txt
echo Rebuilding test code quietly ...
scons -cu -Q --quiet && scons -u -Q --quiet
echo Running test ...
./test-opt.out | tee test-result.txt
if [ ${PIPESTATUS[0]} -ne 0 ];
then echo Test FAILED.
fi
