#ifndef  MIR_LOCK_H
#define  MIR_LOCK_H 1

#ifdef __tile__
//#include <tmc/sync.h>
//#define MIR_LOCK_INITIALIZER TMC_SYNC_MUTEX_INIT
#include <tmc/spin.h>
#define MIR_LOCK_INITIALIZER TMC_SPIN_MUTEX_INIT
#else
#include <pthread.h>
#define MIR_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

struct mir_lock_t
{/*{{{*/
#ifdef __tile__
    //tmc_sync_mutex_t m;
    tmc_spin_mutex_t m;
#else
    pthread_mutex_t m;
#endif
};/*}}}*/

void mir_lock_create(struct mir_lock_t* lock);

void mir_lock_destroy(struct mir_lock_t* lock);

void mir_lock_set(struct mir_lock_t* lock);

void mir_lock_unset(struct mir_lock_t* lock);

int mir_lock_tryset(struct mir_lock_t* lock);

#endif

