#!/bin/bash

APP=nqueens
TASKS=smp_ol_nqueens_0_unpacked 
CALLED_FUNCS=nqueens,nqueens_ser
INPUT="5"
MIR_CONF="-w=1 -i -g"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

