#!/bin/bash

APP=reduction
TASKS=for_task_wrapper,reduce_wrapper
CALLED_FUNCS=reduce,for_task
INPUT="8 512"
MIR_CONF="-w=1 -i -g"
OPF="small"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Rename stats file
mv mir-stats ${APP}_${OPF}_mir-stats

