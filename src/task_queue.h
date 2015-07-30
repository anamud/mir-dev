#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H 1

#include "mir_lock.h"
#include <stdint.h>
#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS

#ifdef TASK_QUEUE_DEBUG
static void TQ_DBG(const char* msg, const struct task_queue_t* q)
{ /*{{{*/
    fprintf(stderr, "%ld\t#%d in %d out %d\t%s\n",
        pthread_self(),
        q->size, q->in, q->out,
        msg);
} /*}}}*/
#else
#define TQ_DBG(msg, q)
#endif

// Queue state query macros
#define TASK_QUEUE_FULL(queue) ((queue)->size == (queue)->capacity)
#define TASK_QUEUE_EMPTY(queue) ((queue)->size == 0)

struct task_queue_t { /*{{{*/
    struct mir_task_t** buffer;
    uint32_t capacity; // Max size of queue
    uint32_t size;     // Number of elements
    uint32_t in;       // Heads
    uint32_t out;

    // Its a two-lock queue
    struct mir_lock_t enq_lock;
    struct mir_lock_t deq_lock;
}; /*}}}*/

// Create a task queue.
void* task_queue_create(uint32_t capacity);

// Destroy a task queue.
void task_queue_destroy(struct task_queue_t* queue);

// Add an element to the task queue.
int task_queue_push(struct task_queue_t* queue, struct mir_task_t* task);

// Remove an element from the task queue.
struct mir_task_t* task_queue_pop(struct task_queue_t* queue);

// Get task queue size
uint32_t task_queue_size(const struct task_queue_t* queue);

END_C_DECLS

#endif
