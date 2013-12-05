#!/bin/bash

APP=reduction
TASKS=for_task_wrapper,reduce_wrapper
CALLED_FUNCS=reduce,for_task
INPUT="12 512"
MIR_CONF="-w=1 -i -g"
OPF="large"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

