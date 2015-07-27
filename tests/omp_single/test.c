#include <stdlib.h>
#include <check.h>
#include <omp.h>

START_TEST(omp_parallel_single)
{/*{{{*/
    int a = 0;

#pragma omp parallel shared(a) num_threads(2)
    {
#pragma omp single
        {
            __sync_fetch_and_add(&a, 1);
        }
    }

    ck_assert_int_eq(a, 1);
}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s = suite_create("Test");

    TCase* tc = tcase_create("omp_single");
    tcase_add_test(tc, omp_parallel_single);

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
