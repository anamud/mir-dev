#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "fft.h"
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
    if (argc > 2)
    {
        printf("Usage: %s number\n", argv[0]);
        exit(0);
    }

    // Init the runtime
    mir_create();

    int arg_size = 32*1024*1024;
    if(argc>1)
        arg_size = atoi(argv[0]);

    COMPLEX *in = NULL;
    COMPLEX *out1 = NULL;
    COMPLEX *out2 = NULL;

    in = malloc(arg_size * sizeof(COMPLEX));
    out1 = malloc(arg_size * sizeof(COMPLEX));
    for (int i = 0; i < arg_size; ++i)
    {
        in[i].re = 1.00000000000000000000000000000000000000000000000000000e+00;
        in[i].im = 1.00000000000000000000000000000000000000000000000000000e+00;
    }

    long par_time_start = get_usecs();
    fft(arg_size, in, out1);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    if (CHECK_RESULT)
    {
        out2 = malloc(arg_size * sizeof(COMPLEX));
        for (int i = 0; i < arg_size; ++i)
        {
            in[i].re = 1.00000000000000000000000000000000000000000000000000000e+00;
            in[i].im = 1.00000000000000000000000000000000000000000000000000000e+00;
        }
        fft_seq(arg_size, in, out2);
        check = test_correctness(arg_size, out1, out2);
        ;
    }

    printf("%s(%d),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], arg_size, check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
