#!/bin/bash

# Arguments check
if (( $# != 1 )); then
    echo "Usage: $0 suffix"
    exit 1
fi

# Collect state time from all recorders
STFS=$1-state-time
OUTF=state-time-acc.rec
rm -f $OUTF
cat $STFS-*.rec > $OUTF 
sed '/THREAD/d' $OUTF > $OUTF.body
sed '/THREAD/!d' $OUTF > $OUTF.header
LC_NUMERIC=C sort -n $OUTF.body > $OUTF.body.sorted
cat $OUTF.header $OUTF.body.sorted > $OUTF
rm -f $OUTF.header $OUTF.body*

# Process using R
rm -f $OUTF.info
Rscript $MIR_ROOT/scripts/get-state-stats.R state-time-acc.rec mir-stats
