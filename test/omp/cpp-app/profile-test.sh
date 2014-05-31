#!/bin/bash

APP=cpp-app
TASKS=_ZN4POBJC2Ei._omp_fn.0,_ZN4POBJC2Ei._omp_fn.1
CALLED_FUNCS=printf
INPUT=""
MIR_CONF="-w=1 -i -g -p"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

