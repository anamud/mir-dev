#!/bin/bash

infile="tests"
outfile="screen_output"
store="test_results"
m2p="/local/ananya/mir/scripts/mirtoparaver.py"
ms="mir-stats"

#if [ -f ${outfile} ];
#then
    #echo "Deleting file" ${outfile}
    #rm -rf ${outfile}
#fi

mkdir -p ${store}

loopcnt=10
echo "Running each test" $loopcnt "times ..."
while read -r line
do
    if [ -n "${line}" ] && [ ${line:0:1} != "#" ];
    then
        echo ${line}
        conf_str=${line##*MIR_CONF=\"}
        conf_str=${conf_str%%\"*}
        conf_str="${conf_str// /_}"
        conf_str="${conf_str//-/}"
        conf_str="${conf_str//=/-}"
        conf_dir=${store}"/"${conf_str}
        echo "Storing results in " ${conf_dir}
        rm -rf ${conf_dir}
        mkdir ${conf_dir}
        rm -rf *.rec
        rm -rf mir-stats
        for(( i=1; i<=$loopcnt; i++ ))
        do
            rm -rf ${outfile}
            echo "Run " ${i}
            run_dir=${conf_dir}"/run-"${i}
            mkdir ${run_dir}
            echo ${line} > ${outfile}
            eval ${line} 2>&1 | tee -a ${outfile}
            if test -n "$(find . -maxdepth 1 -name '*-config.rec' -print -quit)"
            then
                echo "Converting recorder traces to paraver format ...!"
                ${m2p} *-config.rec ${conf_str} 1
                mv ${conf_str}-paraver.prv ${run_dir} 
                mv ${conf_str}-paraver.pcf ${run_dir} 
                #mv ${conf_str}.taskmetainfo ${run_dir} 
                #mv ${conf_str}.perfctrinfo ${run_dir} 
                rm -rf *.rec
            fi
            if [ -f ${ms} ];
            then
                mv ${ms} ${run_dir}
            fi
            mv ${outfile} ${run_dir}
        done
    fi
done < $infile
echo "Done!"
