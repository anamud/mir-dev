#!/bin/bash

APP=bintree
TASKS=ol_node_0,ol_node_1
CALLED_FUNCS=fib,node
INPUT="12 20"
MIR_CONF="-w=1 -i -g"
OPF="large"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
