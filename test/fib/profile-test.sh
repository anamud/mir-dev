#!/bin/bash

APP=fib
TASKS=ol_fib_0,ol_fib_1,ol_fib_2
CALLED_FUNCS=fib,fib_seq
INPUT="10 2"
MIR_CONF="-w=1 -i -g -p"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

