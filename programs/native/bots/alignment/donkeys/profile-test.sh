#!/bin/bash

APP=alignment
OUTLINE_FUNCTIONS=smp_ol_pairalign_0_unpacked
CALLED_FUNCTIONS=diff,add,del,forward_pass,reverse_pass,calc_score,tracepath
INPUT="./inputs/prot.10.aa"
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="test"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

