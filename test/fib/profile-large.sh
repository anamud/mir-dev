#!/bin/bash

APP=fib
TASKS=ol_fib_0,ol_fib_1
CALLED_FUNCS=fib,fib_seq
INPUT="42 12"
MIR_CONF="-w=1 -i -g"
OPF="large"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
