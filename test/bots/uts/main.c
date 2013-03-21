#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <alloca.h>

#include "mir_public_int.h"
#include "uts.h"
#include "helper.h"

#define CHECK_RESULT 1

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    // Init the runtime
    mir_create();

    if (argc > 2)
    {
        printf("Usage: uts input-file\n");
        exit(0);
    }

    uts_read_file(argv[1]);

    Node root;
    uts_initRoot(&root);

    long par_time_start = get_usecs();
    parallel_uts(&root);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    uts_show_stats();

    int check = TEST_NOT_PERFORMED;
    if (CHECK_RESULT)
    {
        check = uts_check_result();
    }

    printf("%s(%s),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], argv[1], check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
