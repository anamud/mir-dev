#!/bin/bash

APP=reduction
TASKS=for_task_wrapper,reduce_wrapper
CALLED_FUNCS=reduce,for_task
INPUT="5 512"
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

