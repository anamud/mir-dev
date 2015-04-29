#!/bin/bash

function omp_speedup()
{
    echo $msg
    for j in 1 48
    do
        echo $j core
        for i in 1 2 3 4 5 6 7 8 9 10
        do
            OMP_NUM_THREADS=$j numactl --interleave=0-7 $cmd
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
            #MIR_CONF="-w=$j -l=64 -s=ws -m=fine" $cmd
            MIR_CONF="-w=$j -l=64 -s=ws" $cmd
        done
    done
}

msg="ICC"
cmd="./sort-icc 16777216 262144 262144 128"
omp_speedup

msg="GCC"
cmd="./sort-gcc 16777216 262144 262144 128"
omp_speedup

msg="MIR"
cmd="./sort-opt 16777216 262144 262144 128"
mir_speedup

#msg="MIR"
#cmd="./sort-opt 16777216 262144 262144 128"
#mir_speedup

