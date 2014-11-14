#!/bin/bash

APP=fib
OUTLINE_FUNCTIONS=ol_fib_0,ol_fib_1,ol_fib_2
CALLED_FUNCTIONS=fib,fib_seq
INPUT="20 12"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="$APP-test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

