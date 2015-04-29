#!/bin/bash

function omp_speedup()
{
    echo $msg
    for j in 1 48
    do
        echo $j core
        for i in 1 2 3 4 5 6 7 8 9 10
        do
            OMP_NUM_THREADS=$j $cmd
        done
    done
}

function mir_speedup()
{
    echo $msg
    for j in 1 48
    do
        echo $j core
        for i in 1 2 3 4 5 6 7 8 9 10
        do
            MIR_CONF="-w=$j -l=64 -s=$sch" $cmd
        done
    done
}

msg="ICC"
cmd="./sparselu-opt-icc 64 64"
omp_speedup

msg="GCC"
cmd="./sparselu-opt-gcc 64 64"
omp_speedup

msg="GCC-O2"
cmd="./sparselu-opt-gcc-O2 64 64"
omp_speedup

msg="MIR"
cmd="./sparselu-opt-opt 64 64"
sch=ws-de
mir_speedup

