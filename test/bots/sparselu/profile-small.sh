#!/bin/bash

APP=sparselu
TASKS=_smp__ol_sparselu_par_call_0,_smp__ol_sparselu_par_call_1,_smp__ol_sparselu_par_call_2,_smp__ol_sparselu_par_call_3
CALLED_FUNCS=allocate_clean_block,lu0,bdiv,bmod,fwd
INPUT="10 10"
MIR_CONF="-w=1 -i -g"
OPF="small"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
