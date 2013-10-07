#!/bin/bash

APP=bintree
TASKS=ol_node_0,ol_node_1
CALLED_FUNCS=fib,node
INPUT="8 20"
MIR_CONF="-w=1 -i -g"
OPF="medium"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

# Rename stats file
mv mir-stats ${APP}_${OPF}_mir-stats

