#!/bin/bash

# Arguments check
if (( $# == 0)); then
    echo "Usage: $0 suffix [task perf table] [task perf table field]"
    exit 1
fi

if (( $# > 3)); then
    echo "Usage: $0 suffix [task perf table] [task perf table field]"
    exit 1
fi

if (( $# == 2)); then
    echo "Usage: $0 suffix [task perf table] [task perf table field]"
    exit 1
fi

# Collect state time from all recorders
STFS=$1-recorder
OUTF=$1-events-acc.rec
rm -f $OUTF
cat $STFS-*.rec > $OUTF 
sed -n '/^e:/p' $OUTF > $OUTF.events
cat $OUTF.events > $OUTF
rm -f $OUTF.events
echo Wrote accumulated events to file $OUTF

# Get event information and summary
if (( $# == 3)); then
    Rscript $MIR_ROOT/scripts/get-events-per-task.R $OUTF $2 $3
else
    Rscript $MIR_ROOT/scripts/get-events-per-task.R $OUTF
fi
