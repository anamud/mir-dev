#!/bin/bash

APP=alignment
TASKS=smp_ol_pairalign_0_unpacked
CALLED_FUNCS=diff,add,del,forward_pass,reverse_pass,calc_score,tracepath
INPUT="./inputs/prot.20.aa"
MIR_CONF="-w=1 -i -g"
OPF="medium"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

