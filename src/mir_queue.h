#ifndef MIR_QUEUE_H
#define MIR_QUEUE_H 1

#include "mir_lock.h"
#include <stdint.h>
#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS 

#ifdef MIR_QUEUE_DEBUG
static void Q_DBG(const char*msg, const struct mir_queue_t *q) 
{/*{{{*/
    fprintf(stderr, "%ld\t#%d in %d out %d\t%s\n", 
                    pthread_self(),
                    q->size, q->in, q->out,
                    msg
                    );
}/*}}}*/
#else
#define Q_DBG(msg,q) 
#endif

// Queue state query macros
#define QUEUE_FULL(queue) ((queue)->size == (queue)->capacity)
#define QUEUE_EMPTY(queue) ((queue)->size == 0)

struct mir_queue_t
{/*{{{*/
    void** buffer;
    uint32_t capacity;  // Max size of queue
    uint32_t size;      // Number of elements
    uint32_t in;        // Heads
    uint32_t out;

    // Its a two-lock queue
    struct mir_lock_t enq_lock;
    struct mir_lock_t deq_lock;
};/*}}}*/

// Create a queue
struct mir_queue_t* mir_queue_create(uint32_t capacity);

// Destroy a queue
void mir_queue_destroy(struct mir_queue_t* queue);

// Add an element to the queue
int mir_queue_push(struct mir_queue_t* queue, void* data);

// Remove an element from the queue
void mir_queue_pop(struct mir_queue_t* queue, void** data);

// Get queue size
uint32_t mir_queue_size(const struct mir_queue_t* queue);

END_C_DECLS 

#endif
