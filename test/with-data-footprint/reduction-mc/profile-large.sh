#!/bin/bash

APP=reduction
TASKS=reduce_wrapper
CALLED_FUNCS=reduce
INPUT="12 512"
MIR_CONF="-w=1 -i -g"
OPF="large"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Rename stats file
mv mir-stats ${APP}_${OPF}_mir-stats

