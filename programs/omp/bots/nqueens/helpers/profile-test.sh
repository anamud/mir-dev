#!/bin/bash
set -x

APP=nqueens
OUTLINE_FUNCTIONS=nqueens._omp_fn.0,find_queens._omp_fn.1
CALLED_FUNCTIONS=nqueens,nqueens_ser
INPUT="5"
MIR_CONF="-w=1 -l=64 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

