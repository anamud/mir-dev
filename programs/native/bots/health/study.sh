#!/bin/bash

PROG=health-opt
STACK=64
SCHED=ws

scons -c

source $MIR_ROOT/programs/common/stub-study.sh

function common ()
{
prolog
speedup 1 2 4 8 16 32 48
OUTDIR_PREFIX="${PROG}_${INPUTSPE// /-}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_speedup"
prolog
WORKER=48
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUTSPE// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
}

function inflation ()
{
prolog
WORKER=24
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUTSPE// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
prolog
WORKER=1
recorder_and_stats
OUTDIR_PREFIX="${PROG}_${INPUTSPE// /-}_${WORKER}_${SCHED}_${STACK}"
epilog "${OUTDIR_PREFIX}_recorder-stats"
}

INPUT="inputs/prof-10s.input"
INPUTSPE="prof-10s"
common
inflation
