#!/bin/bash

APP=sort
TASKS=_smp__ol_cilksort_par_2,_smp__ol_cilksort_par_3,_smp__ol_cilksort_par_4,_smp__ol_cilksort_par_5,_smp__ol_cilksort_par_6,_smp__ol_cilksort_par_7,_smp__ol_cilkmerge_par_1,_smp__ol_cilkmerge_par_2
CALLED_FUNCS=cilksort_par,med3,binsplit,choose_pivot,seqpart,seqquick,seqmerge,insertion_sort,cilkmerge_par,memcpy,memset
INPUT=16384
MIR_CONF="-w=1 -i -g"
OPF="test"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
