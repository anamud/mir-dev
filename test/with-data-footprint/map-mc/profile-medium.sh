#!/bin/bash

APP=map-mc
TASKS=map_wrapper
CALLED_FUNCS=map
INPUT="192 128"
MIR_CONF="-w=1 -i -g -p"
OPF="medium"
PROCESS_TASK_GRAPH=1

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

