#!/bin/bash

APP=sort
OUTLINE_FUNCTIONS=_smp__ol_cilksort_par_2,_smp__ol_cilksort_par_3,_smp__ol_cilksort_par_4,_smp__ol_cilksort_par_5,_smp__ol_cilksort_par_6,_smp__ol_cilksort_par_7,_smp__ol_cilkmerge_par_1,_smp__ol_cilkmerge_par_2
CALLED_FUNCTIONS=cilksort_par,med3,binsplit,choose_pivot,seqpart,seqquick,seqmerge,insertion_sort,cilkmerge_par,memcpy,memset
INPUT=16777216
MIR_CONF="-w=1 -i -g -p"
OFILE_PREFIX="large"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

