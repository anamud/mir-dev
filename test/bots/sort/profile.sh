#!/bin/bash

APP=sort
TASKS=_smp__ol_cilksort_par_2,_smp__ol_cilksort_par_3,_smp__ol_cilksort_par_4,_smp__ol_cilksort_par_5,_smp__ol_cilksort_par_6,_smp__ol_cilksort_par_7,_smp__ol_cilkmerge_par_1,_smp__ol_cilkmerge_par_2
CALLED_FUNCS=cilksort_par,med3,binsplit,choose_pivot,seqpart,seqquick,seqmerge,insertion_sort,cilkmerge_par,memcpy,memset
INPUT=2097151
PLOT_TASK_GRAPH=0

. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh

