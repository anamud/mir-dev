#!/bin/bash

echo Rebuilding test cases quietly ...
scons -c -Q --quiet && scons -Q --quiet
echo Running tests ...
./test_1-opt > test-result
cat test-result
