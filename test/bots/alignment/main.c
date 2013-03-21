#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "helper.h"
#include "alignment.h"

#define CHECK_RESULT 1

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 2)
    {
        printf("Usage: alignment input-file\n");
        exit(0);
    }

    // Init the runtime
    mir_create();

    pairalign_init(argv[1]);

    align_init();
    long par_time_start = get_usecs();
    align();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    if (CHECK_RESULT)
    {
        align_seq_init();
        align_seq();
        check = align_verify();
    }

    printf("%s(%s),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], argv[1], check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
