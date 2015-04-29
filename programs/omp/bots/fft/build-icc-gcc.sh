#!/bin/bash
set -x

gcc -Wall -fopenmp -O3 -o fft-gcc -I$MIR_ROOT/programs/common -Wno-unused-but-set-variable -Wno-unused-variable -std=c99 fft-cutoff.c -lm
icc -Wall -openmp -O3 -o fft-icc -I$MIR_ROOT/programs/common -Wno-unused-variable -std=c99 fft-cutoff.c -lm
