#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "sparselu.h"
#include "mir_public_int.h"
#include "helper.h"

#define CHECK_RESULT 1

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
    {
        printf("Usage: %s NB BS \n", argv[0]);
        exit(1);
    }

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
    if (CHECK_RESULT)
    {
        sparselu_init(&SEQ, "serial");
        sparselu_seq_call(SEQ);
        sparselu_fini(SEQ, "serial");
        check = sparselu_check(SEQ, BENCH);
    }

    printf("%s(%d,%d),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], NB, BS, check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
