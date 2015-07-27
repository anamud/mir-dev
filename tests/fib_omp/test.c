#include <stdlib.h>
#include <stdint.h>
#include <check.h>
#include <omp.h>

static uint64_t fib_seq(int n)
{ /*{{{*/
    if (n < 2)
        return n;
    return fib_seq(n - 1) + fib_seq(n - 2);
} /*}}}*/

static uint64_t fib(int n, int d, int cutoff)
{ /*{{{*/
    uint64_t x, y;

    if (n < 2)
        return n;

    if (d < cutoff)
    {
#pragma omp task shared(x) firstprivate(n, d)
        x = fib(n - 1, d + 1, cutoff);

#pragma omp task shared(y) firstprivate(n, d)
        y = fib(n - 2, d + 1, cutoff);

#pragma omp taskwait
    }
    else
    {
        x = fib_seq(n - 1);
        y = fib_seq(n - 2);
    }

    return x + y;
} /*}}}*/

START_TEST(fib_omp)
{/*{{{*/
    int result;
    int n = 42;
    int cutoff = 12;

#pragma omp parallel
    {
#pragma omp single
        {
#pragma omp task firstprivate(n, cutoff)
            {
                result = fib(n, 0, cutoff);
            }
#pragma omp taskwait
        }
    }

    /* fib(42) == 267914296 */
    ck_assert_int_eq(result, 267914296);
}/*}}}*/
END_TEST

Suite* test_suite(void)
{/*{{{*/
    Suite* s;
    s = suite_create("Test");

    TCase* tc = tcase_create("fib_omp");
    tcase_add_test(tc, fib_omp);
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

