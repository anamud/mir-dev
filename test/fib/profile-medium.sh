#!/bin/bash

APP=fib
TASKS=fib
CALLED_FUNCS=fib_seq
INPUT="36 12"
MIR_CONF="-w=1 -i -g"
OPF="medium"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Rename stats file
mv mir-stats ${APP}_${OPF}_mir-stats
