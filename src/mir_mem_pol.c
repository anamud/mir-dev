#include "mir_mem_pol.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_defines.h"
#include "mir_debug.h"
#include "mir_debug.h"
#include "mir_runtime.h"
#include "mir_worker.h"

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
#include <math.h>
#include <unistd.h>

extern struct mir_runtime_t* runtime;

struct mem_header_t
{/*{{{*/
    uint64_t magic;
    size_t sz;
    uint16_t nodeid;
    uint16_t* node_cache;
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

static inline uint16_t get_node_of(void* addr, void* base_addr)
{/*{{{*/
#ifdef MIR_MEM_POL_CACHE_NODES 
    if(base_addr)
    {
        // Get node from page cache
        struct mem_header_t* header = base_addr - sizeof(struct mem_header_t);
        if(header->node_cache)
        {
            int page_num = (addr - base_addr) / sysconf(_SC_PAGESIZE);
            int cached_nodeid = header->node_cache[page_num];
            /*int nodeid = runtime->arch->num_nodes+1;*/
            /*get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE|MPOL_F_ADDR);*/
            /*MIR_ASSERT(cached_nodeid == nodeid);*/
            return cached_nodeid;
        }
        else
        {
            int nodeid = runtime->arch->num_nodes+1;
            get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE|MPOL_F_ADDR);
            return (uint16_t) nodeid;
        }
    }
    else
    {
        int nodeid = runtime->arch->num_nodes+1;
        get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE|MPOL_F_ADDR);
        return (uint16_t) nodeid;
    }
#else
    int nodeid = runtime->arch->num_nodes+1;
    get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE|MPOL_F_ADDR);
    return (uint16_t) nodeid;
#endif
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

static void print_dist(struct mir_mem_node_dist_t* dist)
{/*{{{*/
    MIR_INFORM("Dist: ");
    for(int i=0; i<runtime->arch->num_nodes; i++)
        MIR_INFORM("%lu ", dist->buf[i]);
    MIR_INFORM("\n");
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
            //print_dist(dist);
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
        //print_dist(dist);
        return;
    }

#ifndef __tile__
    // Now we take the costly route.
    // Ask libnuma for node distribution
    // Jump by page size
    // If node localtions are cached, then we can save some time.
    // See code under MIR_MEM_POL_CACHE_NODES
    uint16_t node = get_node_of(addr, part_of);
    uint16_t page_size = sysconf(_SC_PAGESIZE);

    if(sz <= page_size)
    dist->buf[node] += sz;

    uint16_t new_node = runtime->arch->num_nodes + 1;
    size_t ifrom = 0;
    size_t i;
    for (i=page_size; i<sz; i+=page_size) 
    {
        new_node = get_node_of(addr+i, part_of);
        if (new_node != node) 
        {
            dist->buf[node] += (i-ifrom);
            node = new_node;
            ifrom = i;
        }
    }

    if (new_node == node) 
        dist->buf[node] += (sz-ifrom);

    //print_dist(dist);
#endif
}/*}}}*/

/*size_t mir_mem_node_dist_sum(struct mir_mem_node_dist_t* dist)*/
/*{[>{{{<]*/
    /*size_t sz = 0;*/
    /*for(int i=0; i<runtime->arch->num_nodes; i++)*/
        /*sz+= dist->buf[i];*/
    /*return sz;*/
/*}[>}}}<]*/

void mir_mem_node_dist_stat(struct mir_mem_node_dist_t* dist, struct mir_mem_node_dist_stat_t* stat)
{/*{{{*/
    stat->sum = 0;
    stat->min = -1;
    stat->max = 0;
    for(int i=0; i<runtime->arch->num_nodes; i++)
    {
        size_t val = dist->buf[i];
        stat->sum += val;
        if(stat->min > val)
            stat->min = val;
        if(stat->max < val)
            stat->max = val;
    }
    stat->mean = stat->sum / (runtime->arch->num_nodes);
    double sum_dsq = 0;
    for(int i=0; i<runtime->arch->num_nodes; i++)
    {
        double diff = ((double)(dist->buf[i]) - stat->mean);
        sum_dsq = (diff * diff);
    }
    stat->sd = sqrt(sum_dsq / (runtime->arch->num_nodes));
}/*}}}*/

struct mir_mem_pol_t
{/*{{{*/
    struct mir_lock_t lock;
    uint16_t node;
    uint64_t total_allocated;

    // Interfaces
    void* (*allocate) (size_t sz);
    void  (*release) (void* addr, size_t sz);
    void  (*reset) ();
};/*}}}*/

static struct mir_mem_pol_t* mem_pol;

static inline void reset_node()
{/*{{{*/
    mir_lock_set(&mem_pol->lock);
    mem_pol->node = 0;
    mir_lock_unset(&mem_pol->lock);
}/*}}}*/

static inline void advance_node()
{/*{{{*/
    mir_lock_set(&mem_pol->lock);
    mem_pol->node++;
    mem_pol->node = mem_pol->node % runtime->arch->num_nodes;
    mir_lock_unset(&mem_pol->lock);
}/*}}}*/

