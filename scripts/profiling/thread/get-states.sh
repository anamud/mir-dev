#!/bin/bash

# Get arguments
usage()
{
cat << EOF
usage: $0 -w FILE STATE-TIME-FILES

This script gets per-thread state statistics.

OPTIONS:
   -h      Show this message
   -w      Worker stats file (mir-worker-stats)
   STATE-TIME-FILES One or more state time files (*-state-time-*.rec)
EOF
}

WS=
while getopts "hw:" opt; do
  case "$opt" in
    h) usage ; exit 1 ;;
    w) WS=$OPTARG ;;
    ?) usage ; exit 1 ;;
  esac
done
shift $(( OPTIND - 1 ))

# Collect state time from all recorders
OUTF=accumulated-state-time.rec
rm -f $OUTF
cat $@ > $OUTF 
sed '/THREAD/d' $OUTF > $OUTF.body
sed '/THREAD/!d' $OUTF > $OUTF.header
LC_NUMERIC=C sort -n $OUTF.body > $OUTF.body.sorted
cat $OUTF.header $OUTF.body.sorted > $OUTF
rm -f $OUTF.header $OUTF.body*

# Process using R
rm -f $OUTF.info
SCRIPT="`readlink -e $0`"
SCRIPTPATH="`dirname $SCRIPT`"
Rscript $SCRIPTPATH/get-states.R accumulated-state-time.rec $WS

# Cleanup
rm -f $OUTF


