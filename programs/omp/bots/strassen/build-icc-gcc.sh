#!/bin/bash
set -x

gcc -Wall -fopenmp -O3 -o strassen-gcc -I$MIR_ROOT/programs/common -Wno-unused-but-set-variable -Wno-unused-variable strassen.c
icc -Wall -openmp -O3 -o strassen-icc -I$MIR_ROOT/programs/common -Wno-unused-variable strassen.c
