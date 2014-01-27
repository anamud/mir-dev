#!/bin/bash

APP=map-mc
TASKS=map_wrapper
CALLED_FUNCS=map
INPUT="4 4"
MIR_CONF="-w=1 -i -g -p"
OPF="test"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

