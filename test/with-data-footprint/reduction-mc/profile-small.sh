#!/bin/bash

APP=reduction
TASKS=reduce_wrapper
CALLED_FUNCS=reduce
INPUT="8 512"
MIR_CONF="-w=1 -i -g"
OPF="small"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

