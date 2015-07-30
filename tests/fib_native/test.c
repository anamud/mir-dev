#include <stdlib.h>
#include <check.h>
#include <stdint.h>
#include "mir_public_int.h"

static uint64_t fib(int n, int d);
const int cutoff_value = 12;

static uint64_t fib_seq(int n)
{ /*{{{*/
    if (n < 2)
        return n;
    return fib_seq(n - 1) + fib_seq(n - 2);
} /*}}}*/

typedef struct data_env_0_t_tag { /*{{{*/
    uint64_t* x_0;
    int n_0;
    int d_0;
} data_env_0_t; /*}}}*/

void ol_fib_0(void* arg)
{ /*{{{*/
    data_env_0_t* _args = (data_env_0_t*)arg;
    uint64_t* x_0 = (uint64_t*)(_args->x_0);
    (*x_0) = fib((_args->n_0) - 1, (_args->d_0) + 1);
} /*}}}*/

typedef struct data_env_1_t_tag { /*{{{*/
    uint64_t* y_0;
    int n_0;
    int d_0;
} data_env_1_t; /*}}}*/

void ol_fib_1(data_env_1_t* arg)
{ /*{{{*/
    data_env_1_t* _args = (data_env_1_t*)arg;
    uint64_t* y_0 = (uint64_t*)(_args->y_0);
    (*y_0) = fib((_args->n_0) - 2, (_args->d_0) + 1);
} /*}}}*/

uint64_t fib(int n, int d)
{ /*{{{*/
    uint64_t x, y;
    if (n < 2)
        return n;
    if (d < cutoff_value) {
        // Create task1
        data_env_0_t imm_args_0;
        imm_args_0.x_0 = &(x);
        imm_args_0.n_0 = n;
        imm_args_0.d_0 = d;

        mir_task_create((mir_tfunc_t)ol_fib_0, (void*)&imm_args_0, sizeof(data_env_0_t), 0, NULL, "ol_fib_0");

        // Create task2
        data_env_1_t imm_args_1;
        imm_args_1.y_0 = &(y);
        imm_args_1.n_0 = n;
        imm_args_1.d_0 = d;

        mir_task_create((mir_tfunc_t)ol_fib_1, (void*)&imm_args_1, sizeof(data_env_1_t), 0, NULL, "ol_fib_1");

        // Task wait
        mir_task_wait();
    }
    else {
        x = fib_seq(n - 1);
        y = fib_seq(n - 2);
    }

    return x + y;
} /*}}}*/

typedef struct data_env_2_t_tag { /*{{{*/
    uint64_t* par_res_0;
    int n_0;
    int d_0;
} data_env_2_t; /*}}}*/

void ol_fib_2(data_env_2_t* arg)
{ /*{{{*/
    data_env_2_t* _args = (data_env_2_t*)arg;
    uint64_t* par_res_0 = (uint64_t*)(_args->par_res_0);
    (*par_res_0) = fib((_args->n_0), (_args->d_0));
} /*}}}*/

START_TEST(fib_native)
{/*{{{*/
    int num = 42;
    uint64_t result;

    mir_create();

    data_env_2_t imm_args_2;
    imm_args_2.par_res_0 = &(result);
    imm_args_2.n_0 = num;
    imm_args_2.d_0 = 0;

    mir_task_create((mir_tfunc_t)ol_fib_2, (void*)&imm_args_2, sizeof(data_env_2_t), 0, NULL, "ol_fib_2");

    mir_task_wait();

    mir_destroy();

    ck_assert_int_eq(result, 267914296); // fib(42) = 267914296
}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s;
    s = suite_create("Test");

    TCase* tc = tcase_create("fib_native");
    tcase_add_test(tc, fib_native);
    tcase_set_timeout(tc, 10);
    suite_add_tcase(s, tc);

    return s;
}/*}}}*/

int main(void)
{/*{{{*/
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = test_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}/*}}}*/

