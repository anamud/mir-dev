#!/bin/bash

APP=generic
TASKS=process._omp_fn.0,process._omp_fn.2,process._omp_fn.1,test._omp_fn.3
CALLED_FUNCS=process,subprocess
INPUT="10"
MIR_CONF="-w=1 -i -g -p"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

