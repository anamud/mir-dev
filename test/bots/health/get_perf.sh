#!/bin/bash

command="numactl --interleave=0-7 ./health-opt inputs/prof-5s.input"
command2="./health-prof inputs/prof-5s.input"
sched=ws

# Prepare
rm -f *.rec* mir-stats speedup.txt *.prv* *.pcf event-*
scons

# Collect detailed perf
echo "Collecting detailed performance ..."
MIR_CONF="-r -i -s=$sched" $command

# Post-process
echo "Processing detailed performance ..."
rf=`ls *-config.rec`
rfl=(${rf//-/ })
$MIR_ROOT/scripts/get-state-stats.sh $rfl
$MIR_ROOT/scripts/mirtoparaver.py *-config.rec
$MIR_ROOT/scripts/get-event-counts.sh *.prv

# Speedup
#echo "Running speedup test ..."
##for i in 1 2 4 8 16 32 48
#for i in 48
#do
#MIR_CONF="-w=$i -s=$sched" $command 2>&1 | tee -a speedup.txt
#MIR_CONF="-w=$i -s=$sched -r" $command 2>&1 | tee -a speedup.txt
#done 

#MIR_CONF="-w=1 -s=$sched" $command 2>&1 | tee -a speedup.txt
#MIR_CONF="-w=1 -s=$sched -g" $command 2>&1 | tee -a speedup.txt

# Copy perf files
max=`ls -1d perf_report_* | sed -e "s/perf_report_//g" | tr -dc '[0-9\n]' | sort -k 1,1n | tail -1`
outdir=perf_report_$((max + 1))
echo "Copying perf information to $outdir"
mkdir $outdir
mv *.rec* mir-stats *.prv *.pcf ./$outdir 
mv event-* ./$outdir
mv speedup.txt ./$outdir
cp get_perf.sh ./$outdir

