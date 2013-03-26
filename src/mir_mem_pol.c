#include "mir_mem_pol.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_defines.h"
#include "mir_debug.h"
#include "mir_runtime.h"

#ifndef __tile__
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#else
#include <tmc/alloc.h>
#include <tmc/mem.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

extern struct mir_runtime_t* runtime;

struct mem_header_t
{/*{{{*/
    uint64_t magic;
    size_t sz;
    uint16_t nodeid;
};/*}}}*/

static struct mem_header_t* get_mem_header(void* addr)
{/*{{{*/
    if(!addr)
        return NULL;

    struct mem_header_t* header = addr - sizeof(struct mem_header_t);
    if(header->magic == runtime->init_time)
    {
        return header;
    }
    else
    {
        return NULL;
    }
}/*}}}*/

static inline uint16_t get_node_of(void* addr)
{/*{{{*/
    int nodeid = runtime->arch->num_nodes+1;
    get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE|MPOL_F_ADDR);
    return (uint16_t) nodeid;
}/*}}}*/

struct mir_mem_node_dist_t* mir_mem_node_dist_create()
{/*{{{*/
    struct mir_mem_node_dist_t* dist = mir_malloc_int(sizeof(struct mir_mem_node_dist_t));
    if(dist == NULL) 
        MIR_ABORT(MIR_ERROR_STR "Cannot allocate memory!\n");

    dist->buf = mir_malloc_int(sizeof(size_t) * runtime->arch->num_nodes);
    if(dist->buf == NULL) 
        MIR_ABORT(MIR_ERROR_STR "Cannot allocate memory!\n");
    for(uint16_t i=0; i<runtime->arch->num_nodes; i++)
        dist->buf[i] = 0;

    return dist;
}/*}}}*/

void  mir_mem_node_dist_destroy(struct mir_mem_node_dist_t* dist)
{/*{{{*/
    mir_free_int(dist->buf, sizeof(size_t) * runtime->arch->num_nodes);
    mir_free_int(dist, sizeof(struct mir_mem_node_dist_t));
}/*}}}*/

void mir_mem_get_dist(struct mir_mem_node_dist_t* dist, void* addr, size_t sz, void* part_of)
{/*{{{*/
    if(addr == NULL || sz == 0 || dist == NULL)
        return;

    // Check if the part_of address contains a header
    // If yes, then the ... 
    // distribution is fully contained at the nodeid of the header. ...
    // Fill and return
    if(part_of)
    {
        struct mem_header_t* header = get_mem_header(part_of);
        if(header)
        {
            dist->buf[header->nodeid] += sz;
            return;
        }
    }

    // Check if the addr contains a header
    // If yes, then the ... 
    // distribution is fully contained at the nodeid of the header. ...
    // Fill and return
    struct mem_header_t* header = get_mem_header(addr);
    if(header)
    {
        dist->buf[header->nodeid] += sz;
        return;
    }

#ifndef __tile__
    // Now we take the costly route.
    // Ask libnuma for node distribution
    // Jump by page size
    uint16_t node = get_node_of(addr);
    uint16_t page_size = sysconf(_SC_PAGESIZE);

    if(sz <= page_size)
    dist->buf[node] += sz;

    uint16_t new_node = runtime->arch->num_nodes + 1;
    size_t ifrom = 0;
    size_t i;
    for (i=page_size; i<sz; i+=page_size) 
    {
        new_node = get_node_of(addr+i);
        if (new_node != node) 
        {
            dist->buf[node] += (i-ifrom);
            node = new_node;
            ifrom = i;
        }
    }

    if (new_node == node) 
        dist->buf[node] += (sz-ifrom);
#endif

}/*}}}*/

size_t mir_mem_node_dist_sum(struct mir_mem_node_dist_t* dist)
{/*{{{*/
    size_t sz = 0;
    for(int i=0; i<runtime->arch->num_nodes; i++)
        sz+= dist->buf[i];
    return sz;
}/*}}}*/

struct mir_mem_pol_t
{/*{{{*/
    struct mir_lock_t lock;
    uint16_t node;
    uint64_t total_allocated;

    // Interfaces
    void* (*allocate) (size_t sz);
    void  (*release) (void* addr, size_t sz);
};/*}}}*/

static struct mir_mem_pol_t* mem_pol;

static inline void advance_node()
{/*{{{*/
    mir_lock_set(&mem_pol->lock);
    mem_pol->node++;
    mem_pol->node = mem_pol->node % runtime->arch->num_nodes;
    mir_lock_unset(&mem_pol->lock);
}/*}}}*/

