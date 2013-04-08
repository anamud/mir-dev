#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "helper.h"
#include "sparselu.h"
#include "mir_public_int.h"

extern int NB, BS;

int checkmat (float *M, float *N)
{/*{{{*/
    int i, j;
    float r_err;

    for (i = 0; i < BS; i++)
    {
        for (j = 0; j < BS; j++)
        {
            r_err = M[i*BS+j] - N[i*BS+j];
            if ( r_err == 0.0 ) continue;

            if (r_err < 0.0 ) r_err = -r_err;

            if ( M[i*BS+j] == 0 ) 
            {
                PMSG("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; \n",
                        i,j, M[i*BS+j], i,j, N[i*BS+j]);
                return FALSE;
            }  
            r_err = r_err / M[i*BS+j];
            if(r_err > EPSILON)
            {
                PMSG("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; Relative Error=%f\n",
                        i,j, M[i*BS+j], i,j, N[i*BS+j], r_err);
                return FALSE;
            }
        }
    }
    return TRUE;
}/*}}}*/

void genmat (float *M[])
{/*{{{*/
    int null_entry, init_val, i, j, ii, jj;
    float *p;

    init_val = 1325;

    /* generating the structure */
    for (ii=0; ii < NB; ii++)
    {
        for (jj=0; jj < NB; jj++)
        {
            /* computing null entries */
            null_entry=FALSE;
            if ((ii<jj) && (ii%3 !=0)) null_entry = TRUE;
            if ((ii>jj) && (jj%3 !=0)) null_entry = TRUE;
            if (ii%2==1) null_entry = TRUE;
            if (jj%2==1) null_entry = TRUE;
            if (ii==jj) null_entry = FALSE;
            if (ii==jj-1) null_entry = FALSE;
            if (ii-1 == jj) null_entry = FALSE; 
            /* allocating matrix */
            if (null_entry == FALSE){
                M[ii*NB+jj] = (float *) mir_mem_pol_allocate (BS*BS*sizeof(float));
                if ((M[ii*NB+jj] == NULL))
                {
                    PMSG("Error: Out of memory\n");
                    exit(101);
                }
                /* initializing matrix */
                p = M[ii*NB+jj];
                for (i = 0; i < BS; i++) 
                {
                    for (j = 0; j < BS; j++)
                    {
                        init_val = (3125 * init_val) % 65536;
                        (*p) = (float)((init_val - 32768.0) / 16384.0);
                        p++;
                    }
                }
            }
            else
            {
                M[ii*NB+jj] = NULL;
            }
        }
    }
}/*}}}*/

void print_structure(char *name, float *M[])
{/*{{{*/
    return;
    int ii, jj;
    PMSG("Structure for matrix %s @ 0x%p\n",name, M);
    for (ii = 0; ii < NB; ii++) {
        for (jj = 0; jj < NB; jj++) {
            if (M[ii*NB+jj]!=NULL) {PMSG("x");}
            else PMSG(" ");
        }
        PMSG("\n");
    }
    PMSG("\n");
}/*}}}*/

float * allocate_clean_block()
{/*{{{*/
    int i,j;
    float *p, *q;

    p = (float *) mir_mem_pol_allocate (BS*BS*sizeof(float));
    q=p;
    if (p!=NULL){
        for (i = 0; i < BS; i++) 
            for (j = 0; j < BS; j++){(*p)=0.0; p++;}

    }
    else
    {
        PMSG("Error: Out of memory\n");
        exit (101);
    }
    return (q);
}/*}}}*/

void lu0(float *diag)
{/*{{{*/
    int i, j, k;

    for (k=0; k<BS; k++)
        for (i=k+1; i<BS; i++)
        {
            diag[i*BS+k] = diag[i*BS+k] / diag[k*BS+k];
            for (j=k+1; j<BS; j++)
                diag[i*BS+j] = diag[i*BS+j] - diag[i*BS+k] * diag[k*BS+j];
        }
}/*}}}*/

void bdiv(float *diag, float *row)
{/*{{{*/
    int i, j, k;
    for (i=0; i<BS; i++)
        for (k=0; k<BS; k++)
        {
            row[i*BS+k] = row[i*BS+k] / diag[k*BS+k];
            for (j=k+1; j<BS; j++)
                row[i*BS+j] = row[i*BS+j] - row[i*BS+k]*diag[k*BS+j];
        }
}/*}}}*/

void bmod(float *row, float *col, float *inner)
{/*{{{*/
    int i, j, k;
    for (i=0; i<BS; i++)
        for (j=0; j<BS; j++)
            for (k=0; k<BS; k++)
                inner[i*BS+j] = inner[i*BS+j] - row[i*BS+k]*col[k*BS+j];
}/*}}}*/

