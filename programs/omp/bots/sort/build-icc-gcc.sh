#!/bin/bash
set -x

gcc -Wall -fopenmp -O3 -o sort-gcc -I$MIR_ROOT/programs/common -Wno-unused-but-set-variable -Wno-unused-variable sort.c
icc -Wall -openmp -O3 -o sort-icc -I$MIR_ROOT/programs/common -Wno-unused-variable sort.c
