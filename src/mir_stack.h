#ifndef MIR_STACK_H
#define MIR_STACK_H 1

#include "mir_lock.h"
#include <stdint.h>
#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS

#ifdef MIR_STACK_DEBUG
static void S_DBG(const char* msg, const struct mir_stack_t* q)
{ /*{{{*/
    fprintf(stderr, "%ld\t#%d size %d head \t%s\n",
        pthread_self(),
        q->head - 1, q->head,
        msg);
} /*}}}*/
#else
#define S_DBG(msg, q)
#endif

// stack state query macros
#define STACK_FULL(stack) ((stack)->head == (stack)->capacity)
#define STACK_EMPTY(stack) ((stack)->head == 0)

struct mir_stack_t { /*{{{*/
    void** buffer;
    uint32_t capacity; // Max size of stack
    uint32_t head;     // Free location pointer, also size

    // Its a single-lock stack
    struct mir_lock_t lock;
}; /*}}}*/

// Create a stack
struct mir_stack_t* mir_stack_create(uint32_t capacity);

// Destroy a stack
void mir_stack_destroy(struct mir_stack_t* stack);

// Add an element to the stack
int mir_stack_push(struct mir_stack_t* stack, void* data);

// Remove an element from the stack
void mir_stack_pop(struct mir_stack_t* stack, void** data);

// Get the cuurent stack size
uint32_t mir_stack_size(const struct mir_stack_t* stack);

END_C_DECLS

#endif
