#!/bin/bash

APP=nqueens
TASKS=smp_ol_nqueens_0_unpacked 
CALLED_FUNCS=nqueens,nqueens_ser
INPUT="8"
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