static inline void fault_all_pages(void* addr, size_t sz)
{/*{{{*/
    uint16_t pagesz = sysconf(_SC_PAGESIZE);
    size_t num_pages = (sz +  pagesz - 1)/pagesz;
    for(size_t i=0; i<num_pages; i++)
    {
        *((char*)(addr) + (i*pagesz)) = 0;
    }
}/*}}}*/

static void* allocate_coarse(size_t sz)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    void* addr = NULL;
    size_t new_sz = sz + sizeof(struct mem_header_t);

#ifndef __tile__
    numa_set_bind_policy(1);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    numa_bitmask_clearall(mask);
    numa_bitmask_setbit(mask, mem_pol->node);
    numa_set_membind(mask);
    addr = numa_alloc_onnode(new_sz, mem_pol->node);
    numa_bitmask_free(mask);
    numa_set_bind_policy(0);
#ifdef MIR_MEM_POL_LOCK_PAGES
    mlock(addr, new_sz);
#endif
#ifdef MIR_MEM_POL_FAULT_IN_PAGES
    // Fault it in
    //memset(addr, 0, new_sz);
    fault_all_pages(addr, new_sz);
#endif
#else
    tmc_alloc_t home = TMC_ALLOC_INIT;
    tmc_alloc_set_home(&home, mem_pol->node);
    mir_page_attr_set(&home);
    addr = tmc_alloc_map(&home, new_sz);
#endif

    if(addr)
    {
        // Write header
        struct mem_header_t* header = (struct mem_header_t*) addr;
        header->magic = runtime->init_time;
        header->sz = sz;
        header->nodeid = mem_pol->node;
        header->node_cache = NULL;

        __sync_fetch_and_add(&mem_pol->total_allocated, new_sz);
    }

    advance_node();

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*) ((unsigned char*)addr + sizeof(struct mem_header_t));
}/*}}}*/

static void* allocate_fine(size_t sz)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    void* addr = NULL;
    size_t new_sz = sz + sizeof(struct mem_header_t);
    bool faulted_in = false;

#ifndef __tile__
    int cur_pol;
    get_mempolicy(&cur_pol, NULL, 0, 0, 0);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    numa_bitmask_setall(mask);
    numa_set_interleave_mask(mask);
    addr = numa_alloc_interleaved_subset(new_sz, mask);
    set_mempolicy(cur_pol, mask->maskp, mask->size + 1);
    numa_bitmask_free(mask);
#ifdef MIR_MEM_POL_LOCK_PAGES
    mlock(addr, new_sz);
#endif
#ifdef MIR_MEM_POL_FAULT_IN_PAGES
    // Fault it in
    //memset(addr, 0, new_sz);
    fault_all_pages(addr, new_sz);
    faulted_in = true;
#endif
#else
    tmc_alloc_t home = TMC_ALLOC_INIT;
    tmc_alloc_set_home(&home, TMC_ALLOC_HOME_HASH);
    mir_page_attr_set(&home);
    addr = tmc_alloc_map(&home, new_sz);
#endif

    if(addr)
    {
        // Write header
        struct mem_header_t* header = (struct mem_header_t*) addr;
        header->magic = 0;
        header->sz = sz;
        header->nodeid = runtime->arch->num_nodes + 1;
#ifdef MIR_MEM_POL_CACHE_NODES 
        // Have to fault in the pages!
        // Or else the query will not work
        if(faulted_in == false)
        {
            //memset(addr, 0, new_sz);
            fault_all_pages(addr, new_sz);
            faulted_in = true;
        }

        // Create node cache
        uint16_t pagesz = sysconf(_SC_PAGESIZE);
        size_t num_pages = (new_sz +  pagesz - 1)/pagesz;
        header->node_cache = mir_malloc_int(sizeof(uint16_t) * num_pages);
        for(size_t i=0; i<num_pages; i++)
        {
            uint16_t node = get_node_of(addr + (i*pagesz), NULL);
            header->node_cache[i] = node;
        }
        /*MIR_INFORM("Node cache: ");*/
        /*for(size_t i=0; i<num_pages; i++)*/
            /*MIR_INFORM("%lu->%d ", i, header->node_cache[i]);*/
        /*MIR_INFORM("\n");*/
#else
        header->node_cache = NULL;
#endif

        __sync_fetch_and_add(&mem_pol->total_allocated, sz);
    }

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*) ((unsigned char*)addr + sizeof(struct mem_header_t));
}/*}}}*/

static void* allocate_system(size_t sz)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    size_t new_sz = sz + sizeof(struct mem_header_t);
    void* addr = malloc(new_sz);
    if(addr)
    {
        // Write header
        struct mem_header_t* header = (struct mem_header_t*) addr;
        header->magic = 0;
        header->sz = sz;
        header->nodeid = runtime->arch->num_nodes + 1;
        header->node_cache = NULL;
    }

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*) ((unsigned char*)addr + sizeof(struct mem_header_t));
}/*}}}*/

