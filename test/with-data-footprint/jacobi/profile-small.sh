#!/bin/bash

APP=jacobi
TASKS=jacobi_block_wrapper,for_task_wrapper
CALLED_FUNCS=jacobi_point,jacobi_block,for_task
INPUT="2048 64"
MIR_CONF="-w=1 -i -g"
OPF="small"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
