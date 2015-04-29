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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <libgen.h>
#include "sparselu.h"
#include <sys/time.h>
#include <omp.h>

#ifdef USE_MIR
#include "mir_public_int.h"
#endif
#include "helper.h"

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

float **SEQ, **BENCH;
int bots_arg_size, bots_arg_size_1;

#ifdef ENABLE_SPECOMP12_MOD
/***********************************************************************
 * checkmat: 
 **********************************************************************/
int checkmat (float *M, float *N)
{
    int i, j;
    float r_err;

    for (i = 0; i < bots_arg_size_1; i++)
    {
        for (j = 0; j < bots_arg_size_1; j++)
        {
            r_err = M[i*bots_arg_size_1+j] - N[i*bots_arg_size_1+j];
            if (r_err < 0.0 ) r_err = -r_err;
            r_err = r_err / M[i*bots_arg_size_1+j];
            if(r_err > EPSILON)
            {
                bots_message("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; Relative Error=%f\n",
                        i,j, M[i*bots_arg_size_1+j], i,j, N[i*bots_arg_size_1+j], r_err);
                return FALSE;
            }
        }
    }
    return TRUE;
}
/***********************************************************************
 * genmat: 
 **********************************************************************/
void genmat (float *M[])
{
    int null_entry, init_val, i, j, ii, jj;
    float *p;
    float *prow;
    float rowsum;

    init_val = 1325;

    /* generating the structure */
    for (ii=0; ii < bots_arg_size; ii++)
    {
        for (jj=0; jj < bots_arg_size; jj++)
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
                M[ii*bots_arg_size+jj] = (float *) malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
                if ((M[ii*bots_arg_size+jj] == NULL))
                {
                    bots_message("Error: Out of memory\n");
                    exit(101);
                }
                /* initializing matrix */
                /* Modify diagonal element of each row in order */
                /* to ensure matrix is diagonally dominant and  */
                /* well conditioned. */
                prow = p = M[ii*bots_arg_size+jj];
                for (i = 0; i < bots_arg_size_1; i++) 
                {
                    rowsum = 0.0;
                    for (j = 0; j < bots_arg_size_1; j++)
                    {
                        init_val = (3125 * init_val) % 65536;
                        (*p) = (float)((init_val - 32768.0) / 16384.0);
                        rowsum += abs(*p);
                        p++;
                    }
                    if (ii == jj) 
                        *(prow+i) = rowsum * (float) bots_arg_size + abs(*(prow+i));
                    prow += bots_arg_size_1;
                }
            }
            else
            {
                M[ii*bots_arg_size+jj] = NULL;
            }
        }
    }
}
#else
/***********************************************************************
 * checkmat: 
 **********************************************************************/
int checkmat (float *M, float *N)
{
    int i, j;
    float r_err;

    for (i = 0; i < bots_arg_size_1; i++)
    {
        for (j = 0; j < bots_arg_size_1; j++)
        {
            r_err = M[i*bots_arg_size_1+j] - N[i*bots_arg_size_1+j];
            if ( r_err == 0.0 ) continue;

            if (r_err < 0.0 ) r_err = -r_err;

            if ( M[i*bots_arg_size_1+j] == 0 ) 
            {
                bots_message("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; \n",
                        i,j, M[i*bots_arg_size_1+j], i,j, N[i*bots_arg_size_1+j]);
                return FALSE;
            }  
            r_err = r_err / M[i*bots_arg_size_1+j];
            if(r_err > EPSILON)
            {
                bots_message("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; Relative Error=%f\n",
                        i,j, M[i*bots_arg_size_1+j], i,j, N[i*bots_arg_size_1+j], r_err);
                return FALSE;
            }
        }
    }
    return TRUE;
}
/***********************************************************************
 * genmat: 
 **********************************************************************/
void genmat (float *M[])
{
    int null_entry, init_val, i, j, ii, jj;
    float *p;

    init_val = 1325;

    /* generating the structure */
    for (ii=0; ii < bots_arg_size; ii++)
    {
        for (jj=0; jj < bots_arg_size; jj++)
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
                M[ii*bots_arg_size+jj] = (float *) malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
                if ((M[ii*bots_arg_size+jj] == NULL))
                {
                    bots_message("Error: Out of memory\n");
                    exit(101);
                }
                /* initializing matrix */
                p = M[ii*bots_arg_size+jj];
                for (i = 0; i < bots_arg_size_1; i++) 
                {
                    for (j = 0; j < bots_arg_size_1; j++)
                    {
                        init_val = (3125 * init_val) % 65536;
                        (*p) = (float)((init_val - 32768.0) / 16384.0);
                        p++;
                    }
                }
            }
            else
            {
                M[ii*bots_arg_size+jj] = NULL;
            }
        }
    }
}
#endif
/***********************************************************************
 * print_structure: 
 **********************************************************************/
