#!/bin/bash

APP=nqueens
OUTLINE_FUNCTIONS=smp_ol_nqueens_0_unpacked 
CALLED_FUNCTIONS=nqueens,nqueens_ser
INPUT="10"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="small"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh
