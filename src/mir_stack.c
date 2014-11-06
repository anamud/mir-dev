#include "mir_stack.h"
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_defines.h"
#include "mir_lock.h"

#include <stdbool.h>

struct mir_stack_t* mir_stack_create(uint32_t capacity)
{/*{{{*/
    struct mir_stack_t* stack = (struct mir_stack_t*)mir_cmalloc_int(sizeof(struct mir_stack_t));
    MIR_ASSERT(stack != NULL);

    stack->buffer = NULL;
    stack->buffer = mir_cmalloc_int(capacity * sizeof(void*));
    MIR_ASSERT(stack->buffer != NULL);

    stack->capacity = capacity;
    stack->head = 0;

    mir_lock_create(&(stack->lock));
    return stack;
}/*}}}*/

void mir_stack_destroy(struct mir_stack_t* stack)
{/*{{{*/
    MIR_ASSERT(stack != NULL);
    MIR_ASSERT(stack->buffer != NULL);

    mir_lock_destroy(&(stack->lock));
    mir_free_int(stack->buffer, stack->capacity * sizeof(void*));
    stack->buffer = NULL;
    mir_free_int(stack, sizeof(struct mir_stack_t));
    stack = NULL;
}/*}}}*/

bool mir_stack_push(struct mir_stack_t* stack, void* data)
{/*{{{*/
    MIR_ASSERT(stack != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(stack->lock));

    if (STACK_FULL(stack))
    {
        S_DBG("stack full!", stack);
        //__sync_synchronize();
        mir_lock_unset(&(stack->lock));
        return false;
    }
    stack->buffer[stack->head] = (void*) data;
    stack->head++;

    //__sync_synchronize();
    mir_lock_unset(&(stack->lock));

    return true;
}/*}}}*/

void mir_stack_pop(struct mir_stack_t* stack, void** data)
{/*{{{*/
    MIR_ASSERT(stack != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(stack->lock));
    if (STACK_EMPTY(stack))
    {
        S_DBG("stack empty", stack);
        mir_lock_unset(&(stack->lock));
        return;
    }
    *data = stack->buffer[stack->head - 1];
    MIR_ASSERT(*data != NULL);
    stack->head--;
    //__sync_synchronize();
    mir_lock_unset(&(stack->lock));
    return;
}/*}}}*/

uint32_t mir_stack_size(const struct mir_stack_t *stack)
{/*{{{*/
    MIR_ASSERT(stack != NULL);

    //__sync_synchronize();
    //mir_lock_set(&(stack->lock));
    uint32_t size = stack->head ;
    //mir_lock_unset(&(stack->lock));
    return size;
}/*}}}*/