void print_structure(char *name, float *M[])
{
    return;
    int ii, jj;
    bots_message("Structure for matrix %s @ 0x%p\n",name, M);
    for (ii = 0; ii < bots_arg_size; ii++) {
        for (jj = 0; jj < bots_arg_size; jj++) {
            if (M[ii*bots_arg_size+jj]!=NULL) {bots_message("x");}
            else bots_message(" ");
        }
        bots_message("\n");
    }
    bots_message("\n");
}
/***********************************************************************
 * allocate_clean_block: 
 **********************************************************************/
float * allocate_clean_block()
{
    int i,j;
    float *p, *q;

    p = (float *) malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
    q=p;
    if (p!=NULL){
        for (i = 0; i < bots_arg_size_1; i++) 
            for (j = 0; j < bots_arg_size_1; j++){(*p)=0.0; p++;}

    }
    else
    {
        bots_message("Error: Out of memory\n");
        exit (101);
    }
    return (q);
}

/***********************************************************************
 * lu0: 
 **********************************************************************/
void lu0(float *diag)
{
    int i, j, k;

    for (k=0; k<bots_arg_size_1; k++)
        for (i=k+1; i<bots_arg_size_1; i++)
        {
            diag[i*bots_arg_size_1+k] = diag[i*bots_arg_size_1+k] / diag[k*bots_arg_size_1+k];
            for (j=k+1; j<bots_arg_size_1; j++)
                diag[i*bots_arg_size_1+j] = diag[i*bots_arg_size_1+j] - diag[i*bots_arg_size_1+k] * diag[k*bots_arg_size_1+j];
        }
}

/***********************************************************************
 * bdiv: 
 **********************************************************************/
void bdiv(float *diag, float *row)
{
    int i, j, k;
    for (i=0; i<bots_arg_size_1; i++)
        for (k=0; k<bots_arg_size_1; k++)
        {
            row[i*bots_arg_size_1+k] = row[i*bots_arg_size_1+k] / diag[k*bots_arg_size_1+k];
            for (j=k+1; j<bots_arg_size_1; j++)
                row[i*bots_arg_size_1+j] = row[i*bots_arg_size_1+j] - row[i*bots_arg_size_1+k]*diag[k*bots_arg_size_1+j];
        }
}
/***********************************************************************
 * bmod: 
 **********************************************************************/
void bmod(float *row, float *col, float *inner)
{
    int i, j, k;
    for (i=0; i<bots_arg_size_1; i++)
        for (j=0; j<bots_arg_size_1; j++)
            for (k=0; k<bots_arg_size_1; k++)
                inner[i*bots_arg_size_1+j] = inner[i*bots_arg_size_1+j] - row[i*bots_arg_size_1+k]*col[k*bots_arg_size_1+j];
}
/***********************************************************************
 * fwd: 
 **********************************************************************/
void fwd(float *diag, float *col)
{
    int i, j, k;
    for (j=0; j<bots_arg_size_1; j++)
        for (k=0; k<bots_arg_size_1; k++) 
            for (i=k+1; i<bots_arg_size_1; i++)
                col[i*bots_arg_size_1+j] = col[i*bots_arg_size_1+j] - diag[i*bots_arg_size_1+k]*col[k*bots_arg_size_1+j];
}


void sparselu_init (float ***pBENCH, char *pass)
{
    *pBENCH = (float **) malloc(bots_arg_size*bots_arg_size*sizeof(float *));
    genmat(*pBENCH);
    print_structure(pass, *pBENCH);
}

