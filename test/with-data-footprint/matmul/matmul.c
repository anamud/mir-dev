#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <mkl_cblas.h> // COMMENT THIS FOR NOT USE CBLAS

#include "mir_public_int.h"

#ifndef BSIZE
#define BSIZE 128
#endif

#define CHECK 0

float **A;
float **B;
float **C;
float **C_seq;

/*// CODE WITHOUT CBLAS

#pragma omp task input([NB][NB]A, [NB][NB]B) inout([NB][NB]C)
void matmul(float  *A, float *B, float *C, unsigned long NB)
{
    int i, j, k, I;
    float tmp;
    for (i = 0; i < NB; i++)
    {
        I=i*NB;
        for (j = 0; j < NB; j++)
        {
            tmp=C[I+j];
            for (k = 0; k < NB; k++)
                tmp+=A[I+k]*B[k*NB+j];
            C[I+j]=tmp;
        }
    }
}
*/

//#pragma omp task input([NB][NB]A, [NB][NB]B) inout([NB][NB]C)
void matmul(float *A, float *B, float *C, unsigned long NB)
{/*{{{*/
    unsigned char TR='T', NT='N';
    float DONE=1.0;

    cblas_sgemm(
            CblasRowMajor,
            CblasNoTrans, CblasNoTrans,
            NB, NB, NB,
            1.0, A, NB,
            B, NB,
            1.0, C, NB);

}/*}}}*/

struct matmul_wrapper_arg_t 
{/*{{{*/
    float* A;
    float* B;
    float* C;
    unsigned long NB;
};/*}}}*/

void matmul_wrapper(void* arg)
{/*{{{*/
    struct matmul_wrapper_arg_t* warg = (struct matmul_wrapper_arg_t*)(arg);
    matmul(warg->A, warg->B, warg->C, warg->NB);
}/*}}}*/

void compute_seq(struct timeval *start, struct timeval *stop, unsigned long NB, unsigned long DIM, float *A[DIM][DIM], float *B[DIM][DIM], float *C[DIM][DIM])
{/*{{{*/
    gettimeofday(start,NULL);

    for (unsigned long i = 0; i < DIM; i++)
        for (unsigned long j = 0; j < DIM; j++)
            for (unsigned long k = 0; k < DIM; k++)
                matmul (A[i][k], B[k][j], C[i][j], NB);

    gettimeofday(stop,NULL);
}/*}}}*/

int check(unsigned long DIM)
{/*{{{*/
    for (unsigned long i = 0; i < DIM*DIM; i++)
            for(unsigned long j=0; j<BSIZE*BSIZE; j++)
            {
                if(fabsf(C[i][j]- C_seq[i][j]) > 1e-3)
                {
                    fprintf(stderr, "%f != %f \n", C[i][j], C_seq[i][j]);
                    return 1;
                }
            }

    return 0;
}/*}}}*/

void compute_task(unsigned long i, unsigned long j, unsigned long NB, unsigned long DIM, float *A[DIM][DIM], float *B[DIM][DIM], float *C[DIM][DIM])
{/*{{{*/
#if 0
    struct mir_twc_t* twc = mir_twc_create();
    for (unsigned long k = 0; k < DIM; k++)
    {
//#pragma omp task firstprivate(i,j,k)
        //matmul (A[i][k], B[k][j], C[i][j], NB);

        // Data env
        struct matmul_wrapper_arg_t arg;
        arg.A =  A[i][k];
        arg.B = B[k][j];
        arg.C = C[i][j];
        arg.NB = NB;

        // Data footprint
        struct mir_data_footprint_t footprints[4];
        footprints[0].base = A[i][k];
        footprints[0].start = 0;
        footprints[0].end = (NB*NB)-1;
        footprints[0].row_sz = 1;
        footprints[0].type = sizeof(float);
        footprints[0].data_access = MIR_DATA_ACCESS_READ;
        footprints[0].part_of = A[i][k];

        footprints[1].base = B[k][j];
        footprints[1].start = 0;
        footprints[1].end = (NB*NB)-1;
        footprints[1].row_sz = 1;
        footprints[1].type = sizeof(float);
        footprints[1].data_access = MIR_DATA_ACCESS_READ;
        footprints[1].part_of = B[k][j];

        footprints[2].base = C[i][j];
        footprints[2].start = 0;
        footprints[2].end = (NB*NB)-1;
        footprints[2].row_sz = 1;
        footprints[2].type = sizeof(float);
        footprints[2].data_access = MIR_DATA_ACCESS_READ;
        footprints[2].part_of = C[i][j];

        footprints[3].base = C[i][j];
        footprints[3].start = 0;
        footprints[3].end = (NB*NB)-1;
        footprints[3].row_sz = 1;
        footprints[3].type = sizeof(float);
        footprints[3].data_access = MIR_DATA_ACCESS_WRITE;
        footprints[3].part_of = C[i][j];

        struct mir_task_t* task = mir_task_create((mir_tfunc_t) matmul_wrapper, &arg, sizeof(struct matmul_wrapper_arg_t), twc, 4, footprints, NULL);
        mir_twc_wait(twc);
    }
#else
    for (unsigned long k = 0; k < DIM; k++)
    {
        matmul (A[i][k], B[k][j], C[i][j], NB);
    }
#endif

}/*}}}*/

