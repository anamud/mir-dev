#ifndef MIR_OMP_INT_H
#define MIR_OMP_INT_H

#include <stdbool.h>

void GOMP_task(void (*fn)(void *), void *data, void (*copyfn)(void *, void *),
                long arg_size, long arg_align, bool if_clause, unsigned flags __attribute__((unused)));
void GOMP_taskwait(void);
#endif
