#!/bin/bash

PROG=alignment-opt
STACK=64
SCHED=ws-de

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

INPUT="./inputs/prot.200.aa"
INPUTSPE="prot-200aa"
common
inflation
