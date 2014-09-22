#include "mir_lock.h"

// Uncomment this to use sync mutex on the tilepro64
//#define TILEPRO_USE_SYNCMUTEX

#ifdef __tile__
#ifdef TILEPRO_USE_SYNCMUTEX
#include <tmc/sync.h>
#else
#include <tmc/spin.h>
#endif
#else
#include <pthread.h>
#endif

void mir_lock_create(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
#ifdef TILEPRO_USE_SYNCMUTEX
    tmc_sync_mutex_init(&lock->m);
#else
    tmc_spin_mutex_init(&lock->m);
#endif
#else
    pthread_mutex_init(&lock->m, NULL);
#endif
}/*}}}*/

void mir_lock_destroy(struct mir_lock_t* lock)
{/*{{{*/
#ifndef __tile__
    pthread_mutex_destroy(&lock->m);
#endif
}/*}}}*/

void mir_lock_set(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
#ifdef TILEPRO_USE_SYNCMUTEX
    tmc_sync_mutex_lock(&lock->m);
#else
    tmc_spin_mutex_lock(&lock->m);
#endif
#else
    pthread_mutex_lock(&lock->m);
#endif
}/*}}}*/

void mir_lock_unset(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
#ifdef TILEPRO_USE_SYNCMUTEX
    tmc_sync_mutex_unlock(&lock->m);
#else
    tmc_spin_mutex_unlock(&lock->m);
#endif
#else
    pthread_mutex_unlock(&lock->m);
#endif
}/*}}}*/

int mir_lock_tryset(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
#ifdef TILEPRO_USE_SYNCMUTEX
    return tmc_sync_mutex_trylock(&lock->m);
#else
    return tmc_spin_mutex_trylock(&lock->m);
#endif
#else
    return pthread_mutex_trylock(&lock->m);
#endif
}/*}}}*/

