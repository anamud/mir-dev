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

/*
 *  Original code from the Cilk project
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 */

/*
 * this program uses an algorithm that we call `cilksort'.
 * The algorithm is essentially mergesort:
 *
 *   cilksort(in[1..n]) =
 *       spawn cilksort(in[1..n/2], tmp[1..n/2])
 *       spawn cilksort(in[n/2..n], tmp[n/2..n])
 *       sync
 *       spawn cilkmerge(tmp[1..n/2], tmp[n/2..n], in[1..n])
 *
 *
 * The procedure cilkmerge does the following:
 *       
 *       cilkmerge(A[1..n], B[1..m], C[1..(n+m)]) =
 *          find the median of A \union B using binary
 *          search.  The binary search gives a pair
 *          (ma, mb) such that ma + mb = (n + m)/2
 *          and all elements in A[1..ma] are smaller than
 *          B[mb..m], and all the B[1..mb] are smaller
 *          than all elements in A[ma..n].
 *
 *          spawn cilkmerge(A[1..ma], B[1..mb], C[1..(n+m)/2])
 *          spawn cilkmerge(A[ma..m], B[mb..n], C[(n+m)/2 .. (n+m)])
 *          sync
 *
 * The algorithm appears for the first time (AFAIK) in S. G. Akl and
 * N. Santoro, "Optimal Parallel Merging and Sorting Without Memory
 * Conflicts", IEEE Trans. Comp., Vol. C-36 No. 11, Nov. 1987 .  The
 * paper does not express the algorithm using recursion, but the
 * idea of finding the median is there.
 *
 * For cilksort of n elements, T_1 = O(n log n) and
 * T_\infty = O(log^3 n).  There is a way to shave a
 * log factor in the critical path (left as homework).
 */

/* Ananya Muddukrishna (ananya@kth.se) ported to MIR */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>

#include "mir_public_int.h"
#include "helper.h"

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

static unsigned long arg_size;
/*static int cutoff_value = 262144; */
/*static int cutoff_value_1 = 524288; */
/*static int cutoff_value = 2048;*/
/*static int cutoff_value_1 = 2048;*/
/*static int cutoff_value_2 = 20;*/
static int cutoff_value = 4096;
static int cutoff_value_1 = 4096;
static int cutoff_value_2 = 128;
static unsigned long rand_nxt = 0;

typedef long ELM;
ELM *array, *tmp;

static unsigned long my_rand(void)
{/*{{{*/
    rand_nxt = rand_nxt * 1103515245 + 12345;
    return rand_nxt;
}/*}}}*/

static void my_srand(unsigned long seed)
{/*{{{*/
    rand_nxt = seed;
}/*}}}*/

/*static*/  ELM med3(ELM a, ELM b, ELM c)
{/*{{{*/
    if (a < b)
    {
        if (b < c)
        {
            return b;
        }
        else
        {
            if (a < c)
                return c;
            else
                return a;
        }
    }
    else
    {
        if (b > c)
        {
            return b;
        }
        else
        {
            if (a > c)
                return c;
            else
                return a;
        }
    }
}/*}}}*/

/*static*/  ELM choose_pivot(ELM *low, ELM *high)
{/*{{{*/
    return med3(*low, *high, low[(high - low) / 2]);
}/*}}}*/

/*static*/ ELM *seqpart(ELM *low, ELM *high)
{/*{{{*/
    ELM pivot;
    ELM h, l;
    ELM *curr_low = low;
    ELM *curr_high = high;
    pivot = choose_pivot(low, high);
    while (1)
    {
        while ((h = *curr_high) > pivot)
            curr_high--;
        while ((l = *curr_low) < pivot)
            curr_low++;
        if (curr_low >= curr_high)
            break;
        *curr_high-- = l;
        *curr_low++ = h;
    }
    if (curr_high < high)
        return curr_high;
    else
        return curr_high - 1;
}/*}}}*/

/*static*/ void insertion_sort(ELM *low, ELM *high)
{/*{{{*/
    ELM *p, *q;
    ELM a, b;
    for (q = low + 1;
        q <= high;
        ++q)
    {
        a = q[0];
        for (p = q - 1;
            p >= low && (b = p[0]) > a;
            p--)
            p[1] = b;
        p[1] = a;
    }
}/*}}}*/

void seqquick(ELM *low, ELM *high)
{/*{{{*/
    ELM *p;
    while (high - low >= cutoff_value_2)
    {
        p = seqpart(low, high);
        seqquick(low, p);
        low = p + 1;
    }
    insertion_sort(low, high);
}/*}}}*/

void seqmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest)
{/*{{{*/
    ELM a1, a2;
    if (low1 < high1 && low2 < high2)
    {
        a1 = *low1;
        a2 = *low2;
        for (;
            ;
            )
        {
            if (a1 < a2)
            {
                *lowdest++ = a1;
                a1 = *++low1;
                if (low1 >= high1)
                    break;
            }
            else
            {
                *lowdest++ = a2;
                a2 = *++low2;
                if (low2 >= high2)
                    break;
            }
        }
    }
    if (low1 <= high1 && low2 <= high2)
    {
        a1 = *low1;
        a2 = *low2;
        for (;
            ;
            )
        {
            if (a1 < a2)
            {
                *lowdest++ = a1;
                ++low1;
                if (low1 > high1)
                    break;
                a1 = *low1;
            }
            else
            {
                *lowdest++ = a2;
                ++low2;
                if (low2 > high2)
                    break;
                a2 = *low2;
            }
        }
    }
    if (low1 > high1)
    {
        memcpy(lowdest, low2, sizeof(ELM) * (high2 - low2 + 1));
    }
    else
    {
        memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1 + 1));
    }
}/*}}}*/

