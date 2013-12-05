#!/bin/bash

APP=map-mc
TASKS=map_wrapper
CALLED_FUNCS=map
INPUT="4 10"
MIR_CONF="-w=1 -i -g"
OPF="small"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

