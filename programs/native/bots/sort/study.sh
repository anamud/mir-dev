#!/bin/bash

PROG=sort-opt
STACK=64
SCHED=ws
MEMPOL=fine

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
prolog
WORKER=24
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUT// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
prolog
WORKER=1
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUT// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
}

#INPUT="16777216 4096 4096 128"
#common

#INPUT="16777216 262144 262144 128"
#common
#inflation

INPUT="16777216 16384 16384 64"
common
inflation
