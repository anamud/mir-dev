#include "mir_utils.h"
#include <sys/resource.h>

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
