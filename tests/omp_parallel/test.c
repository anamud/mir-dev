#include <stdlib.h>
#include <check.h>
#include <omp.h>

START_TEST(omp_parallel_plain)
{/*{{{*/
    int a = 0;
    int num_threads;

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
        num_threads = omp_get_num_threads();
    }

    ck_assert_int_eq(a, num_threads);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_num_threads_shell)
{/*{{{*/
    int a = 0;
    int num_threads_reqd = 1;
    setenv("OMP_NUM_THREADS", "1", 1);

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
    }

    unsetenv("OMP_NUM_THREADS");

    ck_assert_int_eq(a, num_threads_reqd);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_num_threads_small)
{/*{{{*/
    int a = 0;
    int num_threads_reqd = 2;

#pragma omp parallel shared(a) num_threads(num_threads_reqd)
    {
        __sync_fetch_and_add(&a, 1);
    }

    ck_assert_int_eq(a, num_threads_reqd);
}/*}}}*/
END_TEST

// TODO: Add test for teams larger than number of MIR workers.
// Large teams are unsupported in MIR.
// The test should therefore pass if MIR throws an assertion.

START_TEST(omp_sequential_parallel)
{/*{{{*/
    int a = 0;
    int num_threads;

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
        num_threads = omp_get_num_threads();
    }
#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
    }

    ck_assert_int_eq(a, 2*num_threads);
}/*}}}*/
END_TEST

START_TEST(omp_nested_parallel)
{/*{{{*/
    int a = 0;

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
        /* Nested parallel block. */
#pragma omp parallel shared(a)
        {
            __sync_fetch_and_add(&a, 1);
        }
    }

    ck_assert_int_eq(a, 2);
}/*}}}*/
END_TEST

START_TEST(omp_nested_sequential_parallel)
{/*{{{*/
    int a = 0;

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
        /* Nested parallel block. */
#pragma omp parallel shared(a)
        {
            __sync_fetch_and_add(&a, 1);
        }
    }

    ck_assert_int_eq(a, 2);

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
    }

    ck_assert_int_eq(a, 3);
}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s = suite_create("Test");

    TCase* tc = tcase_create("omp_parallel");
    tcase_add_test(tc, omp_parallel_plain);
    tcase_add_test(tc, omp_parallel_num_threads_shell);
    tcase_add_test(tc, omp_parallel_num_threads_small);
    tcase_add_test(tc, omp_sequential_parallel);
    /* tcase_add_test(tc, omp_nested_parallel); */
    /* tcase_add_test(tc, omp_nested_sequential_parallel); */

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
