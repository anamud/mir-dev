#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "sparselu.h"
#include "mir_public_int.h"
#include "helper.h"

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

float **SEQ, **BENCH;
int NB, BS;

int main(int argc, char *argv[])
{/*{{{*/
    if (argc != 3)
        PABRT("Usage: %s NB BS \n", argv[0]);

    // Init the runtime
    mir_create();

    // Locals
    NB = atoi(argv[1]);
    BS = atoi(argv[2]);

    sparselu_init(&BENCH, "benchmark");
    long par_time_start = get_usecs();
    sparselu_par_call(BENCH);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;
    sparselu_fini(BENCH, "benchmark");

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    PDBG("Checking ... \n");
    sparselu_init(&SEQ, "serial");
    long seq_time_start = get_usecs();
    sparselu_seq_call(SEQ);
    long seq_time_end = get_usecs();
    double seq_time = (double)( seq_time_end - seq_time_start) / 1000000;
    sparselu_fini(SEQ, "serial");
    check = sparselu_check(SEQ, BENCH);
    PMSG("Seq. time=%f secs\n", seq_time);
#endif

    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], NB, BS, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
