#!/bin/bash

APP=alignment
OUTLINE_FUNCTIONS=smp_ol_pairalign_0_unpacked
CALLED_FUNCTIONS=diff,add,del,forward_pass,reverse_pass,calc_score,tracepath
INPUT="./inputs/prot.400.aa"
MIR_CONF="-w=1 -i -g -p -q=90000"
OFILE_PREFIX="10s"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

