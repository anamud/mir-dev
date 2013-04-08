#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "helper.h"
#include "alignment.h"

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 2)
        PABRT("Usage: %s input-file\n", argv[0]);

    // Init the runtime
    mir_create();

    pairalign_init(argv[1]);

    align_init();
    long par_time_start = get_usecs();
    align();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
        PDBG("Checking ... \n");
        align_seq_init();
        long seq_time_start = get_usecs();
        align_seq();
        long seq_time_end = get_usecs();
        double seq_time = (double)( seq_time_end - seq_time_start) / 1000000;
        check = align_verify();
        align_seq_deinit();
        PMSG("Seq. time=%f secs\n", seq_time);
#endif

    align_deinit();

    PMSG("%s(%s),check=%d in %s,time=%f secs\n", argv[0], argv[1], check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