void fwd(float *diag, float *col)
{/*{{{*/
    int i, j, k;
    for (j=0; j<BS; j++)
        for (k=0; k<BS; k++) 
            for (i=k+1; i<BS; i++)
                col[i*BS+j] = col[i*BS+j] - diag[i*BS+k]*col[k*BS+j];
}/*}}}*/

void sparselu_init (float ***pBENCH, char *pass)
{/*{{{*/
    *pBENCH = (float **) malloc (NB*NB*sizeof(float *));
    genmat(*pBENCH);
    print_structure(pass, *pBENCH);
}/*}}}*/

void sparselu_par_call(float **bench);

typedef struct _nx_data_env_0_t_tag
{/*{{{*/
    int *NB_0;
    float ***bench_0;
    int jj_0;
    int kk_0;
} _nx_data_env_0_t;/*}}}*/

static void _smp__ol_sparselu_par_call_0(void* arg)
{/*{{{*/
    _nx_data_env_0_t* _args = (_nx_data_env_0_t* ) arg;  
    int *NB_0 = (int *) (_args->NB_0);
    float ***bench_0 = (float ***) (_args->bench_0);
    {
        fwd((*bench_0)[(_args->kk_0) * (*NB_0) + (_args->kk_0)], (*bench_0)[(_args->kk_0) * (*NB_0) + (_args->jj_0)]);
    }
}/*}}}*/

typedef struct _nx_data_env_1_t_tag
{/*{{{*/
    int *NB_0;
    float ***bench_0;
    int ii_0;
    int kk_0;
} _nx_data_env_1_t;/*}}}*/

static void _smp__ol_sparselu_par_call_1(void* arg)
{/*{{{*/
    _nx_data_env_1_t* _args = (_nx_data_env_1_t* ) arg; 
    int *NB_0 = (int *) (_args->NB_0);
    float ***bench_0 = (float ***) (_args->bench_0);
    {
        bdiv((*bench_0)[(_args->kk_0) * (*NB_0) + (_args->kk_0)], (*bench_0)[(_args->ii_0) * (*NB_0) + (_args->kk_0)]);
    }
}/*}}}*/

typedef struct _nx_data_env_2_t_tag
{/*{{{*/
    int *NB_0;
    float ***bench_0;
    int ii_0;
    int jj_0;
    int kk_0;
} _nx_data_env_2_t;/*}}}*/

static void _smp__ol_sparselu_par_call_2(void* arg)
{/*{{{*/
    _nx_data_env_2_t* _args = (_nx_data_env_2_t* ) arg; 
    int *NB_0 = (int *) (_args->NB_0);
    float ***bench_0 = (float ***) (_args->bench_0);
    {
        if ((*bench_0)[(_args->ii_0) * (*NB_0) + (_args->jj_0)] == ((void *) 0))
            (*bench_0)[(_args->ii_0) * (*NB_0) + (_args->jj_0)] = allocate_clean_block();
        bmod((*bench_0)[(_args->ii_0) * (*NB_0) + (_args->kk_0)], (*bench_0)[(_args->kk_0) * (*NB_0) + (_args->jj_0)], (*bench_0)[(_args->ii_0) * (*NB_0) + (_args->jj_0)]);
    }
}/*}}}*/

typedef struct _nx_data_env_3_t_tag
{/*{{{*/
    int *NB_0;
    float **bench_0;
    int ii_0;
    int jj_0;
    int kk_0;
} _nx_data_env_3_t;/*}}}*/

