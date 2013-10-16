#!/bin/bash

APP=matmul
TASKS=compute_task_wrapper
CALLED_FUNCS=matmul,compute_task
INPUT="128"
MIR_CONF="-w=1 -i -g"
OPF="large"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