struct compute_task_wrapper_arg_t
{/*{{{*/
    unsigned long i;
    unsigned long j;
    unsigned long NB;
    unsigned long DIM;
    void *A;
    void *B;
    void *C;
};/*}}}*/

void compute_task_wrapper(void* arg)
{/*{{{*/
    struct compute_task_wrapper_arg_t* warg = (struct compute_task_wrapper_arg_t*)(arg);
    compute_task(warg->i, warg->j, warg->NB, warg->DIM, warg->A, warg->B, warg->C);
}/*}}}*/

void compute(struct timeval *start, struct timeval *stop, unsigned long NB, unsigned long DIM, float *A[DIM][DIM], float *B[DIM][DIM], float *C[DIM][DIM])
{/*{{{*/
    gettimeofday(start,NULL);

    struct mir_twc_t* twc = mir_twc_create();
    for (unsigned long i = 0; i < DIM; i++)
        for (unsigned long j = 0; j < DIM; j++)
        {
            struct compute_task_wrapper_arg_t arg;
            arg.i = i;
            arg.j = j;
            arg.NB = NB;
            arg.DIM = DIM;
            arg.A = (void*)A;
            arg.B = (void*)B;
            arg.C = (void*)C;

            struct mir_data_footprint_t footprints[DIM+DIM+2];
            int f=0;

            // Footprint for A[i][k]
            for(int l=0;l<DIM;l++,f++)
            {
                footprints[f].base = A[i][l];
                footprints[f].start = 0;
                footprints[f].end = (NB*NB)-1;
                footprints[f].row_sz = 1;
                footprints[f].type = sizeof(float);
                footprints[f].data_access = MIR_DATA_ACCESS_READ;
                footprints[f].part_of = A[i][l];
            }

            // Footprint for B[k][j]
            for(int l=0;l<DIM;l++,f++)
            {
                footprints[f].base = B[l][j];
                footprints[f].start = 0;
                footprints[f].end = (NB*NB)-1;
                footprints[f].row_sz = 1;
                footprints[f].type = sizeof(float);
                footprints[f].data_access = MIR_DATA_ACCESS_READ;
                footprints[f].part_of = B[l][j];
            }

            // Footprint for C[i][j]
            footprints[f].base = C[i][j];
            footprints[f].start = 0;
            footprints[f].end = (NB*NB)-1;
            footprints[f].row_sz = 1;
            footprints[f].type = sizeof(float);
            footprints[f].data_access = MIR_DATA_ACCESS_READ;
            footprints[f].part_of = C[i][j];
            f++;
            footprints[f].base = C[i][j];
            footprints[f].start = 0;
            footprints[f].end = (NB*NB)-1;
            footprints[f].row_sz = 1;
            footprints[f].type = sizeof(float);
            footprints[f].data_access = MIR_DATA_ACCESS_WRITE;
            footprints[f].part_of = C[i][j];

            struct mir_task_t* task = mir_task_create((mir_tfunc_t) compute_task_wrapper, &arg, sizeof(struct compute_task_wrapper_arg_t), twc, DIM+DIM+2, footprints, NULL);
        }

//#pragma omp taskwait
    mir_twc_wait(twc);
    gettimeofday(stop,NULL);
}/*}}}*/

static void convert_to_blocks(unsigned long NB,unsigned long DIM, unsigned long N, float *Alin, float *A[DIM][DIM])
{/*{{{*/
    for (unsigned long i = 0; i < N; i++)
    {
        for (unsigned long j = 0; j < N; j++)
        {
            A[i/NB][j/NB][(i%NB)*NB+j%NB] = Alin[j*N+i];
        }
    }

}/*}}}*/

void fill_random(float *Alin, int NN)
{/*{{{*/
    int i;
    for (i = 0; i < NN; i++)
    {
        Alin[i]=((float)rand())/((float)RAND_MAX);
    }
}/*}}}*/

void slarnv_(unsigned long *idist, unsigned long *iseed, unsigned long *n, float *x);

void deinit(unsigned long DIM)
{/*{{{*/
    for (unsigned long i = 0; i < DIM*DIM; i++)
    {
        mir_mem_pol_release(A[i], BSIZE*BSIZE*sizeof(float));
        mir_mem_pol_release(B[i], BSIZE*BSIZE*sizeof(float));
        mir_mem_pol_release(C[i], BSIZE*BSIZE*sizeof(float));
    }

    free(A);
    free(B);
    free(C);
}/*}}}*/