static void _smp__ol_sparselu_par_call_3(void* arg)
{/*{{{*/
    // Task wait counter 
    struct mir_twc_t* twc = mir_twc_create();

    _nx_data_env_3_t* _args = (_nx_data_env_3_t* ) arg; 
    int *NB_0 = (int *) (_args->NB_0);

    for ((_args->kk_0) = 0; (_args->kk_0) < (*NB_0); (_args->kk_0)++)
    {
        lu0((_args->bench_0)[(_args->kk_0) * (*NB_0) + (_args->kk_0)]);

        for ((_args->jj_0) = (_args->kk_0) + 1; (_args->jj_0) < (*NB_0); (_args->jj_0)++)
            if ((_args->bench_0)[(_args->kk_0) * (*NB_0) + (_args->jj_0)] != ((void *) 0))
            {
                _nx_data_env_0_t imm_args_0;
                imm_args_0.NB_0 = &((*NB_0));
                imm_args_0.bench_0 = &((_args->bench_0));
                imm_args_0.jj_0 = (_args->jj_0);
                imm_args_0.kk_0 = (_args->kk_0);

                struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) _smp__ol_sparselu_par_call_0, (void*) &imm_args_0, sizeof(_nx_data_env_0_t), twc, 0, NULL, NULL);
            }

        for ((_args->ii_0) = (_args->kk_0) + 1; (_args->ii_0) < (*NB_0); (_args->ii_0)++)
            if ((_args->bench_0)[(_args->ii_0) * (*NB_0) + (_args->kk_0)] != ((void *) 0))
            {
                _nx_data_env_1_t imm_args_1;
                imm_args_1.NB_0 = &((*NB_0));
                imm_args_1.bench_0 = &((_args->bench_0));
                imm_args_1.ii_0 = (_args->ii_0);
                imm_args_1.kk_0 = (_args->kk_0);

                struct mir_task_t* task_1 = mir_task_create((mir_tfunc_t) _smp__ol_sparselu_par_call_1, (void*) &imm_args_1, sizeof(_nx_data_env_1_t), twc, 0, NULL, NULL);
            }

        // Task wait 
        mir_twc_wait(twc);

        for ((_args->ii_0) = (_args->kk_0) + 1; (_args->ii_0) < (*NB_0); (_args->ii_0)++)
            if ((_args->bench_0)[(_args->ii_0) * (*NB_0) + (_args->kk_0)] != ((void *) 0))
                for ((_args->jj_0) = (_args->kk_0) + 1; (_args->jj_0) < (*NB_0); (_args->jj_0)++)
                    if ((_args->bench_0)[(_args->kk_0) * (*NB_0) + (_args->jj_0)] != ((void *) 0))
                    {
                        _nx_data_env_2_t imm_args_2;
                        imm_args_2.NB_0 = &((*NB_0));
                        imm_args_2.bench_0 = &((_args->bench_0));
                        imm_args_2.ii_0 = (_args->ii_0);
                        imm_args_2.jj_0 = (_args->jj_0);
                        imm_args_2.kk_0 = (_args->kk_0);

                        struct mir_task_t* task_2 = mir_task_create((mir_tfunc_t) _smp__ol_sparselu_par_call_2, (void*) &imm_args_2, sizeof(_nx_data_env_2_t), twc, 0, NULL, NULL);
                    }

        // Task wait
        mir_twc_wait(twc);
    }
}/*}}}*/

void sparselu_par_call(float **bench)
{/*{{{*/
    // Task wait counter
    struct mir_twc_t* twc = mir_twc_create();
    int ii = 0, jj = 0, kk = 0;

    // Create task
    _nx_data_env_3_t imm_args_3;
    imm_args_3.NB_0 = &(NB);
    imm_args_3.bench_0 = bench;
    imm_args_3.ii_0 = ii;
    imm_args_3.jj_0 = jj;
    imm_args_3.kk_0 = kk;

    struct mir_task_t* task_3 = mir_task_create((mir_tfunc_t) _smp__ol_sparselu_par_call_3, (void*) &imm_args_3, sizeof(_nx_data_env_3_t), twc, 0, NULL, NULL);

    // Task wait
    mir_twc_wait(twc);
}/*}}}*/

void sparselu_seq_call(float **bench)
{/*{{{*/
    int ii, jj, kk;
    for (kk = 0; kk < NB; kk++)
    {
        lu0(bench[kk * NB + kk]);

        for (jj = kk + 1; jj < NB; jj++)
            if (bench[kk * NB + jj] != ((void *) 0))
            {
                fwd(bench[kk * NB + kk], bench[kk * NB + jj]);
            }

        for (ii = kk + 1; ii < NB; ii++)
            if (bench[ii * NB + kk] != ((void *) 0))
            {
                bdiv(bench[kk * NB + kk], bench[ii * NB + kk]);
            }

        for (ii = kk + 1; ii < NB; ii++)
            if (bench[ii * NB + kk] != ((void *) 0))
                for (jj = kk + 1; jj < NB; jj++)
                    if (bench[kk * NB + jj] != ((void *) 0))
                    {
                        if (bench[ii * NB + jj] == ((void *) 0))
                            bench[ii * NB + jj] = allocate_clean_block();

                        bmod(bench[ii * NB + kk], bench[kk * NB + jj], bench[ii * NB + jj]);
                    }
    }
}/*}}}*/

void sparselu_fini(float **bench, char *pass)
{/*{{{*/
    print_structure(pass, bench);
}/*}}}*/

int sparselu_check(float **seq, float **bench)
{/*{{{*/
    int ii, jj, ok = 1;

    for (ii = 0; ((ii < NB) && ok); ii++)
    {
        for (jj = 0; ((jj < NB) && ok); jj++)
        {
            if ((seq[ii * NB + jj] == ((void *) 0)) && (bench[ii * NB + jj] != ((void *) 0)))
                ok = 0;
            if ((seq[ii * NB + jj] != ((void *) 0)) && (bench[ii * NB + jj] == ((void *) 0)))
                ok = 0;
            if ((seq[ii * NB + jj] != ((void *) 0)) && (bench[ii * NB + jj] != ((void *) 0)))
                ok = checkmat(seq[ii * NB + jj], bench[ii * NB + jj]);
        }
    }

    if (ok)
        return TEST_SUCCESSFUL;
    else
        return TEST_UNSUCCESSFUL;
}/*}}}*/
