/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify                      */
/*  it under the terms of the GNU General Public License as published by                      */
/*  the Free Software Foundation; either version 2 of the License, or                         */
/*  (at your option) any later version.                                                       */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful,                           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             */
/*  GNU General Public License for more details.                                              */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License                         */
/*  along with this program; if not, write to the Free Software                               */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            */
/**********************************************************************************************/

/* Ananya Muddukrishna (ananya@kth.se) ported to MIR */

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

#include "helper.h"
#include "sparselu.h"
#include "mir_public_int.h"

//#define HINT_ONLY_ACCESS_INTENSIVE_FOOTPRINTS 1

int BS = 256;
int NB = 64;
float **SEQ, **BENCH;

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

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
    *pBENCH = (float **) malloc(NB*NB*sizeof(float *));
    genmat(*pBENCH);
    print_structure(pass, *pBENCH);
}/*}}}*/

/* Task specific */
struct  nanos_args_0_t
{/*{{{*/
  int *NB;
  float ***BENCH;
  int jj;
  int kk;
};/*}}}*/

struct  nanos_args_1_t
{/*{{{*/
  int *NB;
  float ***BENCH;
  int ii;
  int kk;
};/*}}}*/

struct  nanos_args_2_t
{/*{{{*/
  int *NB;
  float ***BENCH;
  int ii;
  int jj;
  int kk;
};/*}}}*/

static void smp_ol_sparselu_par_call_0_unpacked(int *const NB, float ***const BENCH, int *const jj, int *const kk)
{/*{{{*/
  {
    {
      fwd((*BENCH)[(*kk) * (*NB) + (*kk)], (*BENCH)[(*kk) * (*NB) + (*jj)]);
    }
  }
}/*}}}*/

static void smp_ol_sparselu_par_call_0(struct nanos_args_0_t *const args)
{/*{{{*/
  {
    smp_ol_sparselu_par_call_0_unpacked((*args).NB, (*args).BENCH, &((*args).jj), &((*args).kk));
  }
}/*}}}*/

static void smp_ol_sparselu_par_call_1_unpacked(int *const NB, float ***const BENCH, int *const ii, int *const kk)
{/*{{{*/
  {
    {
      bdiv((*BENCH)[(*kk) * (*NB) + (*kk)], (*BENCH)[(*ii) * (*NB) + (*kk)]);
    }
  }
}/*}}}*/

static void smp_ol_sparselu_par_call_1(struct nanos_args_1_t *const args)
{/*{{{*/
  {
    smp_ol_sparselu_par_call_1_unpacked((*args).NB, (*args).BENCH, &((*args).ii), &((*args).kk));
  }
}/*}}}*/

static void smp_ol_sparselu_par_call_2_unpacked(int *const NB, float ***const BENCH, int *const ii, int *const jj, int *const kk)
{/*{{{*/
  {
    {
      /*if ((*BENCH)[(*ii) * (*NB) + (*jj)] == (void *)0)*/
        /*(*BENCH)[(*ii) * (*NB) + (*jj)] = allocate_clean_block();*/
      bmod((*BENCH)[(*ii) * (*NB) + (*kk)], (*BENCH)[(*kk) * (*NB) + (*jj)], (*BENCH)[(*ii) * (*NB) + (*jj)]);
    }
  }
}/*}}}*/

static void smp_ol_sparselu_par_call_2(struct nanos_args_2_t *const args)
{/*{{{*/
  {
    smp_ol_sparselu_par_call_2_unpacked((*args).NB, (*args).BENCH, &((*args).ii), &((*args).jj), &((*args).kk));
  }
}/*}}}*/

