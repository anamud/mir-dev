#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "fft.h"
#include "helper.h"

int magic_cutoff = 128; // Original 
int aux_cutoff = 40; // Maximum size of factors array

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

struct main_task_wrapper_arg_t 
{/*{{{*/
    int n; COMPLEX * in; COMPLEX * out;
};/*}}}*/

void main_task_wrapper(void* arg)
{/*{{{*/
    struct main_task_wrapper_arg_t* warg = (struct main_task_wrapper_arg_t*) arg;
    fft(warg->n, warg->in, warg->out);
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 4)
        PABRT("Usage: %s number magic_cutoff aux_cutoff\n", argv[0]);

    // Init the runtime
    mir_create();

    int arg_size = 8*1024*1024;
    if(argc>1)
        arg_size = atoi(argv[1]);
    if(argc>2)
        magic_cutoff = atoi(argv[2]);
    if(argc>3)
        aux_cutoff = atoi(argv[3]);

    COMPLEX *in = NULL;
    COMPLEX *out1 = NULL;
    COMPLEX *out2 = NULL;

    in = mir_mem_pol_allocate (arg_size * sizeof(COMPLEX));
    out1 = mir_mem_pol_allocate (arg_size * sizeof(COMPLEX));
    for (int i = 0; i < arg_size; ++i)
    {
        in[i].re = 1.00000000000000000000000000000000000000000000000000000e+00;
        in[i].im = 1.00000000000000000000000000000000000000000000000000000e+00;
    }

    long par_time_start = get_usecs();
    struct main_task_wrapper_arg_t mt_arg;
    mt_arg.n = arg_size;
    mt_arg.in = in;
    mt_arg.out = out1;
    mir_task_create((mir_tfunc_t) main_task_wrapper, &mt_arg, sizeof(struct main_task_wrapper_arg_t), 0, NULL, "main_task_wrapper");
    mir_task_wait();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    out2 = mir_mem_pol_allocate (arg_size * sizeof(COMPLEX));
    for (int i = 0; i < arg_size; ++i)
    {
        in[i].re = 1.00000000000000000000000000000000000000000000000000000e+00;
        in[i].im = 1.00000000000000000000000000000000000000000000000000000e+00;
    }
    long seq_time_start = get_usecs();
    fft_seq(arg_size, in, out2);
    long seq_time_end = get_usecs();
    double seq_time = (double)( seq_time_end - seq_time_start) / 1000000;
    check = test_correctness(arg_size, out1, out2);
    mir_mem_pol_release (out2, arg_size * sizeof(COMPLEX));
    PMSG("Seq. time=%f secs\n", seq_time);
#endif

    PMSG("%s(%d),check=%d in %s,time=%f secs\n", argv[0], arg_size, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Release memory 
    mir_mem_pol_release (in, arg_size * sizeof(COMPLEX));
    mir_mem_pol_release (out1, arg_size * sizeof(COMPLEX));

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