void deinit_seq(unsigned long DIM)
{/*{{{*/
    for (unsigned long i = 0; i < DIM*DIM; i++)
        free(C_seq[i]);

    free(C_seq);
}/*}}}*/

void init (unsigned long argc, char **argv, unsigned long * N_p, unsigned long * DIM_p)
{/*{{{*/
    unsigned long ISEED[4] = {0,0,0,1};
    unsigned long IONE=1;
    unsigned long DIM;
    char UPLO='n';
    float FZERO=0.0;

    if (argc==2)
    {
        DIM=atoi(argv[1]);
    }
    else
    {
        printf("usage: %s DIM\n",argv[0]);
        exit(0);
    }

    // matrix init
    unsigned long N=BSIZE*DIM;
    unsigned long NN=N*N;

    *N_p=N;
    *DIM_p=DIM;

    // linear matrix
    float *Alin = (float *) malloc(NN * sizeof(float));
    float *Blin = (float *) malloc(NN * sizeof(float));
    float *Clin = (float *) malloc(NN * sizeof(float));

    // fill the matrix with random values
    srand(0);
    fill_random(Alin,NN);
    fill_random(Blin,NN);
    for (int i=0; i < NN; i++)
        Clin[i]=0.0;

    A = (float **) malloc(DIM*DIM*sizeof(float *));
    B = (float **) malloc(DIM*DIM*sizeof(float *));
    C = (float **) malloc(DIM*DIM*sizeof(float *));

    for (unsigned long i = 0; i < DIM*DIM; i++)
    {
        A[i] = (float *) mir_mem_pol_allocate(BSIZE*BSIZE*sizeof(float));
        B[i] = (float *) mir_mem_pol_allocate(BSIZE*BSIZE*sizeof(float));
        C[i] = (float *) mir_mem_pol_allocate(BSIZE*BSIZE*sizeof(float));
    }

    convert_to_blocks(BSIZE,DIM, N, Alin, (void *)A);
    convert_to_blocks(BSIZE,DIM, N, Blin, (void *)B);
    convert_to_blocks(BSIZE,DIM, N, Clin, (void *)C);

    free(Alin);
    free(Blin);
    free(Clin);
}/*}}}*/

void init_seq (unsigned long N, unsigned long DIM)
{/*{{{*/
    unsigned long NN=N*N;
    float *Clin = (float *) malloc(NN * sizeof(float));
    for (int i=0; i < NN; i++)
        Clin[i]=0.0;

    C_seq = (float **) malloc(DIM*DIM*sizeof(float *));
    for (unsigned long i = 0; i < DIM*DIM; i++)
        C_seq[i] = (float *) malloc (BSIZE*BSIZE*sizeof(float));

    convert_to_blocks(BSIZE,DIM, N, Clin, (void *)C_seq);

    free(Clin);
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    // local vars
    unsigned long N, DIM;

    struct timeval start;
    struct timeval stop;
    unsigned long elapsed;

    mir_create();

    // application inicializations
    init(argc, argv, &N, &DIM);

    // compute
    compute(&start, &stop,(unsigned long) BSIZE, DIM, (void *)A, (void *)B, (void *)C);

    if(CHECK)
    {
        struct timeval start_seq;
        struct timeval stop_seq;
        unsigned long elapsed_seq;
        init_seq(N, DIM);
        compute_seq(&start_seq, &stop_seq,(unsigned long) BSIZE, DIM, (void *)A, (void *)B, (void *)C_seq);
        int result = check(DIM);
        deinit_seq(DIM);
        if(result == 0)
            printf("Check successful!\n");
        else
            printf("Check UNSUCCESSFUL :-(\n");
        elapsed_seq = 1000000 * (stop_seq.tv_sec - start_seq.tv_sec);
        elapsed_seq += stop_seq.tv_usec - start_seq.tv_usec;
        printf ("Seq. time %f secs\n", (double)(elapsed_seq)/1000000);
    }

    elapsed = 1000000 * (stop.tv_sec - start.tv_sec);
    elapsed += stop.tv_usec - start.tv_usec;
    printf("Matrix dimension: %ld\n",N);
    // time in usecs
    printf ("Time %f secs\n", (double)(elapsed)/1000000);
    // performance in MFLOPS
    printf("Perf %lu MFlops\n", (unsigned long)((((double)N)*((double)N)*((double)N)*2)/elapsed));
    printf("Perf %lu MBytes/s\n", (unsigned long)((((double)N)*((double)N)*2*sizeof(float))/elapsed));

    deinit(DIM);

    mir_destroy();

    return 0;
}/*}}}*/