void sparselu_par_call(float **BENCH)
{/*{{{*/
    int ii, jj, kk;

    PMSG("Computing SparseLU Factorization (%dx%d matrix with %dx%d blocks) ",
            NB,NB,BS,BS);
    struct mir_twc_t* twc = mir_twc_create();
    for (kk=0; kk<NB; kk++)
    {
        lu0(BENCH[kk*NB+kk]);
        for (jj=kk+1; jj<NB; jj++)
            if (BENCH[kk*NB+jj] != NULL)
//#pragma omp task firstprivate(kk, jj) shared(BENCH)
            {
                //fwd(BENCH[kk*NB+kk], BENCH[kk*NB+jj]);
                {
                    // Data env
                    struct nanos_args_0_t imm_args;
                    imm_args.NB = &NB;
                    imm_args.BENCH = &BENCH;
                    imm_args.jj = jj;
                    imm_args.kk = kk;

                    // Footprint
#ifdef HINT_ONLY_ACCESS_INTENSIVE_FOOTPRINTS
                    const int num_footprints = 2;
                    struct mir_data_footprint_t footprints[num_footprints];

                    footprints[0].base = BENCH[kk*NB+jj];
                    footprints[0].start = 0;
                    footprints[0].end = BS*BS-1;
                    footprints[0].row_sz = 1;
                    footprints[0].type = sizeof(float);
                    footprints[0].data_access = MIR_DATA_ACCESS_READ;
                    footprints[0].part_of = BENCH[kk*NB+jj];

                    footprints[1].base = BENCH[kk*NB+jj];
                    footprints[1].start = 0;
                    footprints[1].end = BS*BS-1;
                    footprints[1].row_sz = 1;
                    footprints[1].type = sizeof(float);
                    footprints[1].data_access = MIR_DATA_ACCESS_WRITE;
                    footprints[1].part_of = BENCH[kk*NB+jj];
#else
                    const int num_footprints = 3;
                    struct mir_data_footprint_t footprints[num_footprints];

                    footprints[0].base = BENCH[kk*NB+kk];
                    footprints[0].start = 0;
                    footprints[0].end = BS*BS-1;
                    footprints[0].row_sz = 1;
                    footprints[0].type = sizeof(float);
                    footprints[0].data_access = MIR_DATA_ACCESS_READ;
                    footprints[0].part_of = BENCH[kk*NB+kk];

                    footprints[1].base = BENCH[kk*NB+jj];
                    footprints[1].start = 0;
                    footprints[1].end = BS*BS-1;
                    footprints[1].row_sz = 1;
                    footprints[1].type = sizeof(float);
                    footprints[1].data_access = MIR_DATA_ACCESS_READ;
                    footprints[1].part_of = BENCH[kk*NB+jj];

                    footprints[2].base = BENCH[kk*NB+jj];
                    footprints[2].start = 0;
                    footprints[2].end = BS*BS-1;
                    footprints[2].row_sz = 1;
                    footprints[2].type = sizeof(float);
                    footprints[2].data_access = MIR_DATA_ACCESS_WRITE;
                    footprints[2].part_of = BENCH[kk*NB+jj];
#endif

                    struct mir_task_t* task = mir_task_create((mir_tfunc_t) smp_ol_sparselu_par_call_0, (void*) &imm_args, sizeof(struct nanos_args_0_t), twc, num_footprints, footprints, NULL);
                }
            }

        for (ii=kk+1; ii<NB; ii++)
            if (BENCH[ii*NB+kk] != NULL)
//#pragma omp task firstprivate(kk, ii) shared(BENCH)
            {
                //bdiv (BENCH[kk*NB+kk], BENCH[ii*NB+kk]);
                {
                    // Data env
                    struct nanos_args_1_t imm_args;
                    imm_args.NB = &NB;
                    imm_args.BENCH = &BENCH;
                    imm_args.ii = ii;
                    imm_args.kk = kk;

                    // Footprint
#ifdef HINT_ONLY_ACCESS_INTENSIVE_FOOTPRINTS 
                    const int num_footprints = 2;
                    struct mir_data_footprint_t footprints[num_footprints];

                    footprints[0].base = BENCH[ii*NB+kk];
                    footprints[0].start = 0;
                    footprints[0].end = BS*BS-1;
                    footprints[0].row_sz = 1;
                    footprints[0].type = sizeof(float);
                    footprints[0].data_access = MIR_DATA_ACCESS_READ;
                    footprints[0].part_of = BENCH[ii*NB+kk];

                    footprints[1].base = BENCH[ii*NB+kk];
                    footprints[1].start = 0;
                    footprints[1].end = BS*BS-1;
                    footprints[1].row_sz = 1;
                    footprints[1].type = sizeof(float);
                    footprints[1].data_access = MIR_DATA_ACCESS_WRITE;
                    footprints[1].part_of = BENCH[ii*NB+kk];
#else
                    const int num_footprints = 3;
                    struct mir_data_footprint_t footprints[num_footprints];

                    footprints[0].base = BENCH[kk*NB+kk];
                    footprints[0].start = 0;
                    footprints[0].end = BS*BS-1;
                    footprints[0].row_sz = 1;
                    footprints[0].type = sizeof(float);
                    footprints[0].data_access = MIR_DATA_ACCESS_READ;
                    footprints[0].part_of = BENCH[kk*NB+kk];

                    footprints[1].base = BENCH[ii*NB+kk];
                    footprints[1].start = 0;
                    footprints[1].end = BS*BS-1;
                    footprints[1].row_sz = 1;
                    footprints[1].type = sizeof(float);
                    footprints[1].data_access = MIR_DATA_ACCESS_READ;
                    footprints[1].part_of = BENCH[ii*NB+kk];

                    footprints[2].base = BENCH[ii*NB+kk];
                    footprints[2].start = 0;
                    footprints[2].end = BS*BS-1;
                    footprints[2].row_sz = 1;
                    footprints[2].type = sizeof(float);
                    footprints[2].data_access = MIR_DATA_ACCESS_WRITE;
                    footprints[2].part_of = BENCH[ii*NB+kk];
#endif

                    struct mir_task_t* task = mir_task_create((mir_tfunc_t) smp_ol_sparselu_par_call_1, (void*) &imm_args, sizeof(struct nanos_args_1_t), twc, num_footprints, footprints, NULL);
                }
            }

//#pragma omp taskwait
        mir_twc_wait(twc);

        for (ii=kk+1; ii<NB; ii++)
            if (BENCH[ii*NB+kk] != NULL)
                for (jj=kk+1; jj<NB; jj++)
                    if (BENCH[kk*NB+jj] != NULL)
                    {
                        if (BENCH[ii*NB+jj]==NULL)
                            BENCH[ii*NB+jj] = allocate_clean_block();

//#pragma omp task firstprivate(kk, jj, ii) shared(BENCH)
                        //bmod(BENCH[ii*NB+kk], BENCH[kk*NB+jj], BENCH[ii*NB+jj]);
                        {
                            // Data env
                            struct nanos_args_2_t imm_args;
                            imm_args.NB = &NB;
                            imm_args.BENCH = &BENCH;
                            imm_args.ii = ii;
                            imm_args.jj = jj;
                            imm_args.kk = kk;

                            // Footprint
#ifdef HINT_ONLY_ACCESS_INTENSIVE_FOOTPRINTS 
                            const int num_footprints = 2;
                            struct mir_data_footprint_t footprints[num_footprints];
                            footprints[0].base = BENCH[ii*NB+jj];
                            footprints[0].start = 0;
                            footprints[0].end = BS*BS-1;
                            footprints[0].row_sz = 1;
                            footprints[0].type = sizeof(float);
                            footprints[0].data_access = MIR_DATA_ACCESS_READ;
                            footprints[0].part_of = BENCH[ii*NB+jj];

                            footprints[1].base = BENCH[ii*NB+jj];
                            footprints[1].start = 0;
                            footprints[1].end = BS*BS-1;
                            footprints[1].row_sz = 1;
                            footprints[1].type = sizeof(float);
                            footprints[1].data_access = MIR_DATA_ACCESS_WRITE;
                            footprints[1].part_of = BENCH[ii*NB+jj];
#else
                            const int num_footprints = 4;
                            struct mir_data_footprint_t footprints[num_footprints];
                            footprints[0].base = BENCH[ii*NB+kk];
                            footprints[0].start = 0;
                            footprints[0].end = BS*BS-1;
                            footprints[0].row_sz = 1;
                            footprints[0].type = sizeof(float);
                            footprints[0].data_access = MIR_DATA_ACCESS_READ;
                            footprints[0].part_of = BENCH[ii*NB+kk];

                            footprints[1].base = BENCH[kk*NB+jj];
                            footprints[1].start = 0;
                            footprints[1].end = BS*BS-1;
                            footprints[1].row_sz = 1;
                            footprints[1].type = sizeof(float);
                            footprints[1].data_access = MIR_DATA_ACCESS_READ;
                            footprints[1].part_of = BENCH[kk*NB+jj];

                            footprints[2].base = BENCH[ii*NB+jj];
                            footprints[2].start = 0;
                            footprints[2].end = BS*BS-1;
                            footprints[2].row_sz = 1;
                            footprints[2].type = sizeof(float);
                            footprints[2].data_access = MIR_DATA_ACCESS_READ;
                            footprints[2].part_of = BENCH[ii*NB+jj];

                            footprints[3].base = BENCH[ii*NB+jj];
                            footprints[3].start = 0;
                            footprints[3].end = BS*BS-1;
                            footprints[3].row_sz = 1;
                            footprints[3].type = sizeof(float);
                            footprints[3].data_access = MIR_DATA_ACCESS_WRITE;
                            footprints[3].part_of = BENCH[ii*NB+jj];
#endif

                            struct mir_task_t* task = mir_task_create((mir_tfunc_t) smp_ol_sparselu_par_call_2, (void*) &imm_args, sizeof(struct nanos_args_2_t), twc, num_footprints, footprints, NULL);
                        }
                    }

//#pragma omp taskwait
        mir_twc_wait(twc);
    }
    PMSG(" completed!\n");
}/*}}}*/

