#!/bin/bash

APP=uts
OUTLINE_FUNCTIONS=smp_ol_parTreeSearch_2_unpacked
CALLED_FUNCTIONS=memcpy,rng_rand,rng_showstate,rng_showtype,sha1_compile,sha1_begin,sha1_hash,sha1_end,rng_spawn,rng_nextrand,rng_toProb,uts_numChildren_bin,uts_numChildren,parTreeSearch
INPUT="inputs/prof-10s.input"
MIR_CONF="-w=1 -i -g -q=510000 -l=256 -p"
OFILE_PREFIX="10s"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

