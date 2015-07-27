#include <stdlib.h>
#include <check.h>
#include <omp.h>

START_TEST(omp_critical)
{/*{{{*/
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
}/*}}}*/
END_TEST

START_TEST(omp_critical_named)
{/*{{{*/
    int a = 42;
    int a_copy = a;

#pragma omp parallel shared(a)
    {
#pragma omp single
        {
#pragma omp task shared(a)
            {
#pragma omp critical(a_crit_sec)
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
#pragma omp critical(a_crit_sec)
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
    ck_assert_int_eq(a, a_copy);
}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s = suite_create("Test");

    TCase* tc = tcase_create("omp_critical");
    tcase_add_test(tc, omp_critical);
    tcase_add_test(tc, omp_critical_named);

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