void sparselu_seq_call(float **BENCH)
{/*{{{*/
    int ii, jj, kk;

    for (kk=0; kk<NB; kk++)
    {
        lu0(BENCH[kk*NB+kk]);
        for (jj=kk+1; jj<NB; jj++)
            if (BENCH[kk*NB+jj] != NULL)
            {
                fwd(BENCH[kk*NB+kk], BENCH[kk*NB+jj]);
            }
        for (ii=kk+1; ii<NB; ii++)
            if (BENCH[ii*NB+kk] != NULL)
            {
                bdiv (BENCH[kk*NB+kk], BENCH[ii*NB+kk]);
            }
        for (ii=kk+1; ii<NB; ii++)
            if (BENCH[ii*NB+kk] != NULL)
                for (jj=kk+1; jj<NB; jj++)
                    if (BENCH[kk*NB+jj] != NULL)
                    {
                        if (BENCH[ii*NB+jj]==NULL) BENCH[ii*NB+jj] = allocate_clean_block();
                        bmod(BENCH[ii*NB+kk], BENCH[kk*NB+jj], BENCH[ii*NB+jj]);
                    }

    }
}/*}}}*/

void sparselu_fini (float **BENCH, char *pass)
{/*{{{*/
    print_structure(pass, BENCH);
    for(int i=0;i<NB*NB; i++)
        if(BENCH[i])
            mir_mem_pol_release(BENCH[i], sizeof(float) * BS * BS);
    free(BENCH);
}/*}}}*/

