#!/bin/bash

APP=fib
TASKS=ol_fib_0,ol_fib_1
CALLED_FUNCS=fib,fib_seq
INPUT="30 12"
MIR_CONF="-w=1 -i -g"
OPF="small"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Rename stats file
mv mir-stats ${APP}_${OPF}_mir-stats
