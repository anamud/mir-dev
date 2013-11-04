#!/bin/bash

APP=floorplan
TASKS=smp_ol_add_cell_0_unpacked
CALLED_FUNCS=starts,lay_down,add_cell_ser,add_cell,memcpy
INPUT="./inputs/input.10 7"
MIR_CONF="-w=1 -i -g -l=128"
OPF="1s"
BIND_TASK_GRAPH=0
PLOT_TASK_GRAPH=0
SUMMARIZE_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

