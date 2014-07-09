#!/bin/bash

APP=generic
TASKS=test._omp_fn.0,main._omp_fn.1
CALLED_FUNCS=process,preprocess
INPUT="4"
MIR_CONF="-w=1 -i -g -p"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

