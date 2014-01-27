#!/bin/bash

APP=uts
TASKS=smp_ol_parTreeSearch_2_unpacked
CALLED_FUNCS=memcpy,rng_rand,rng_showstate,rng_showtype,sha1_compile,sha1_begin,sha1_hash,sha1_end,rng_spawn,rng_nextrand,rng_toProb,uts_numChildren_bin,uts_numChildren,parTreeSearch
INPUT="inputs/prof-test.input"
MIR_CONF="-w=1 -i -g -q=110000 -l=64 -p"
OPF="1s"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

