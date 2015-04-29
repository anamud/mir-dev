#!/bin/bash

PROG=strassen-opt
STACK=64
SCHED=central-stack
MEMPOL=system

scons -c

source $MIR_ROOT/programs/common/stub-study.sh

function common ()
{
prolog
speedup 1 2 4 8 16 32 48
OUTDIR_PREFIX="${PROG}_${INPUT// /-}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_speedup"
prolog
WORKER=48
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUT// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
}

function inflation ()
{
#prolog
#WORKER=24
#recorder_and_stats
#OUTDIR_PREFIX="${PROG}_${INPUT// /-}_${WORKER}_${SCHED}_${STACK}"
#epilog "${OUTDIR_PREFIX}_recorder-stats"
prolog
WORKER=1
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUT// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
}

INPUT="2048 128 2000"
common
inflation

INPUT="2048 128 3"
common
inflation
