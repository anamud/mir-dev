#!/bin/bash

APP=jacobi
TASKS=jacobi_block_wrapper,for_task_wrapper
CALLED_FUNCS=jacobi_point,jacobi_block,for_task
INPUT="512 32"
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

