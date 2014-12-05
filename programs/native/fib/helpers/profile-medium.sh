#!/bin/bash

APP=fib
OUTLINE_FUNCTIONS=ol_fib_0,ol_fib_1
CALLED_FUNCTIONS=fib,fib_seq
INPUT="36 12"
MIR_CONF="-w=1 -i -g"
OFILE_PREFIX="medium"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

