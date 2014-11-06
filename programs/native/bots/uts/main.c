#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "uts.h"
#include "helper.h"

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

    uts_read_file(argv[1]);

    Node root;
    uts_initRoot(&root);

    long par_time_start = get_usecs();
    parallel_uts(&root);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    uts_show_stats();

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    PDBG("Checking ... \n");
    check = uts_check_result();
#endif

    PMSG("%s(%s),check=%d in %s,time=%f secs\n", argv[0], argv[1], check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
