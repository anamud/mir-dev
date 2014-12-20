#!/bin/bash

# Arguments check
if (($# == 0)); then
    echo "Usage: $0 recorder-files"
    echo "Recorder files have .rec as extension."
    exit 1
fi

# Collect state time from all recorders
OUTF=accumulated-events.rec
rm -f $OUTF
cat "$@" > $OUTF 
sed -n '/^e:/p' $OUTF > $OUTF.temp
cat $OUTF.temp > $OUTF
rm -f $OUTF.temp

# Get event information and summary
SCRIPT="`readlink -e $0`"
SCRIPTPATH="`dirname $SCRIPT`"
Rscript $SCRIPTPATH/get-events-per-task.R $OUTF

# Cleanup
rm -f $OUTF
