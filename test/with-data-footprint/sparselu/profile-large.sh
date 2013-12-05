#!/bin/bash

APP=sparselu
TASKS=smp_ol_sparselu_par_call_0_unpacked,smp_ol_sparselu_par_call_1_unpacked,smp_ol_sparselu_par_call_2_unpacked
CALLED_FUNCS=allocate_clean_block,lu0,bdiv,bmod,fwd
INPUT="128 64"
MIR_CONF="-w=1 -i -g"
OPF="large"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

