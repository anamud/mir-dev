#!/bin/bash

APP=cpp-app
OUTLINE_FUNCTIONS=_ZN4POBJC2Ei._omp_fn.0,_ZN4POBJC2Ei._omp_fn.1
CALLED_FUNCTIONS=printf
INPUT=""
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

