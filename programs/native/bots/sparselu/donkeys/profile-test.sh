#!/bin/bash

APP=sparselu
OUTLINE_FUNCTIONS=_smp__ol_sparselu_par_call_0,_smp__ol_sparselu_par_call_1,_smp__ol_sparselu_par_call_2,_smp__ol_sparselu_par_call_3
CALLED_FUNCTIONS=allocate_clean_block,lu0,bdiv,bmod,fwd
INPUT="25 25"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