static void* allocate_local(size_t sz)
{/*{{{*/
    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    void* addr = NULL;
    size_t new_sz = sz + sizeof(struct mem_header_t);

    // Get this worker's node
    struct mir_worker_t* worker = mir_worker_get_context();
    int node = runtime->arch->node_of(worker->id);

#ifndef __tile__
    numa_set_bind_policy(1);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    numa_bitmask_clearall(mask);
    numa_bitmask_setbit(mask, node);
    numa_set_membind(mask);
    addr = numa_alloc_onnode(new_sz, node);
    numa_bitmask_free(mask);
    numa_set_bind_policy(0);
#ifdef MIR_MEM_POL_LOCK_PAGES
    mlock(addr, new_sz);
#endif
#ifdef MIR_MEM_POL_FAULT_IN_PAGES
    // Fault it in
    //memset(addr, 0, new_sz);
    fault_all_pages(addr, new_sz);
#endif
#else
    tmc_alloc_t home = TMC_ALLOC_INIT;
    tmc_alloc_set_home(&home, node);
    mir_page_attr_set(&home);
    addr = tmc_alloc_map(&home, new_sz);
#endif

    if(addr)
    {
        // Write header
        struct mem_header_t* header = (struct mem_header_t*) addr;
        header->magic = runtime->init_time;
        header->sz = sz;
        header->nodeid = node;
        header->node_cache = NULL;

        __sync_fetch_and_add(&mem_pol->total_allocated, new_sz);
    }

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*) ((unsigned char*)addr + sizeof(struct mem_header_t));
}/*}}}*/

static void release_coarse(void* addr, size_t sz)
{/*{{{*/
    size_t new_sz = sz + sizeof(struct mem_header_t);
    void* new_addr = (void*)((char*)addr - sizeof(struct mem_header_t));
#ifdef MIR_MEM_POL_CACHE_NODES
    // Release node cache
    struct mem_header_t* header = (struct mem_header_t*)(new_addr);
    if(header->node_cache)
    {
        uint16_t pagesz = sysconf(_SC_PAGESIZE);
        size_t num_pages = (new_sz +  pagesz - 1)/pagesz;
        mir_free_int(header->node_cache, sizeof(uint16_t) * num_pages);
    }
#endif
#ifndef __tile__
#ifdef MIR_MEM_POL_LOCK_PAGES 
    munlock(new_addr, new_sz);
#endif 
    numa_free(new_addr, new_sz);
#else
    tmc_alloc_unmap(new_addr, new_sz);
#endif

    if(addr)
        __sync_fetch_and_sub(&mem_pol->total_allocated, new_sz);
}/*}}}*/

static void release_fine(void* addr, size_t sz)
{/*{{{*/
    release_coarse(addr, sz);
}/*}}}*/

static void release_system(void* addr, size_t sz)
{/*{{{*/
    void* new_addr = (void*)((char*)addr - sizeof(struct mem_header_t));
    free(new_addr);
}/*}}}*/

static void release_local(void* addr, size_t sz)
{/*{{{*/
    release_coarse(addr, sz);
}/*}}}*/

static void reset_common()
{/*{{{*/
}/*}}}*/

static void reset_coarse()
{/*{{{*/
    reset_node();
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
                            mem_pol->reset = reset_coarse;
                        }
                        else if(0 == strcmp(pol, "system"))
                        {
                            mem_pol->allocate = allocate_system;
                            mem_pol->release = release_system;
                        }
                        else if(0 == strcmp(pol, "local"))
                        {
                            mem_pol->allocate = allocate_local;
                            mem_pol->release = release_local;
                        }
                        else
                        {
                            MIR_ABORT(MIR_ERROR_STR "Incorrect MIR_CONF parameter [%c]\n", c);
                        }

                        MIR_DEBUG(MIR_DEBUG_STR "Memory allocation policy changed to %s\n", pol);
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
    mem_pol->node =0;
    // Lock
    mir_lock_create(&mem_pol->lock);
    // Default allocation
    mem_pol->allocate = allocate_system;
    mem_pol->release = release_system;
    mem_pol->reset = reset_common;
    MIR_DEBUG(MIR_DEBUG_STR "Memory allocation policy set to %s\n", "system");

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
    /*void* block =  mem_pol->allocate(sz);*/
    /*struct mir_mem_node_dist_t* dist = mir_mem_node_dist_create();*/
    /*mir_mem_node_dist_destroy(dist);*/
    /*mir_mem_get_dist(dist, block, sz, NULL);*/
    /*print_dist(dist);*/
    /*return block;*/
}/*}}}*/

void mir_mem_pol_release (void* addr, size_t sz)
{/*{{{*/
    mem_pol->release(addr, sz);
}/*}}}*/

void mir_mem_pol_reset ()
{/*{{{*/
    mem_pol->reset();
}/*}}}*/