int sparselu_check(float **SEQ, float **BENCH)
{/*{{{*/
    int ii,jj,ok=1;

    for (ii=0; ((ii<NB) && ok); ii++)
    {
        for (jj=0; ((jj<NB) && ok); jj++)
        {
            if ((SEQ[ii*NB+jj] == NULL) && (BENCH[ii*NB+jj] != NULL)) ok = FALSE;
            if ((SEQ[ii*NB+jj] != NULL) && (BENCH[ii*NB+jj] == NULL)) ok = FALSE;
            if ((SEQ[ii*NB+jj] != NULL) && (BENCH[ii*NB+jj] != NULL))
                ok = checkmat(SEQ[ii*NB+jj], BENCH[ii*NB+jj]);
        }
    }
    if (ok) return TEST_SUCCESSFUL;
    else return TEST_UNSUCCESSFUL;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 3)
        PABRT("Usage: %s NB BS\n", argv[0]);

    // Init the runtime
    mir_create();

    if(argc == 2)
        NB = atoi(argv[1]);

    if(argc == 3)
    {
        NB = atoi(argv[1]);
        BS = atoi(argv[2]);
    }

    sparselu_init(&BENCH, "benchmark");
    long par_time_start = get_usecs();
    sparselu_par_call(BENCH);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    PDBG("Checking ... \n");
    sparselu_init(&SEQ, "serial");
    long seq_time_start = get_usecs();
    sparselu_seq_call(SEQ);
    long seq_time_end = get_usecs();
    double seq_time = (double)( seq_time_end - seq_time_start) / 1000000;
    check = sparselu_check(SEQ, BENCH);
    sparselu_fini(SEQ, "serial");
    PMSG("Seq. time=%f secs\n", seq_time);
#endif

    sparselu_fini(BENCH, "benchmark");

    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], NB, BS, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/

