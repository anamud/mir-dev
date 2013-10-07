#!/bin/bash

APP=sparselu
TASKS=_smp__ol_sparselu_par_call_0,_smp__ol_sparselu_par_call_1,_smp__ol_sparselu_par_call_2,_smp__ol_sparselu_par_call_3
CALLED_FUNCS=allocate_clean_block,lu0,bdiv,bmod,fwd
INPUT="128 64"
MIR_CONF="-w=1 -i -g"
OPF="large"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Rename stats file
mv mir-stats ${APP}_${OPF}_mir-stats
