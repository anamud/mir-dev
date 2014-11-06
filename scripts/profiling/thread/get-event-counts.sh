#!/bin/bash

# Arguments check
if (( $# != 1 )); then
    echo "Usage: $0 prv-file"
    exit 1
fi

# Remove comment character
sed '/^#/ d' $1 > $1.templ
# Remove state information
sed '/^1:/ d' $1.templ > $1.tempr
# Get per thread event values
Rscript $MIR_ROOT/scripts/get-event-counts.R $1.tempr
# Cleanup
rm -f *.tempr *.templ


