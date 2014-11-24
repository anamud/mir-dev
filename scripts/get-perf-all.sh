#cd ${MIR_ROOT}/test/bots/alignment/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/sort/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/sparselu/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/health/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/fft/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/strassen/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/nqueens/ && ./get_perf.sh > get_perf.stdout 2>&1
##cd ${MIR_ROOT}/test/bots/uts/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/bots/floorplan/ && ./get_perf.sh > get_perf.stdout 2>&1
#cd ${MIR_ROOT}/test/fib/ && ./get_perf.sh > get_perf.stdout 2>&1

# IWOMP paper test
#cd ${MIR_ROOT}/test/with-data-footprint/map-mc && ./get_perf.sh > get_perf.stdout 2>&1
cd ${MIR_ROOT}/test/with-data-footprint/reduction-mc && ./get_perf.sh > get_perf.stdout 2>&1
cd ${MIR_ROOT}/test/with-data-footprint/jacobi-cont && ./get_perf.sh > get_perf.stdout 2>&1
cd ${MIR_ROOT}/test/with-data-footprint/matmul && ./get_perf.sh > get_perf.stdout 2>&1
cd ${MIR_ROOT}/test/with-data-footprint/sparselu && ./get_perf.sh > get_perf.stdout 2>&1
