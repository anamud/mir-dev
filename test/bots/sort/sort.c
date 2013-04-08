#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mir_public_int.h"
#include "helper.h"

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

static unsigned long arg_size;
static int cutoff_value = 262144; 
static int cutoff_value_1 = 524288; 
static int cutoff_value_2 = (20);
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

static  ELM med3(ELM a, ELM b, ELM c)
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

static  ELM choose_pivot(ELM *low, ELM *high)
{/*{{{*/
    return med3(*low, *high, low[(high - low) / 2]);
}/*}}}*/

static ELM *seqpart(ELM *low, ELM *high)
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

static void insertion_sort(ELM *low, ELM *high)
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

typedef struct _nx_data_env_0_t_tag
{/*{{{*/
        long int *low1_0;
        long int *low2_0;
        long int *lowdest_0;
        long int *split1_0;
        long int *split2_0;
} _nx_data_env_0_t;/*}}}*/

void cilkmerge_par(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);

static void _smp__ol_cilkmerge_par_0(void* arg)
{/*{{{*/
    _nx_data_env_0_t* _args = (_nx_data_env_0_t* ) arg; 
    cilkmerge_par((_args->low1_0), (_args->split1_0) - 1, (_args->low2_0), (_args->split2_0), (_args->lowdest_0));
}/*}}}*/

typedef struct _nx_data_env_1_t_tag
{/*{{{*/
        long int *high1_0;
        long int *high2_0;
        long int *lowdest_0;
        long int *split1_0;
        long int *split2_0;
        long int lowsize_0;
} _nx_data_env_1_t;/*}}}*/

static void _smp__ol_cilkmerge_par_1(void* arg)
{/*{{{*/
    _nx_data_env_1_t* _args = (_nx_data_env_1_t* ) arg; 
    cilkmerge_par((_args->split1_0) + 1, (_args->high1_0), (_args->split2_0) + 1, (_args->high2_0), (_args->lowdest_0) + (_args->lowsize_0) + 2);
}/*}}}*/

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

    // Create task
    _nx_data_env_0_t imm_args_0;
    imm_args_0.low1_0 = low1;
    imm_args_0.low2_0 = low2;
    imm_args_0.lowdest_0 = lowdest;
    imm_args_0.split1_0 = split1;
    imm_args_0.split2_0 = split2;

    struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) _smp__ol_cilkmerge_par_0, (void*) &imm_args_0, sizeof(_nx_data_env_0_t), NULL, 0, NULL, NULL);

    // Create task
    _nx_data_env_1_t imm_args_1;
    imm_args_1.high1_0 = high1;
    imm_args_1.high2_0 = high2;
    imm_args_1.lowdest_0 = lowdest;
    imm_args_1.split1_0 = split1;
    imm_args_1.split2_0 = split2;
    imm_args_1.lowsize_0 = lowsize;

    struct mir_task_t* task_1 = mir_task_create((mir_tfunc_t) _smp__ol_cilkmerge_par_1, (void*) &imm_args_1, sizeof(_nx_data_env_1_t), NULL, 0, NULL, NULL);
    
    // Wait for two tasks
    mir_task_wait(task_0);
    mir_task_wait(task_1);

    return;
}/*}}}*/

typedef struct _nx_data_env_2_t_tag
{/*{{{*/
        long int quarter_0;
        long int *A_0;
        long int *tmpA_0;
} _nx_data_env_2_t;/*}}}*/

void cilksort_par(ELM *low, ELM *tmp, long size);

static void _smp__ol_cilksort_par_2(void* arg)
{/*{{{*/
    _nx_data_env_2_t* _args = (_nx_data_env_2_t* ) arg; 
    cilksort_par((_args->A_0), (_args->tmpA_0), (_args->quarter_0));
}/*}}}*/

typedef struct _nx_data_env_3_t_tag
{/*{{{*/
        long int quarter_0;
        long int *B_0;
        long int *tmpB_0;
} _nx_data_env_3_t;/*}}}*/

static void _smp__ol_cilksort_par_3(void* arg)
{/*{{{*/
    _nx_data_env_3_t* _args = (_nx_data_env_3_t* ) arg; 
    cilksort_par((_args->B_0), (_args->tmpB_0), (_args->quarter_0));
}/*}}}*/

typedef struct _nx_data_env_4_t_tag
{/*{{{*/
        long int quarter_0;
        long int *C_0;
        long int *tmpC_0;
} _nx_data_env_4_t;/*}}}*/

static void _smp__ol_cilksort_par_4(void* arg)
{/*{{{*/
    _nx_data_env_4_t* _args = (_nx_data_env_4_t* ) arg; 
    cilksort_par((_args->C_0), (_args->tmpC_0), (_args->quarter_0));
}/*}}}*/

typedef struct _nx_data_env_5_t_tag
{/*{{{*/
        long int size_0;
        long int quarter_0;
        long int *D_0;
        long int *tmpD_0;
} _nx_data_env_5_t;/*}}}*/

static void _smp__ol_cilksort_par_5(void* arg)
{/*{{{*/
    _nx_data_env_5_t* _args = (_nx_data_env_5_t* ) arg; 
    cilksort_par((_args->D_0), (_args->tmpD_0), (_args->size_0) - 3 * (_args->quarter_0));
}/*}}}*/

typedef struct _nx_data_env_6_t_tag
{/*{{{*/
        long int quarter_0;
        long int *A_0;
        long int *B_0;
        long int *tmpA_0;
} _nx_data_env_6_t;/*}}}*/

