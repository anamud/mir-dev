#!/bin/bash

APP=fib
OUTLINE_FUNCTIONS=fib._omp_fn.1,fib._omp_fn.2,main._omp_fn.0
CALLED_FUNCTIONS=fib,fib_seq
INPUT="8 2"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

