#include "mir_utils.h"
#include "mir_runtime.h"
#include "mir_worker.h"

#include <sys/resource.h>
#include <stdlib.h>
#include <stdint.h>

extern struct mir_runtime_t* runtime; 

int mir_pstack_set_size(size_t sz)
{/*{{{*/
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if(result == 0)
    {
        if(rl.rlim_cur < sz)
        {
            rl.rlim_cur = sz;
            result = setrlimit(RLIMIT_STACK, &rl);
        }
    }

    return result;
}/*}}}*/

int mir_get_num_threads()
{/*{{{*/
    return runtime->num_workers;
}/*}}}*/

int mir_get_threadid()
{/*{{{*/
    struct mir_worker_t* worker = mir_worker_get_context();
    return worker->id;
}/*}}}*/


