#!/bin/bash
set -x

APP=strassen
OUTLINE_FUNCTIONS=OptimizedStrassenMultiply_par._omp_fn.0,OptimizedStrassenMultiply_par._omp_fn.1,OptimizedStrassenMultiply_par._omp_fn.2,OptimizedStrassenMultiply_par._omp_fn.3,OptimizedStrassenMultiply_par._omp_fn.4,OptimizedStrassenMultiply_par._omp_fn.5,OptimizedStrassenMultiply_par._omp_fn.6,strassen_main_par._omp_fn.7
CALLABLE_FUNCTIONS=matrixmul,FastNaiveMatrixMultiply,FastAdditiveNaiveMatrixMultiply,MultiplyByDivideAndConquer,OptimizedStrassenMultiply_seq,OptimizedStrassenMultiply_par,init_matrix,compare_matrix,alloc_matrix,strassen_main_par,strassen_main_seq
INPUT="4096 128 2000"
MIR_CONF="-w=1 -l=64 -i -g -p"
OFILE_PREFIX="5s"
PROCESS_PROFILE_DATA=1

# Profile and generate data
. ${MIR_ROOT}/programs/common/stub-profile.sh

