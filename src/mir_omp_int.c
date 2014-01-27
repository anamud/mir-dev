#include "mir_omp_int.h"
#include "mir_task.h"

void GOMP_task(void (*fn)(void *), void *data, void (*copyfn)(void *, void *),
                long arg_size, long arg_align, bool if_clause, unsigned flags __attribute__((unused)))
{
    struct mir_task_t* task = mir_task_create_pw((mir_tfunc_t) fn, (void*) data, (size_t)(arg_size), 0, NULL, NULL);
    return;
}

void GOMP_taskwait(void)
{
    mir_twc_wait_pw();
}
