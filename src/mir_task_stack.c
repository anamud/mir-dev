#include "mir_task_stack.h"
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_task.h"
#include "mir_runtime.h"

void* mir_task_stack_create(uint32_t capacity)
{ /*{{{*/
    struct mir_task_stack_t* stack = mir_cmalloc_int(sizeof(struct mir_task_stack_t));
    MIR_CHECK_MEM(stack != NULL);

    stack->buffer = mir_cmalloc_int(capacity * sizeof(void*));
    MIR_CHECK_MEM(stack->buffer != NULL);

    stack->capacity = capacity;
    stack->head = 0;

    mir_lock_create(&(stack->lock));
    return stack;
} /*}}}*/

void mir_task_stack_destroy(struct mir_task_stack_t* stack)
{ /*{{{*/
    MIR_ASSERT(stack != NULL);
    MIR_ASSERT(stack->buffer != NULL);

    mir_lock_destroy(&(stack->lock));
    mir_free_int(stack->buffer, stack->capacity * sizeof(void*));
    stack->buffer = NULL;
    mir_free_int(stack, sizeof(struct mir_task_stack_t));
    stack = NULL;
} /*}}}*/

int mir_task_stack_push(struct mir_task_stack_t* stack, struct mir_task_t* data)
{ /*{{{*/
    MIR_ASSERT(stack != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(stack->lock));

    if (TASK_STACK_FULL(stack)) {
        TS_DBG("stack full!", stack);
        mir_lock_unset(&(stack->lock));
        return 0;
    }
    stack->buffer[stack->head] = (void*)data;
    stack->head++;

    mir_lock_unset(&(stack->lock));
    return 1;
} /*}}}*/

void mir_task_stack_pop(struct mir_task_stack_t* stack, struct mir_task_t** data)
{ /*{{{*/
    MIR_ASSERT(stack != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(stack->lock));
    if (TASK_STACK_EMPTY(stack)) {
        TS_DBG("stack empty", stack);
        goto cleanup;
    }

    // Ensure we have executed our parallel block before this task.
    struct mir_task_t* task = stack->buffer[stack->head - 1];
    struct mir_worker_t* worker = mir_worker_get_context();
    if (!runtime->single_parallel_block && task->team &&
        task->team->parallel_block_flag[worker->id] == 0)
        goto cleanup;

    *data = stack->buffer[stack->head - 1];
    MIR_ASSERT(*data != NULL);
    stack->head--;

cleanup:
    mir_lock_unset(&(stack->lock));
} /*}}}*/

uint32_t mir_task_stack_size(const struct mir_task_stack_t* stack)
{ /*{{{*/
    MIR_ASSERT(stack != NULL);

    return stack->head;
} /*}}}*/

