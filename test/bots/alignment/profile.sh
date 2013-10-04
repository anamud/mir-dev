#!/bin/bash

APP=alignment
TASKS=smp_ol_pairalign_0_unpacked
CALLED_FUNCS=diff,add,del,forward_pass,reverse_pass,calc_score,tracepath
INPUT="./inputs/prot.50.aa"
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