ELM *binsplit(ELM val, ELM *low, ELM *high)
{/*{{{*/
    ELM *mid;
    while (low != high)
    {
        mid = low + ((high - low + 1) >> 1);
        if (val <= *mid)
            high = mid - 1;
        else
            low = mid;
    }
    if (*low > val)
        return low - 1;
    else
        return low;
}/*}}}*/

void cilkmerge_par(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);

void cilkmerge_par(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest)
{/*{{{*/
    ELM *split1, *split2;

    long int lowsize;

    if (high2 - low2 > high1 - low1)
    {
            ELM *tmp;

            tmp = low1;
            low1 = low2;
            low2 = tmp;

            tmp = high1;
            high1 = high2;
            high2 = tmp;
    }

    if (high2 < low2)
    {
        memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1));
        return;
    }

    if (high2 - low2 < cutoff_value)
    {
        seqmerge(low1, high1, low2, high2, lowdest);
        return;
    }

    split1 = ((high1 - low1 + 1) / 2) + low1;
    split2 = binsplit(*split1, low2, high2);
    lowsize = split1 - low1 + split2 - low2;
    *(lowdest + lowsize + 1) = *split1;

#pragma omp task shared(low1, split1, low2, split2, lowdest) 
    cilkmerge_par(low1, split1 - 1, low2, split2, lowdest);
#pragma omp task shared(split1, high1, split2, high2, lowdest, lowsize)
    cilkmerge_par(split1 + 1, high1, split2 + 1, high2, lowdest + lowsize + 2);
#pragma omp taskwait

    return;
}/*}}}*/

void cilksort_par(ELM *low, ELM *tmp, long size);

void cilksort_par(ELM *low, ELM *tmp, long size)
{/*{{{*/
    if (size < cutoff_value_1)
    {
        seqquick(low, low + size - 1);
        return;
    }

    long quarter = size / 4;

    ELM *A, *B, *C, *D, *tmpA, *tmpB, *tmpC, *tmpD;
    A = low;
    tmpA = tmp;
    B = A + quarter;
    tmpB = tmpA + quarter;
    C = B + quarter;
    tmpC = tmpB + quarter;
    D = C + quarter;
    tmpD = tmpC + quarter;

#pragma omp task shared(A, tmpA, quarter)
     cilksort_par(A, tmpA, quarter);
#pragma omp task shared(B, tmpB, quarter)
     cilksort_par(B, tmpB, quarter);
#pragma omp task shared(C, tmpC, quarter)
     cilksort_par(C, tmpC, quarter);
#pragma omp task shared(D, tmpD, size, quarter)
     cilksort_par(D, tmpD, size - 3 * quarter);
#pragma omp taskwait

#pragma omp task shared(A, quarter, B, tmpA)
     cilkmerge_par(A, A + quarter - 1, B, B + quarter - 1, tmpA);
#pragma omp task shared(C, quarter, D, low, size, tmpC)
     cilkmerge_par(C, C + quarter - 1, D, low + size - 1, tmpC);
#pragma omp taskwait

    // Recurse
    cilkmerge_par(tmpA, tmpC - 1, tmpC, tmpA + size - 1, A);
}/*}}}*/

void scramble_array(ELM *array)
{/*{{{*/
    unsigned long i;
    unsigned long j;
    for (i = 0; i < arg_size; ++i)
    {
        j = my_rand();
        j = j % arg_size;
        {
            ELM tmp;
            tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
        }
        ;
    }
}/*}}}*/

void fill_array(ELM *array)
{/*{{{*/
    unsigned long i;
    my_srand(1);
    for (i = 0;
        i < arg_size;
        ++i)
    {
        array[i] = i;
    }
}/*}}}*/

void sort_init(void)
{/*{{{*/
    if (arg_size < 4)
    {
        PMSG("%lu can not be less than 4, using 4 as a parameter.\n", arg_size);
        arg_size = 4;
    }

    array = (ELM *) mir_mem_pol_allocate (arg_size * sizeof(ELM));
    tmp = (ELM *) mir_mem_pol_allocate (arg_size * sizeof(ELM));
    fill_array(array);
    scramble_array(array);
}/*}}}*/

void sort_deinit(void)
{/*{{{*/
    mir_mem_pol_release(array, arg_size * sizeof(ELM));
    mir_mem_pol_release(tmp, arg_size * sizeof(ELM));
}/*}}}*/

void sort_par(void)
{/*{{{*/
    #pragma omp task shared(array, tmp, arg_size)
         cilksort_par(array, tmp, arg_size);
    #pragma omp taskwait
}/*}}}*/

int sort_verify(void)
{/*{{{*/
    int i, success = 1;
    for (i = 0;
        i < arg_size;
        ++i)
        if (array[i] != i)
            success = 0;
    return success ? TEST_SUCCESSFUL : TEST_UNSUCCESSFUL;
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    if (argc > 2)
        PABRT("Usage: %s num_elements\n", argv[0]);

    // Init the runtime
    mir_create();

    arg_size = 33554432;
    if(argc == 2)
        arg_size = atoi(argv[1]);

    sort_init();
    long par_time_start = get_usecs();
    sort_par();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT 
    check = sort_verify();
#endif

    sort_deinit();

    PMSG("%s(%lu),check=%d in %s,time=%f secs\n", argv[0], arg_size, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/