static void* allocate_coarse(size_t sz)
{/*{{{*/
    void* addr = NULL;
    advance_node();

#ifndef __tile__
    numa_set_bind_policy(1);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    numa_bitmask_clearall(mask);
    numa_bitmask_setbit(mask, mem_pol->node);
    numa_set_membind(mask);
    addr = numa_alloc_onnode(sz, mem_pol->node);
    numa_bitmask_free(mask);
    numa_set_bind_policy(0);
#ifdef MIR_MEM_POL_LOCK_PAGES
    mlock(addr, sz);
#endif
    // Fault it in
    memset(addr, 0, sz);
#else
    tmc_alloc_t home = TMC_ALLOC_INIT;
    tmc_alloc_set_home(&home, mem_pol->node);
    mir_page_attr_set(&home);
    addr = tmc_alloc_map(&home, sz);
#endif

    if(addr)
        __sync_fetch_and_add(&mem_pol->total_allocated, sz);

    return addr;
}/*}}}*/

static void* allocate_fine(size_t sz)
{/*{{{*/
    void* addr = NULL;

#ifndef __tile__
    int cur_pol;
    get_mempolicy(&cur_pol, NULL, 0, 0, 0);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    numa_bitmask_setall(mask);
    numa_set_interleave_mask(mask);
    addr = numa_alloc_interleaved_subset(sz, mask);
    set_mempolicy(cur_pol, mask->maskp, mask->size + 1);
    numa_bitmask_free(mask);
#ifdef MIR_MEM_POL_LOCK_PAGES
    mlock(addr, sz);
#endif
    memset(addr, 0, sz);
#else
    tmc_alloc_t home = TMC_ALLOC_INIT;
    tmc_alloc_set_home(&home, TMC_ALLOC_HOME_HASH);
    mir_page_attr_set(&home);
    addr = tmc_alloc_map(&home, sz);
#endif

    if(addr)
        __sync_fetch_and_add(&mem_pol->total_allocated, sz);

    return addr;
}/*}}}*/

static void* allocate_system(size_t sz)
{/*{{{*/
    return mir_malloc_int(sz);
}/*}}}*/

static void release_coarse(void* addr, size_t sz)
{/*{{{*/
#ifndef __tile__
#ifdef MIR_MEM_POL_LOCK_PAGES 
    munlock(addr, sz);
#endif 
    numa_free(addr, sz);
#else
    tmc_alloc_unmap(addr, sz);
#endif

    if(addr)
        __sync_fetch_and_sub(&mem_pol->total_allocated, sz);
}/*}}}*/

static void release_fine(void* addr, size_t sz)
{/*{{{*/
    release_coarse(addr, sz);
}/*}}}*/

static void release_system(void* addr, size_t sz)
{/*{{{*/
    free(addr);
}/*}}}*/

void mir_mem_pol_config (const char* conf_str)
{/*{{{*/
    char str[MIR_LONG_NAME_LEN];
    strcpy(str, conf_str);

    char* tok = strtok(str, " ");
    while(tok)
    {
        if(tok[0] == '-')
        {
            char c = tok[1];
            switch(c)
            {/*{{{*/
                case 'm':
                    if(tok[2] == '=')
                    {
                        char* pol = tok+3;
                        if(0 == strcmp(pol, "fine"))
                        {
                            mem_pol->allocate = allocate_fine;
                            mem_pol->release = release_fine;
                        }
                        else if(0 == strcmp(pol, "coarse"))
                        {
                            mem_pol->allocate = allocate_coarse;
                            mem_pol->release = release_coarse;
                        }
                        else
                        {
                            MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                        }

                        MIR_DEBUG(MIR_DEBUG_STR "Memory policy set to %s\n", pol);
                    }
                    break;
                default:
                    break;
            }/*}}}*/
        }
        tok = strtok(NULL, " ");
    }
}/*}}}*/

void mir_mem_pol_create ()
{/*{{{*/
    // Allocate mem_pol structure
    mem_pol = mir_malloc_int(sizeof(struct mir_mem_pol_t));
    if(!mem_pol)
        MIR_ABORT(MIR_ERROR_STR "Cannot create memory allocation policy!\n");
    // Statistics
    mem_pol->total_allocated = 0;
    // Node for coarse allocation
    // FIXME: This assumes there is always a node 0
    mem_pol->node = 0;
    // Lock
    mir_lock_create(&mem_pol->lock);
    // Default allocation
    mem_pol->allocate = allocate_system;
    mem_pol->release = release_system;

    // Low-level stuff
#ifndef __tile__
    numa_set_strict(1);
#endif
}/*}}}*/

void mir_mem_pol_destroy ()
{/*{{{*/
    MIR_DEBUG(MIR_DEBUG_STR "Total unfreed mem_pol memory=%" MIR_FORMSPEC_UL " bytes\n", mem_pol->total_allocated);
    mir_lock_destroy(&mem_pol->lock);
    mir_free_int(mem_pol, sizeof(struct mir_mem_pol_t));
}/*}}}*/

void* mir_mem_pol_allocate (size_t sz)
{/*{{{*/
    return mem_pol->allocate(sz);
}/*}}}*/

void mir_mem_pol_release (void* addr, size_t sz)
{/*{{{*/
    mem_pol->release(addr, sz);
}/*}}}*/

