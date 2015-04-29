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
cmd="./strassen-icc 8192 128"
omp_speedup

msg="GCC"
cmd="./strassen-gcc 8192 128 "
omp_speedup

msg="MIR central"
cmd="./strassen-opt 8192 128 "
sch=central
mir_speedup

msg="MIR central-stack"
cmd="./strassen-opt 8192 128 "
sch=central-stack
mir_speedup

msg="MIR ws"
cmd="./strassen-opt 8192 128 "
sch=ws-de
mir_speedup

msg="MIR ws-node"
cmd="./strassen-opt 8192 128 "
sch=ws-de-node
mir_speedup
