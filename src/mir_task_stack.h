#ifndef TASK_STACK_H
#define TASK_STACK_H 1

#include "mir_lock.h"
#include <stdint.h>
#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS

#ifdef TASK_STACK_DEBUG
static void TS_DBG(const char* msg, const struct mir_task_stack_t* q)
{ /*{{{*/
    fprintf(stderr, "%ld\t#%d size %d head \t%s\n",
        pthread_self(),
        q->head - 1, q->head,
        msg);
} /*}}}*/
#else
#define TS_DBG(msg, q)
#endif

// task stack state query macros
#define TASK_STACK_FULL(stack) ((stack)->head == (stack)->capacity)
#define TASK_STACK_EMPTY(stack) ((stack)->head == 0)

struct mir_task_stack_t { /*{{{*/
    struct mir_task_t** buffer;
    uint32_t capacity; // Max size of stack
    uint32_t head;     // Free location pointer, also size

    // Its a single-lock task stack
    struct mir_lock_t lock;
}; /*}}}*/

// Create a task stack.
void* mir_task_stack_create(uint32_t capacity);

// Destroy a task stack.
void mir_task_stack_destroy(struct mir_task_stack_t* stack);

// Add an element to the task stack.
int mir_task_stack_push(struct mir_task_stack_t* stack, struct mir_task_t* data);

// Remove an element from the task stack.
void mir_task_stack_pop(struct mir_task_stack_t* stack, struct mir_task_t** data);

// Get the current task stack size.
uint32_t mir_task_stack_size(const struct mir_task_stack_t* stack);

END_C_DECLS

#endif