void sparselu_par_call(float **BENCH)
{
    int ii, jj, kk;

    bots_message("Computing SparseLU Factorization (%dx%d matrix with %dx%d blocks) ",
            bots_arg_size,bots_arg_size,bots_arg_size_1,bots_arg_size_1);
#pragma omp parallel
{
#pragma omp single
{
#pragma omp task
    for (kk=0; kk<bots_arg_size; kk++) 
    {
        lu0(BENCH[kk*bots_arg_size+kk]);
        for (jj=kk+1; jj<bots_arg_size; jj++)
            if (BENCH[kk*bots_arg_size+jj] != NULL)
#pragma omp task firstprivate(kk, jj) shared(BENCH)
            {
                fwd(BENCH[kk*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj]);
            }
        for (ii=kk+1; ii<bots_arg_size; ii++) 
            if (BENCH[ii*bots_arg_size+kk] != NULL)
#pragma omp task firstprivate(kk, ii) shared(BENCH)
            {
                bdiv (BENCH[kk*bots_arg_size+kk], BENCH[ii*bots_arg_size+kk]);
            }

#pragma omp taskwait

        for (ii=kk+1; ii<bots_arg_size; ii++)
            if (BENCH[ii*bots_arg_size+kk] != NULL)
                for (jj=kk+1; jj<bots_arg_size; jj++)
                    if (BENCH[kk*bots_arg_size+jj] != NULL)
#pragma omp task firstprivate(kk, jj, ii) shared(BENCH)
                    {
                        if (BENCH[ii*bots_arg_size+jj]==NULL) BENCH[ii*bots_arg_size+jj] = allocate_clean_block();
                        bmod(BENCH[ii*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj], BENCH[ii*bots_arg_size+jj]);
                    }

#pragma omp taskwait
    }
#pragma omp taskwait
}
}
    bots_message(" completed!\n");
}


void sparselu_seq_call(float **BENCH)
{
    int ii, jj, kk;

    for (kk=0; kk<bots_arg_size; kk++)
    {
        lu0(BENCH[kk*bots_arg_size+kk]);
        for (jj=kk+1; jj<bots_arg_size; jj++)
            if (BENCH[kk*bots_arg_size+jj] != NULL)
            {
                fwd(BENCH[kk*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj]);
            }
        for (ii=kk+1; ii<bots_arg_size; ii++)
            if (BENCH[ii*bots_arg_size+kk] != NULL)
            {
                bdiv (BENCH[kk*bots_arg_size+kk], BENCH[ii*bots_arg_size+kk]);
            }
        for (ii=kk+1; ii<bots_arg_size; ii++)
            if (BENCH[ii*bots_arg_size+kk] != NULL)
                for (jj=kk+1; jj<bots_arg_size; jj++)
                    if (BENCH[kk*bots_arg_size+jj] != NULL)
                    {
                        if (BENCH[ii*bots_arg_size+jj]==NULL) BENCH[ii*bots_arg_size+jj] = allocate_clean_block();
                        bmod(BENCH[ii*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj], BENCH[ii*bots_arg_size+jj]);
                    }

    }
}

void sparselu_fini (float **BENCH, char *pass)
{
    print_structure(pass, BENCH);
}

int sparselu_check(float **SEQ, float **BENCH)
{
    int ii,jj,ok=1;

    for (ii=0; ((ii<bots_arg_size) && ok); ii++)
    {
        for (jj=0; ((jj<bots_arg_size) && ok); jj++)
        {
            if ((SEQ[ii*bots_arg_size+jj] == NULL) && (BENCH[ii*bots_arg_size+jj] != NULL)) ok = FALSE;
            if ((SEQ[ii*bots_arg_size+jj] != NULL) && (BENCH[ii*bots_arg_size+jj] == NULL)) ok = FALSE;
            if ((SEQ[ii*bots_arg_size+jj] != NULL) && (BENCH[ii*bots_arg_size+jj] != NULL))
                ok = checkmat(SEQ[ii*bots_arg_size+jj], BENCH[ii*bots_arg_size+jj]);
        }
    }
    if (ok) return BOTS_RESULT_SUCCESSFUL;
    else return BOTS_RESULT_UNSUCCESSFUL;
}

int main(int argc, char *argv[])
{/*{{{*/
    if (argc != 3)
        PABRT("Usage: %s bots_arg_size bots_arg_size_1 \n", argv[0]);

#ifdef USE_MIR
    // Init the runtime
    mir_create();
#endif

    // Locals
    bots_arg_size = atoi(argv[1]);
    bots_arg_size_1 = atoi(argv[2]);

    sparselu_init(&BENCH, "benchmark");
    long par_time_start = get_usecs();
    sparselu_par_call(BENCH);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;
    sparselu_fini(BENCH, "benchmark");

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT
    PDBG("Checking ... \n");
    sparselu_init(&SEQ, "serial");
    long seq_time_start = get_usecs();
    sparselu_seq_call(SEQ);
    long seq_time_end = get_usecs();
    double seq_time = (double)( seq_time_end - seq_time_start) / 1000000;
    sparselu_fini(SEQ, "serial");
    check = sparselu_check(SEQ, BENCH);
    PMSG("Seq. time=%f secs\n", seq_time);
#endif

    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], bots_arg_size, bots_arg_size_1, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

#ifdef USE_MIR
    // Pull down the runtime
    mir_destroy();
#endif

    return 0;
}/*}}}*/
