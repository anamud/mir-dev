#!/bin/bash

APP=sparselu
OUTLINE_FUNCTIONS=smp_ol_sparselu_par_call_0_unpacked,smp_ol_sparselu_par_call_1_unpacked,smp_ol_sparselu_par_call_2_unpacked
CALLED_FUNCTIONS=allocate_clean_block,lu0,bdiv,bmod,fwd
INPUT="64 64"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="medium"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

