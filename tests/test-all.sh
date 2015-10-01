#!/bin/bash

exitcode=0
num_trials=1
if [ $# -gt 0 ];
then num_trials=$1
fi
for d in */ ;
do
    pushd $d > /dev/null
    # Override default behaviour
    if [ -f test.sh ];
    then
        ./test.sh $num_trials
        if [ $? -ne 0 ];
        then
            exitcode=1
        fi
    # Regular test.
    elif [ -f SConscript ];
    then
        cat test-info.txt
        scons -cu -Q --quiet &> /dev/null && scons -u -Q --quiet &> /dev/null
	if [ $? -ne 0 ];
	then echo Building test FAILED.
	     exitcode=1
	fi
	if [ -f test-opt.out ];
	then echo -n Running test ...
	     > test-result.txt
	     for i in `seq 1 $num_trials`;
	     do
		 echo -n "  trial $i ..."
		 ./test-opt.out >> test-result.txt
		 if [ $? -ne 0 ];
		 then cat test-result.txt
                     echo Test FAILED.
                     exitcode=1
		 fi
	     done
             echo " Passed"
        else
            echo "  Test NOT APPLICABLE"
	fi
    fi
    popd > /dev/null
done

if [ $exitcode -eq 0 ];
then
    echo "Testsuite PASSED."
else
    echo "Testsuite FAILED."
fi

exit $exitcode