static void _smp__ol_cilksort_par_6(void* arg)
{/*{{{*/
    _nx_data_env_6_t* _args = (_nx_data_env_6_t* ) arg; 
    cilkmerge_par((_args->A_0), (_args->A_0) + (_args->quarter_0) - 1, (_args->B_0), (_args->B_0) + (_args->quarter_0) - 1, (_args->tmpA_0));
}/*}}}*/

typedef struct _nx_data_env_7_t_tag
{/*{{{*/
        long int *low_0;
        long int size_0;
        long int quarter_0;
        long int *C_0;
        long int *D_0;
        long int *tmpC_0;
} _nx_data_env_7_t;/*}}}*/

static void _smp__ol_cilksort_par_7(void* arg)
{/*{{{*/
    _nx_data_env_7_t* _args = (_nx_data_env_7_t* ) arg; 
    cilkmerge_par((_args->C_0), (_args->C_0) + (_args->quarter_0) - 1, (_args->D_0), (_args->low_0) + (_args->size_0) - 1, (_args->tmpC_0));
}/*}}}*/

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

    // Task wait counter 
    struct mir_twc_t* twc = mir_twc_create();

    // Create task
    _nx_data_env_2_t imm_args_2;
    imm_args_2.quarter_0 = quarter;
    imm_args_2.A_0 = A;
    imm_args_2.tmpA_0 = tmpA;

    struct mir_task_t* task_2 = mir_task_create((mir_tfunc_t) _smp__ol_cilksort_par_2, (void*) &imm_args_2, sizeof(_nx_data_env_2_t), twc, 0, NULL, NULL);

    // Create task
    _nx_data_env_3_t imm_args_3;
    imm_args_3.quarter_0 = quarter;
    imm_args_3.B_0 = B;
    imm_args_3.tmpB_0 = tmpB;

    struct mir_task_t* task_3 = mir_task_create((mir_tfunc_t) _smp__ol_cilksort_par_3, (void*) &imm_args_3, sizeof(_nx_data_env_3_t), twc, 0, NULL, NULL);

    // Create task
    _nx_data_env_4_t imm_args_4;
    imm_args_4.quarter_0 = quarter;
    imm_args_4.C_0 = C;
    imm_args_4.tmpC_0 = tmpC;
    
    struct mir_task_t* task_4 = mir_task_create((mir_tfunc_t) _smp__ol_cilksort_par_4, (void*) &imm_args_4, sizeof(_nx_data_env_4_t), twc, 0, NULL, NULL);

    // Create task
    _nx_data_env_5_t imm_args_5;
    imm_args_5.size_0 = size;
    imm_args_5.quarter_0 = quarter;
    imm_args_5.D_0 = D;
    imm_args_5.tmpD_0 = tmpD;

    struct mir_task_t* task_5 = mir_task_create((mir_tfunc_t) _smp__ol_cilksort_par_5, (void*) &imm_args_5, sizeof(_nx_data_env_5_t), twc, 0, NULL, NULL);

    // Task wait 
    mir_twc_wait(twc);

    // Create task
    _nx_data_env_6_t imm_args_6;
    imm_args_6.quarter_0 = quarter;
    imm_args_6.A_0 = A;
    imm_args_6.B_0 = B;
    imm_args_6.tmpA_0 = tmpA;
    
    struct mir_task_t* task_6 = mir_task_create((mir_tfunc_t) _smp__ol_cilksort_par_6, (void*) &imm_args_6, sizeof(_nx_data_env_6_t), twc, 0, NULL, NULL);

    // Create task
    _nx_data_env_7_t imm_args_7;
    imm_args_7.low_0 = low;
    imm_args_7.size_0 = size;
    imm_args_7.quarter_0 = quarter;
    imm_args_7.C_0 = C;
    imm_args_7.D_0 = D;
    imm_args_7.tmpC_0 = tmpC;

    struct mir_task_t* task_7 = mir_task_create((mir_tfunc_t) _smp__ol_cilksort_par_7, (void*) &imm_args_7, sizeof(_nx_data_env_7_t), twc, 0, NULL, NULL);

    // Task wait 
    mir_twc_wait(twc);

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

typedef struct _nx_data_env_8_t_tag
{/*{{{*/
        unsigned long *arg_size_0;
        long int **array_0;
        long int **tmp_0;
} _nx_data_env_8_t;/*}}}*/

static void _smp__ol_sort_par_8(void* arg)
{/*{{{*/
    _nx_data_env_8_t* _args = (_nx_data_env_8_t* ) arg; 
    unsigned long *arg_size_0 = (unsigned long *) (_args->arg_size_0);
    long int **array_0 = (long int **) (_args->array_0);
    long int **tmp_0 = (long int **) (_args->tmp_0);
    cilksort_par((*array_0), (*tmp_0), (*arg_size_0));
}/*}}}*/

void sort_par(void)
{/*{{{*/
    // Create task
    _nx_data_env_8_t imm_args_8;
    imm_args_8.arg_size_0 = &(arg_size);
    imm_args_8.array_0 = &(array);
    imm_args_8.tmp_0 = &(tmp);

    struct mir_task_t* task_8 = mir_task_create((mir_tfunc_t) _smp__ol_sort_par_8, (void*) &imm_args_8, sizeof(_nx_data_env_8_t), NULL, 0, NULL, NULL);

    // Wait for task
    mir_task_wait(task_8);
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
