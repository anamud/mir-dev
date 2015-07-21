#!/bin/bash

cd ./test_1
echo In `pwd`
./test.sh
echo
cd ..

cd ./test_2
./test.sh
echo
cd ..

