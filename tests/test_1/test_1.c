#include <stdlib.h>
#include <check.h>
#include <omp.h>
#include "mir_public_int.h"

/* OpenMP parallel and single test cases */
START_TEST(omp_parallel_plain)
{
    int a = 0;
    int num_threads;

#pragma omp parallel shared(a)
    {
        __sync_fetch_and_add(&a, 1);
        num_threads = omp_get_num_threads();
    }

    ck_assert_int_eq(a, num_threads);
}
END_TEST

START_TEST(omp_parallel_single)
{
    int a = 0;

#pragma omp parallel shared(a)
    {
        /* Single works as specified in the standard. */
#pragma omp single
        {
            __sync_fetch_and_add(&a, 1);
        }
    }

    ck_assert_int_eq(a, 1);
}
END_TEST

START_TEST(omp_nested_parallel)
{
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
}
END_TEST

START_TEST(omp_nested_parallel_single)
{
    int a = 0;

#pragma omp parallel shared(a)
    {
        /* Single works as specified in the standard. */
#pragma omp single
        {
        __sync_fetch_and_add(&a, 1);
            /* Nested parallel block. */
#pragma omp parallel shared(a)
            {
                __sync_fetch_and_add(&a, 1);
            }
        }
    }

    ck_assert_int_eq(a, 2);
}
END_TEST

START_TEST(omp_sequential_parallel)
{
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
}
END_TEST

START_TEST(omp_nested_sequential_parallel)
{
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
}
END_TEST

/* OpenMP for test cases */
START_TEST(omp_parallel_for_static)
{
    int a[128] = {0};

#pragma omp parallel for shared(a) schedule(static)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    /* Check if the sum of array elements is (128)*(128-1)/2 */
    int sum = 0;
    for(int i=0; i<128; i++)
        sum += a[i];
    ck_assert_int_eq(sum, (128)*(128-1)/2);
}
END_TEST

START_TEST(omp_parallel_for_dynamic)
{
    int a[128] = {0};

#pragma omp parallel for shared(a) schedule(dynamic)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    /* Check if the sum of array elements is (128)*(128-1)/2 */
    int sum = 0;
    for(int i=0; i<128; i++)
        sum += a[i];
    ck_assert_int_eq(sum, (128)*(128-1)/2);
}
END_TEST

/*START_TEST(omp_parallel_for_guided)*/
/*{*/
    /*int a[128] = {0};*/

/*#pragma omp parallel for shared(a) schedule(guided)*/
    /*for(int i=0; i<128; i++)*/
    /*{*/
        /*a[i] = i;*/
    /*}*/

    /*[> Check if the sum of array elements is (128)*(128-1)/2 <]*/
    /*int sum = 0;*/
    /*for(int i=0; i<128; i++)*/
        /*sum += a[i];*/
    /*ck_assert_int_eq(sum, (128)*(128-1)/2);*/
/*}*/
/*END_TEST*/

START_TEST(omp_parallel_for_runtime)
{
    int a[128] = {0};

#pragma omp parallel for shared(a) schedule(runtime)
    for(int i=0; i<128; i++)
    {
        a[i] = i;
    }

    /* Check if the sum of array elements is (128)*(128-1)/2 */
    int sum = 0;
    for(int i=0; i<128; i++)
        sum += a[i];
    ck_assert_int_eq(sum, (128)*(128-1)/2);
}
END_TEST

START_TEST(omp_for_static)
{
    int a[128] = {0};

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(static)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    /* Check if the sum of array elements is (128)*(128-1)/2 */
    int sum = 0;
    for(int i=0; i<128; i++)
        sum += a[i];
    ck_assert_int_eq(sum, (128)*(128-1)/2);
}
END_TEST

START_TEST(omp_for_dynamic)
{
    int a[128] = {0};

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(dynamic)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    /* Check if the sum of array elements is (128)*(128-1)/2 */
    int sum = 0;
    for(int i=0; i<128; i++)
        sum += a[i];
    ck_assert_int_eq(sum, (128)*(128-1)/2);
}
END_TEST

