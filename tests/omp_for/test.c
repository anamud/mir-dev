#include <stdlib.h>
#include <check.h>
#include <omp.h>

START_TEST(omp_parallel_for_static)
{/*{{{*/
    int a[128] = {0};

#pragma omp parallel for shared(a) schedule(static)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_static_num_threads_small)
{/*{{{*/
    int a[128] = {0};
    int num_threads_reqd = 2;
    int num_threads;

#pragma omp parallel for shared(a) schedule(static) num_threads(num_threads_reqd)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
        num_threads = omp_get_num_threads();
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
    ck_assert_int_eq(num_threads, num_threads_reqd);
}/*}}}*/
END_TEST

// TODO: Add test for teams larger than number of MIR workers.
// Large teams are unsupported in MIR.
// The test should therefore pass if MIR throws an assertion.

START_TEST(omp_parallel_for_dynamic)
{/*{{{*/
    int a[128] = {0};

#pragma omp parallel for shared(a) schedule(dynamic)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_dynamic_num_threads_small)
{/*{{{*/
    int a[128] = {0};
    int num_threads_reqd = 2;
    int num_threads;

#pragma omp parallel for shared(a) schedule(dynamic) num_threads(num_threads_reqd)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
        num_threads = omp_get_num_threads();
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
    ck_assert_int_eq(num_threads, num_threads_reqd);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_runtime)
{/*{{{*/
    int a[128] = {0};

#pragma omp parallel for shared(a) schedule(runtime)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_runtime_num_threads_small)
{/*{{{*/
    int a[128] = {0};
    int num_threads_reqd = 2;
    int num_threads;

#pragma omp parallel for shared(a) schedule(runtime) num_threads(num_threads_reqd)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
        num_threads = omp_get_num_threads();
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
    ck_assert_int_eq(num_threads, num_threads_reqd);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_runtime_static)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "static", 1);

#pragma omp parallel for shared(a) schedule(runtime)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_runtime_static_chunk)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "static,10", 1);

#pragma omp parallel for shared(a) schedule(runtime)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_runtime_dynamic)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "dynamic", 1);

#pragma omp parallel for shared(a) schedule(runtime)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_parallel_for_runtime_dynamic_chunk)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "dynamic,10", 1);

#pragma omp parallel for shared(a) schedule(runtime)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_static)
{/*{{{*/
    int a[128] = {0};

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(static)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_static_num_threads_small)
{/*{{{*/
    int a[128] = {0};
    int num_threads_reqd = 2;
    int num_threads;

#pragma omp parallel shared(a) num_threads(num_threads_reqd)
    {
        num_threads = omp_get_num_threads();
#pragma omp for schedule(static)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
    ck_assert_int_eq(num_threads, num_threads_reqd);
}/*}}}*/
END_TEST

START_TEST(omp_for_dynamic)
{/*{{{*/
    int a[128] = {0};

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(dynamic)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_dynamic_num_threads_small)
{/*{{{*/
    int a[128] = {0};
    int num_threads_reqd = 2;
    int num_threads;

#pragma omp parallel shared(a) num_threads(num_threads_reqd)
    {
        num_threads = omp_get_num_threads();
#pragma omp for schedule(dynamic)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
    ck_assert_int_eq(num_threads, num_threads_reqd);
}/*}}}*/
END_TEST

START_TEST(omp_for_runtime)
{/*{{{*/
    int a[128] = {0};

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_runtime_num_threads_small)
{/*{{{*/
    int a[128] = {0};
    int num_threads_reqd = 2;
    int num_threads;

#pragma omp parallel shared(a) num_threads(num_threads_reqd)
    {
        num_threads = omp_get_num_threads();
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
    ck_assert_int_eq(num_threads, num_threads_reqd);
}/*}}}*/
END_TEST

START_TEST(omp_for_runtime_static)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "static", 1);

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_runtime_static_chunk)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "static,10", 1);

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_runtime_dynamic)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "dynamic", 1);

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

START_TEST(omp_for_runtime_dynamic_chunk)
{/*{{{*/
    int a[128] = {0};

    setenv("OMP_SCHEDULE", "dynamic,10", 1);

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    unsetenv("OMP_SCHEDULE");

    int sum = 0;
    int sum_gold = 0;
    for(int i=0; i<128; i++)
    {
        sum += a[i];
        sum_gold += i;
    }
    ck_assert_int_eq(sum, sum_gold);
}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s = suite_create("Test");

    TCase* tc = tcase_create("omp_for");
    tcase_add_test(tc, omp_parallel_for_static);
    tcase_add_test(tc, omp_parallel_for_static_num_threads_small);
    tcase_add_test(tc, omp_parallel_for_dynamic);
    tcase_add_test(tc, omp_parallel_for_dynamic_num_threads_small);
    tcase_add_test(tc, omp_parallel_for_runtime);
    tcase_add_test(tc, omp_parallel_for_runtime_num_threads_small);
    tcase_add_test(tc, omp_parallel_for_runtime_static);
    tcase_add_test(tc, omp_parallel_for_runtime_static_chunk);
    tcase_add_test(tc, omp_parallel_for_runtime_dynamic);
    tcase_add_test(tc, omp_parallel_for_runtime_dynamic_chunk);
    tcase_add_test(tc, omp_for_static);
    tcase_add_test(tc, omp_for_static_num_threads_small);
    tcase_add_test(tc, omp_for_dynamic);
    tcase_add_test(tc, omp_for_dynamic_num_threads_small);
    tcase_add_test(tc, omp_for_runtime);
    tcase_add_test(tc, omp_for_runtime_static);
    tcase_add_test(tc, omp_for_runtime_static_chunk);
    tcase_add_test(tc, omp_for_runtime_dynamic);
    tcase_add_test(tc, omp_for_runtime_dynamic_chunk);

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
