#!/bin/bash

APP=strassen
TASKS=smp_ol_OptimizedStrassenMultiply_par_0_unpacked,smp_ol_OptimizedStrassenMultiply_par_1_unpacked,smp_ol_OptimizedStrassenMultiply_par_2_unpacked,smp_ol_OptimizedStrassenMultiply_par_3_unpacked,smp_ol_OptimizedStrassenMultiply_par_4_unpacked,smp_ol_OptimizedStrassenMultiply_par_5_unpacked,smp_ol_OptimizedStrassenMultiply_par_6_unpacked
CALLED_FUNCS=matrixmul,FastNaiveMatrixMultiply,FastAdditiveNaiveMatrixMultiply,MultiplyByDivideAndConquer,OptimizedStrassenMultiply_par
INPUT="1024 32 10"
MIR_CONF="-w=1 -i -g"
OPF="medium"
PROCESS_TASK_GRAPH=1

# Profile and generate data
. ${MIR_ROOT}/scripts/task-graph/stub-profile.sh
