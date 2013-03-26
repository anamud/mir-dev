#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "health.h"
#include "helper.h"

#define CHECK_RESULT 1
int cutoff_value = 2;

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
        printf("Usage: uts input-file\n");
        exit(0);
    }

    // Init the runtime
    mir_create();

    struct Village *top;
    read_input_data(argv[1]);
    allocate_village(&top, (void *)0, (void *)0, sim_level, 0);

    long par_time_start = get_usecs();
    sim_village_main_par(top);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    if (CHECK_RESULT)
    {
        check = check_village(top);
    }

    printf("%s(%s),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], argv[1], check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
