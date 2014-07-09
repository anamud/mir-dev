#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <omp.h>

#include "mir_public_int.h"
#include "helper.h"

// Forward declarations
long get_usecs(void);
void preprocess(int p);
void process(int i, int n);
void test(int n);
int main(int argc, char **argv);

// Defines
#define REUSE_COUNT 2

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

void preprocess(int p)
{/*{{{*/
    printf("Preprocessing %d!\n", p);    
}/*}}}*/

void process(int i, int n)
{/*{{{*/
    /*if(i%2 == 0)*/
    /*{*/
/*#pragma omp task*/
        /*preprocess(0);*/
    /*}*/
/*#pragma omp task*/
        /*preprocess(1);*/
/*#pragma omp task*/
        /*preprocess(2);*/
/*#pragma omp taskwait*/

    printf("Processing %d-%d!\n", i, n);    
}/*}}}*/

void test(int n)
{/*{{{*/
    for(int r=0; r<REUSE_COUNT; r++)
    {
        for(int i=0; i<n; i++)
        {
            preprocess(r); 
#pragma omp task firstprivate(i) shared(n)
            process(i,n);
        }
#pragma omp taskwait
    }
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    // Init the runtime
    mir_create();

    if (argc > 2)
        PMSG("Usage: %s number\n", argv[0]);

    int num = 42;
    if(argc > 1)
        num = atoi(argv[1]);
    PMSG("Running %s %d ... \n", argv[0], num);

    long par_time_start = get_usecs();
#pragma omp task
{
    test(num);
    test(num-1);
}
#pragma omp taskwait
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    PMSG("%s(%d),check=%d in %s,time=%f secs\n", argv[0], num, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    mir_destroy();

    return 0;
}/*}}}*/
