#include "mir_lock.h"

#ifdef __tile__
//#include <tmc/sync.h>
#include <tmc/spin.h>
#else
#include <pthread.h>
#endif

void mir_lock_create(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
    // tmc_sync_mutex_init(&lock->m);
    tmc_spin_mutex_init(&lock->m);
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
    // tmc_sync_mutex_lock(&lock->m);
    tmc_spin_mutex_lock(&lock->m);
#else
    pthread_mutex_lock(&lock->m);
#endif
}/*}}}*/

void mir_lock_unset(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
    //tmc_sync_mutex_unlock(&lock->m);
    tmc_spin_mutex_unlock(&lock->m);
#else
    pthread_mutex_unlock(&lock->m);
#endif
}/*}}}*/

int mir_lock_tryset(struct mir_lock_t* lock)
{/*{{{*/
#ifdef __tile__
    //return tmc_sync_mutex_trylock(&lock->m);
    return tmc_spin_mutex_trylock(&lock->m);
#else
    return pthread_mutex_trylock(&lock->m);
#endif
}/*}}}*/

