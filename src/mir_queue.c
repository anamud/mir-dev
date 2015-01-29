#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_queue.h"
#include "mir_utils.h"

struct mir_queue_t* mir_queue_create(uint32_t capacity)
{/*{{{*/
    MIR_ASSERT(capacity > 0);

    struct mir_queue_t* queue = (struct mir_queue_t*)mir_cmalloc_int(sizeof(struct mir_queue_t));
    MIR_ASSERT(queue != NULL);

    queue->buffer = NULL;
    queue->buffer = mir_cmalloc_int(capacity * sizeof(void*));
    MIR_ASSERT(queue->buffer != NULL);

    queue->capacity = capacity;
    queue->size = 0;
    queue->in = 0;
    queue->out = 0;

    mir_lock_create(&(queue->enq_lock));
    mir_lock_create(&(queue->deq_lock));

    return queue;
}/*}}}*/

void mir_queue_destroy(struct mir_queue_t* queue)
{/*{{{*/
    MIR_ASSERT(queue != NULL);

    mir_lock_destroy(&(queue->enq_lock));
    mir_lock_destroy(&(queue->deq_lock));
    mir_free_int(queue->buffer, queue->capacity * sizeof(void*));
    queue->buffer = NULL;
    mir_free_int(queue, sizeof(struct mir_queue_t));
    queue = NULL;
}/*}}}*/

int mir_queue_push(struct mir_queue_t* queue, void* data)
{/*{{{*/
    MIR_ASSERT(queue != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(queue->enq_lock));

    if(QUEUE_FULL(queue)) 
    {
        Q_DBG("queue full!", queue);
        //__sync_synchronize();
        mir_lock_unset(&(queue->enq_lock));
        return 0;
    }

    queue->buffer[queue->in] = data;
    queue->in++;
    if (queue->in >= queue->capacity)
        queue->in -= queue->capacity;
    __sync_fetch_and_add(&(queue->size), 1);

    //__sync_synchronize();
    mir_lock_unset(&(queue->enq_lock));

    return 1;
}/*}}}*/

void mir_queue_pop(struct mir_queue_t* queue, void** data)
{/*{{{*/
    MIR_ASSERT(queue != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(queue->deq_lock));
    if (QUEUE_EMPTY(queue)) 
    {
        Q_DBG("queue empty!", queue);
        mir_lock_unset(&(queue->deq_lock));
        return;
    }

    *data = queue->buffer[queue->out];
    MIR_ASSERT(*data != NULL);
    __sync_fetch_and_sub(&(queue->size), 1);

    queue->out++;
    if (queue->out >= queue->capacity)
        queue->out -= queue->capacity;

    //__sync_synchronize();
    mir_lock_unset(&(queue->deq_lock));
    return;
}/*}}}*/

uint32_t mir_queue_size(const struct mir_queue_t *queue)
{/*{{{*/
    MIR_ASSERT(queue != NULL);
    
    //__sync_synchronize();
    //mir_lock_set(&(queue->lock));
    uint32_t size = queue->size ;
    //mir_lock_unset(&(queue->lock));
    return size;
}/*}}}*/

