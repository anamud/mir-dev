#ifndef MIR_LOCK_H
#define MIR_LOCK_H 1

#include "mir_types.h"

/*PUB_INT_INC_BEGIN*/
// Uncomment this to use sync mutex on the tilepro64
//#define TILEPRO_USE_SYNCMUTEX
#ifdef __tile__
#ifdef TILEPRO_USE_SYNCMUTEX
#include <tmc/sync.h>
#define MIR_LOCK_INITIALIZER TMC_SYNC_MUTEX_INIT
#else
#include <tmc/spin.h>
#define MIR_LOCK_INITIALIZER TMC_SPIN_MUTEX_INIT
#endif
#else
#include <pthread.h>
#define MIR_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif
/*PUB_INT_INC_END*/

BEGIN_C_DECLS

/*PUB_INT_BASE_DECL_BEGIN*/
struct mir_lock_t { /*{{{*/
#ifdef __tile__
    //tmc_sync_mutex_t m;
    tmc_spin_mutex_t m;
#else
    pthread_mutex_t m;
#endif
}; /*}}}*/
/*PUB_INT_BASE_DECL_END*/

/*PUB_INT_BEGIN*/
void mir_lock_create(struct mir_lock_t* lock);

void mir_lock_destroy(struct mir_lock_t* lock);

void mir_lock_set(struct mir_lock_t* lock);

void mir_lock_unset(struct mir_lock_t* lock);

int mir_lock_tryset(struct mir_lock_t* lock);
/*PUB_INT_END*/

END_C_DECLS

#endif

