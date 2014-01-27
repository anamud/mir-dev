#!/bin/bash

APP=map-mc
TASKS=map_wrapper
CALLED_FUNCS=map
INPUT="384 128"
MIR_CONF="-w=1 -i -g -p"
OPF="large"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

