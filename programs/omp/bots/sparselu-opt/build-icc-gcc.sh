#!/bin/bash
set -x

gcc -Wall -fopenmp -O3 -DENABLE_SPECOMP12_MOD -o sparselu-opt-gcc -I$MIR_ROOT/programs/common -Wno-unused-variable sparselu.c
gcc -Wall -fopenmp -O2 -DENABLE_SPECOMP12_MOD -o sparselu-opt-gcc-O2 -I$MIR_ROOT/programs/common -Wno-unused-variable sparselu.c
icc -Wall -openmp -O3 -DENABLE_SPECOMP12_MOD -o sparselu-opt-icc -I$MIR_ROOT/programs/common -Wno-unused-variable sparselu.c
