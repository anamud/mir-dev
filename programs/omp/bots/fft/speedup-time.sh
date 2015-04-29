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
            MIR_CONF="-w=$j -l=64 -s=ws" $cmd
        done
    done
}

msg="ICC default"
cmd="./fft-icc 16777216 128 40"
omp_speedup

msg="ICC cutoff"
cmd="./fft-icc 16777216 4096 2"
omp_speedup

msg="GCC default"
cmd="./fft-gcc 16777216 128 40"
omp_speedup

msg="GCC cutoff"
cmd="./fft-gcc 16777216 8192 2"
omp_speedup

msg="MIR default"
cmd="./fft-opt 16777216 128 40"
mir_speedup

msg="MIR cutoff"
cmd="./fft-opt 16777216 8192 2"
mir_speedup
