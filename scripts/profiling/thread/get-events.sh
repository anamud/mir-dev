#!/bin/bash

# Arguments check
if (( $# != 1 )); then
    echo "Usage: $0 mir-recorder.prv"
    exit 1
fi

# Get PCF file
PCF_FILE=$(basename $1 .prv).pcf
echo "Events found in PCF:"
sed -n '/EVENT_TYPE/ {n;p}' $PCF_FILE

# Remove comment character
sed '/^#/ d' $1 > $1.templ

# Remove state information
sed '/^1:/ d' $1.templ > $1.tempr

# Write info
echo "Wrote temporary processed file: $1.tempr"

# Get per thread event values
SCRIPT="`readlink -e $0`"
SCRIPTPATH="`dirname $SCRIPT`"
Rscript $SCRIPTPATH/get-events.R $1.tempr

# Cleanup
rm -f *.tempr *.templ


