#include <stdlib.h>
#include <check.h>
#include <omp.h>
#include <stdint.h>

static uint64_t fib_seq(int n)
{ /*{{{*/
    if (n < 2)
        return n;
    return fib_seq(n - 1) + fib_seq(n - 2);
} /*}}}*/

START_TEST(omp_barrier_simple)
{/*{{{*/
    int num_threads_reqd = 2;
    int a[2];
    a[0] = 42;
    a[1] = -42;

#pragma omp parallel shared(a) num_threads(num_threads_reqd)
    {
        // Do large work with one thread.
        if(omp_get_thread_num() == 0)
            fib_seq(42);
        // Both threads update their private variables.
        a[omp_get_thread_num()] += ((omp_get_thread_num() == 0 ? -1 : 1) * 42);
#pragma omp barrier
        // Check if the other thread has update its private variable.
        ck_assert_int_eq(a[omp_get_thread_num() == 0 ? 1 : 0], 0);
        // Do large work with one thread.
        if(omp_get_thread_num() == 1)
            fib_seq(42);
        // Both threads update their private variables.
        a[omp_get_thread_num()] += ((omp_get_thread_num() == 0 ? 1 : -1) * 42);
#pragma omp barrier
        // Check if the other thread has update its private variable.
        ck_assert_int_eq(a[omp_get_thread_num() == 0 ? 1 : 0], -1 * a[omp_get_thread_num() == 0 ? 0 : 1]);
    }

}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s = suite_create("Test");

    TCase* tc = tcase_create("omp_barrier");
    tcase_add_test(tc, omp_barrier_simple);

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
