#!/bin/bash

OPTIND=1

debug_exe=""
outline_functions=""
verbose=0

show_help()
{
    echo "Usage: `basename $0` options (-f outline functions in comma seperated list) (-e executable with debug information) (-h for help) (-v for verbose)";
    exit $E_OPTERROR;
}

while getopts "hve:f:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    v)
        verbose=1
        ;;
    f)
        outline_functions=$OPTARG
        ;;
    e)
        debug_exe=$OPTARG
        ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
    esac
done

if [ $verbose -eq 1 ]; then
    echo Debug executable: $debug_exe
    echo Outline functions: $outline_functions
fi

outline_functions_ssv=$(echo $outline_functions | tr "," " ")

for of in $outline_functions_ssv;
do
    echo Outline function: $of is located by GDB at $(gdb $debug_exe -ex "set breakpoint pending on" -ex "b $of" --batch)
done