/*START_TEST(omp_for_guided)*/
/*{*/
    /*int a[128] = {0};*/

/*#pragma omp parallel shared(a)*/
    /*{*/
/*#pragma omp for schedule(guided)*/
        /*for(int i=0; i<128; i++)*/
        /*{*/
            /*a[i] = i;*/
        /*}*/
    /*}*/

    /*[> Check if the sum of array elements is (128)*(128-1)/2 <]*/
    /*int sum = 0;*/
    /*for(int i=0; i<128; i++)*/
        /*sum += a[i];*/
    /*ck_assert_int_eq(sum, (128)*(128-1)/2);*/
/*}*/
/*END_TEST*/

START_TEST(omp_for_runtime)
{
    int a[128] = {0};

#pragma omp parallel shared(a)
    {
#pragma omp for schedule(runtime)
        for(int i=0; i<128; i++)
        {
            a[i] = i;
        }
    }

    /* Check if the sum of array elements is (128)*(128-1)/2 */
    int sum = 0;
    for(int i=0; i<128; i++)
        sum += a[i];
    ck_assert_int_eq(sum, (128)*(128-1)/2);
}
END_TEST

START_TEST(omp_critical)
{
    int a = 0;

#pragma omp parallel shared(a)
    {
#pragma omp single
        {
#pragma omp task shared(a)
            {
#pragma omp critical
                {
                    a--;
                    a++;
                    a--;
                    a++;
                    a--;
                    a--;
                    a++;
                    a++;
                }
            }
#pragma omp task shared(a)
            {
#pragma omp critical
                {
                    a++;
                    a--;
                    a++;
                    a--;
                    a++;
                    a++;
                    a--;
                    a--;
                }
            }
        }
    }

    /* Since the operations are symmetric and mirrored, the variable should have its original value*/
    ck_assert_int_eq(a, 0);
}
END_TEST

Suite* test_suite(void)
{
    Suite* s;
    s = suite_create("Test_1");

    /* Omp parallel test case */
    TCase* tc_omp_parallel;
    tc_omp_parallel = tcase_create("Omp_parallel");
    tcase_add_test(tc_omp_parallel, omp_parallel_plain);
    tcase_add_test(tc_omp_parallel, omp_parallel_single);
    /* tcase_add_test(tc_omp_parallel, omp_nested_parallel); */
    /* tcase_add_test(tc_omp_parallel, omp_nested_parallel_single); */
    tcase_add_test(tc_omp_parallel, omp_sequential_parallel);
    /* tcase_add_test(tc_omp_parallel, omp_nested_sequential_parallel); */
    suite_add_tcase(s, tc_omp_parallel);

    /* Omp parallel for loop test case */
    TCase* tc_omp_for;
    tc_omp_for = tcase_create("Omp_for");
    tcase_add_test(tc_omp_for, omp_parallel_for_static);
    /* tcase_add_test(tc_omp_for, omp_parallel_for_dynamic); */
    /*tcase_add_test(tc_omp_for, omp_parallel_for_guided);*/
    tcase_add_test(tc_omp_for, omp_parallel_for_runtime);
    tcase_add_test(tc_omp_for, omp_for_static);
    /* tcase_add_test(tc_omp_for, omp_for_dynamic); */
    /*tcase_add_test(tc_omp_for, omp_for_guided);*/
    /* tcase_add_test(tc_omp_for, omp_for_runtime); */
    suite_add_tcase(s, tc_omp_for);

    /* Omp critical section test case */
    TCase* tc_omp_critical;
    tc_omp_critical = tcase_create("Omp_critical");
    tcase_add_test(tc_omp_critical, omp_critical);
    suite_add_tcase(s, tc_omp_critical);

    return s;
}

int main(void)
{
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = test_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

