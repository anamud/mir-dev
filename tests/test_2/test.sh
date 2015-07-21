#!/bin/bash

echo In `pwd`
echo Printing test information ...
cat test_2-info.txt
echo Rebuilding test cases quietly ...
scons -c -Q --quiet && scons -Q --quiet
echo Running tests ...
./test_2-opt > test-result
cat test-result
