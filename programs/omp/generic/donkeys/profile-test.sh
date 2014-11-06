#!/bin/bash

APP=generic
#OUTLINE_FUNCTIONS=test._omp_fn.0,main._omp_fn.1
#OUTLINE_FUNCTIONS=test._omp_fn.0,test._omp_fn.1,test._omp_fn.2,test._omp_fn.3,main._omp_fn.4
OUTLINE_FUNCTIONS=test._omp_fn.0,test._omp_fn.1,test._omp_fn.2,test._omp_fn.3,test._omp_fn.4,main._omp_fn.5
CALLED_FUNCTIONS=process,preprocess
INPUT="4"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

