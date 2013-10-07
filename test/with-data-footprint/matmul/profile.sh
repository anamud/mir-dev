#!/bin/bash

APP=matmul
TASKS=compute_task_wrapper
CALLED_FUNCS=matmul,compute_task
INPUT="128"
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

