#include "mir_mem_pol.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_defines.h"
#include "mir_utils.h"
#include "mir_runtime.h"
#include "mir_worker.h"
#include "arch/mir_arch.h"

#ifdef MIR_MEM_POL_ENABLE
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
#include <math.h>
#include <unistd.h>

// FIXME: nodes are inconsistently u16
struct mem_header_t { /*{{{*/
    uint64_t magic;
    size_t sz;
    uint16_t nodeid;
    uint16_t* node_cache;
}; /*}}}*/

static struct mem_header_t* get_mem_header(void* addr)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    struct mem_header_t* header = addr - sizeof(struct mem_header_t);
    if (header->magic == runtime->init_time)
        return header;
    else
        return NULL;
} /*}}}*/

static inline uint16_t get_node_from_system(void* addr)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    int nodeid = runtime->arch->num_nodes + 1;
    get_mempolicy(&nodeid, NULL, 0, (void*)addr, MPOL_F_NODE | MPOL_F_ADDR);
    MIR_ASSERT(nodeid < runtime->arch->num_nodes);
    return (uint16_t)nodeid;
} /*}}}*/

static inline uint16_t get_node_of(void* addr, void* base_addr)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
#ifdef MIR_MEM_POL_CACHE_NODES
    if (base_addr) {
        // Get node from page cache
        struct mem_header_t* header = base_addr - sizeof(struct mem_header_t);
        if (header->node_cache) {
            int page_num = (addr - base_addr) / sysconf(_SC_PAGESIZE);
            int cached_nodeid = header->node_cache[page_num];
            /*MIR_ASSERT(cached_nodeid == get_node_from_system(addr));*/
            return cached_nodeid;
        }
        else {
            return get_node_from_system(addr);
        }
    }
    else {
        return get_node_from_system(addr);
    }
#else
    return get_node_from_system(addr);
#endif
} /*}}}*/

struct mir_mem_node_dist_t* mir_mem_node_dist_create()
{ /*{{{*/
    struct mir_mem_node_dist_t* dist = mir_malloc_int(sizeof(struct mir_mem_node_dist_t));
    MIR_ASSERT(dist != NULL);

    dist->buf = mir_malloc_int(sizeof(size_t) * runtime->arch->num_nodes);
    MIR_ASSERT(dist->buf != NULL);
    for (uint16_t i = 0; i < runtime->arch->num_nodes; i++)
        dist->buf[i] = 0;

    return dist;
} /*}}}*/

void mir_mem_node_dist_destroy(struct mir_mem_node_dist_t* dist)
{ /*{{{*/
    MIR_ASSERT(dist->buf != NULL);
    mir_free_int(dist->buf, sizeof(size_t) * runtime->arch->num_nodes);
    MIR_ASSERT(dist != NULL);
    mir_free_int(dist, sizeof(struct mir_mem_node_dist_t));
} /*}}}*/

unsigned long mir_mem_node_dist_get_comm_cost(const struct mir_mem_node_dist_t* dist, uint16_t from_node)
{ /*{{{*/
    MIR_ASSERT(dist != NULL);
    MIR_ASSERT(dist->buf != NULL);
    MIR_ASSERT(runtime->arch->num_nodes > from_node);

    unsigned long comm_cost = 0;

    for (int i = 0; i < runtime->arch->num_nodes; i++) {
        //MIR_DEBUG(MIR_DEBUG_STR "Comm cost node %d to %d: %d\n", from_node, i, runtime->arch->comm_cost_of(from_node, i));
        comm_cost += (dist->buf[i] * runtime->arch->comm_cost_of(from_node, i));
    }

    return comm_cost;
} /*}}}*/

void mir_mem_node_dist_get_stat(struct mir_mem_node_dist_stat_t* stat, const struct mir_mem_node_dist_t* dist)
{ /*{{{*/
    MIR_ASSERT(dist != NULL);
    MIR_ASSERT(dist->buf != NULL);
    MIR_ASSERT(stat != NULL);

    stat->sum = 0;
    stat->min = -1;
    stat->max = 0;
    for (int i = 0; i < runtime->arch->num_nodes; i++) {
        size_t val = dist->buf[i];
        stat->sum += val;
        if (stat->min > val)
            stat->min = val;
        if (stat->max < val)
            stat->max = val;
    }
    stat->mean = stat->sum / (runtime->arch->num_nodes);
    double sum_dsq = 0;
    for (int i = 0; i < runtime->arch->num_nodes; i++) {
        double diff = ((double)(dist->buf[i]) - stat->mean);
        sum_dsq = (diff * diff);
    }
    stat->sd = sqrt(sum_dsq / (runtime->arch->num_nodes));
} /*}}}*/

static void mem_node_dist_print(struct mir_mem_node_dist_t* dist)
{ /*{{{*/
    MIR_ASSERT(dist != NULL);
    MIR_ASSERT(dist->buf != NULL);
    MIR_INFORM("Dist: ");
    for (int i = 0; i < runtime->arch->num_nodes; i++)
        MIR_INFORM("%lu ", dist->buf[i]);
    MIR_INFORM("\n");
} /*}}}*/

void mir_mem_get_mem_node_dist(struct mir_mem_node_dist_t* dist, void* addr, size_t sz, void* part_of)
{ /*{{{*/
    MIR_ASSERT(addr != NULL && sz != 0 && dist != NULL);

    // Check if the part_of address contains a header
    // If yes, then the ...
    // distribution is fully contained at the nodeid of the header. ...
    // Fill and return
    if (part_of) {
        struct mem_header_t* header = get_mem_header(part_of);
        if (header) {
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
    if (header) {
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

    if (sz <= page_size)
        dist->buf[node] += sz;

    uint16_t new_node = runtime->arch->num_nodes + 1;
    size_t ifrom = 0;
    size_t i;
    for (i = page_size; i < sz; i += page_size) {
        new_node = get_node_of(addr + i, part_of);
        if (new_node != node) {
            dist->buf[node] += (i - ifrom);
            node = new_node;
            ifrom = i;
        }
    }

    if (new_node == node)
        dist->buf[node] += (sz - ifrom);

//print_dist(dist);
#else
    // FIXME: What happens on TILEPRO64?
    MIR_ASSERT(1 == 1);
#endif
} /*}}}*/

struct mir_mem_pol_t { /*{{{*/
    struct mir_lock_t lock;
    uint16_t node;
#ifdef MIR_MEM_POL_RESTRICT
    uint16_t num_nodes;
    uint32_t* nodes;
#endif
    uint64_t total_allocated;

    // Interfaces
    void* (*allocate)(size_t sz);
    void (*release)(void* addr, size_t sz);
    void (*reset)();
}; /*}}}*/

static struct mir_mem_pol_t* mem_pol;

static inline void reset_node()
{ /*{{{*/
    mir_lock_set(&mem_pol->lock);
    mem_pol->node = 0;
    mir_lock_unset(&mem_pol->lock);
} /*}}}*/

static inline void advance_node()
{ /*{{{*/
    mir_lock_set(&mem_pol->lock);
advance:
    mem_pol->node++;
    mem_pol->node = mem_pol->node % runtime->arch->num_nodes;
#ifdef MIR_MEM_POL_RESTRICT
    int allowed = 0;
    for (int i = 0; i < runtime->num_workers; i++)
        if (runtime->arch->node_of(runtime->workers[i].cpu_id) == mem_pol->node) {
            allowed = 1;
            break;
        }
    if (!allowed)
        goto advance;
#endif
    mir_lock_unset(&mem_pol->lock);
} /*}}}*/

static inline void fault_all_pages(void* addr, size_t sz)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    uint16_t pagesz = sysconf(_SC_PAGESIZE);
    MIR_ASSERT(pagesz != 0);
    size_t num_pages = (sz + pagesz - 1) / pagesz;
    for (size_t i = 0; i < num_pages; i++)
        *((char*)(addr) + (i * pagesz)) = 0;
} /*}}}*/

static void* allocate_coarse(size_t sz)
{ /*{{{*/
    MIR_ASSERT(sz > 0);

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    void* addr = NULL;
    size_t new_sz = sz + sizeof(struct mem_header_t);

#ifndef __tile__
    numa_set_bind_policy(1);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    MIR_ASSERT(mask != NULL);
    numa_bitmask_clearall(mask);
    numa_bitmask_setbit(mask, mem_pol->node);
    numa_set_membind(mask);
    addr = numa_alloc_onnode(new_sz, mem_pol->node);
    MIR_ASSERT(addr != NULL);
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
    page_attr_set(&home);
    addr = tmc_alloc_map(&home, new_sz);
    MIR_ASSERT(addr != NULL);
#endif

    // Write header
    struct mem_header_t* header = (struct mem_header_t*)addr;
    header->magic = runtime->init_time;
    header->sz = sz;
    header->nodeid = mem_pol->node;
    header->node_cache = NULL;

    // Book-keeping
    advance_node();
    __sync_fetch_and_add(&mem_pol->total_allocated, new_sz);

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*)((unsigned char*)addr + sizeof(struct mem_header_t));
} /*}}}*/

static void* allocate_fine(size_t sz)
{ /*{{{*/
    MIR_ASSERT(sz > 0);

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    void* addr = NULL;
    size_t new_sz = sz + sizeof(struct mem_header_t);
    int faulted_in = 0;

#ifndef __tile__
    int cur_pol;
    get_mempolicy(&cur_pol, NULL, 0, 0, 0);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    MIR_ASSERT(mask != NULL);
#ifdef MIR_MEM_POL_RESTRICT
    numa_bitmask_clearall(mask);
    for (int i = 0; i < runtime->num_workers; i++) {
        //MIR_DEBUG(MIR_DEBUG_STR "Selecting node %d for allocation\n", runtime->arch->node_of(runtime->workers[i].cpu_id));
        numa_bitmask_setbit(mask, runtime->arch->node_of(runtime->workers[i].cpu_id));
    }
#else
    numa_bitmask_setall(mask);
#endif
    numa_set_interleave_mask(mask);
    addr = numa_alloc_interleaved_subset(new_sz, mask);
    MIR_ASSERT(addr != NULL);
    set_mempolicy(cur_pol, mask->maskp, mask->size + 1);
    numa_bitmask_free(mask);
#ifdef MIR_MEM_POL_LOCK_PAGES
    mlock(addr, new_sz);
#endif
#ifdef MIR_MEM_POL_FAULT_IN_PAGES
    // Fault it in
    //memset(addr, 0, new_sz);
    fault_all_pages(addr, new_sz);
    faulted_in = 1;
#endif
#else
    tmc_alloc_t home = TMC_ALLOC_INIT;
    tmc_alloc_set_home(&home, TMC_ALLOC_HOME_HASH);
    page_attr_set(&home);
    addr = tmc_alloc_map(&home, new_sz);
    MIR_ASSERT(addr != NULL);
#endif

    // Write header
    struct mem_header_t* header = (struct mem_header_t*)addr;
    header->magic = 0;
    header->sz = sz;
    header->nodeid = runtime->arch->num_nodes + 1;
#ifdef MIR_MEM_POL_CACHE_NODES
    // Have to fault in the pages!
    // Or else the query will not work
    if (faulted_in == 0) {
        //memset(addr, 0, new_sz);
        fault_all_pages(addr, new_sz);
        faulted_in = 1;
    }

    // Create node cache
    uint16_t pagesz = sysconf(_SC_PAGESIZE);
    size_t num_pages = (new_sz + pagesz - 1) / pagesz;
    header->node_cache = mir_malloc_int(sizeof(uint16_t) * num_pages);
    MIR_ASSERT(header->node_cache != NULL);
    for (size_t i = 0; i < num_pages; i++) {
        uint16_t node = get_node_of(addr + (i * pagesz), NULL);
        header->node_cache[i] = node;
    }
/*MIR_INFORM("Node cache: ");*/
/*for(size_t i=0; i<num_pages; i++)*/
/*MIR_INFORM("%lu->%d ", i, header->node_cache[i]);*/
/*MIR_INFORM("\n");*/
#else
    header->node_cache = NULL;
#endif

    __sync_fetch_and_add(&mem_pol->total_allocated, new_sz);

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*)((unsigned char*)addr + sizeof(struct mem_header_t));
} /*}}}*/

static void* allocate_system(size_t sz)
{ /*{{{*/
    MIR_ASSERT(sz > 0);

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    size_t new_sz = sz + sizeof(struct mem_header_t);
    void* addr = malloc(new_sz);
    MIR_ASSERT(addr != NULL);
    // Write header
    struct mem_header_t* header = (struct mem_header_t*)addr;
    header->magic = 0;
    header->sz = sz;
    header->nodeid = runtime->arch->num_nodes + 1;
    header->node_cache = NULL;

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*)((unsigned char*)addr + sizeof(struct mem_header_t));
} /*}}}*/

static void* allocate_local(size_t sz)
{ /*{{{*/
    MIR_ASSERT(sz > 0);

    MIR_RECORDER_STATE_BEGIN(MIR_STATE_TMALLOC);

    void* addr = NULL;
    size_t new_sz = sz + sizeof(struct mem_header_t);

    // Get this worker's node
    struct mir_worker_t* worker = mir_worker_get_context();
    int node = runtime->arch->node_of(worker->cpu_id);

#ifndef __tile__
    numa_set_bind_policy(1);
    struct bitmask* mask = numa_bitmask_alloc(runtime->arch->num_nodes);
    MIR_ASSERT(mask != NULL);
    numa_bitmask_clearall(mask);
    numa_bitmask_setbit(mask, node);
    numa_set_membind(mask);
    addr = numa_alloc_onnode(new_sz, node);
    MIR_ASSERT(addr != NULL);
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
    page_attr_set(&home);
    addr = tmc_alloc_map(&home, new_sz);
    MIR_ASSERT(addr != NULL);
#endif

    // Write header
    struct mem_header_t* header = (struct mem_header_t*)addr;
    header->magic = runtime->init_time;
    header->sz = sz;
    header->nodeid = node;
    header->node_cache = NULL;

    __sync_fetch_and_add(&mem_pol->total_allocated, new_sz);

    MIR_RECORDER_STATE_END(NULL, 0);

    return (void*)((unsigned char*)addr + sizeof(struct mem_header_t));
} /*}}}*/

static void release_coarse(void* addr, size_t sz)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    MIR_ASSERT(sz > 0);

    size_t new_sz = sz + sizeof(struct mem_header_t);
    void* new_addr = (void*)((char*)addr - sizeof(struct mem_header_t));
#ifdef MIR_MEM_POL_CACHE_NODES
    // FIXME: This check kicks in when the fine policy is used. Fugly code!
    // Release node cache
    struct mem_header_t* header = (struct mem_header_t*)(new_addr);
    if (header->node_cache != NULL) {
        uint16_t pagesz = sysconf(_SC_PAGESIZE);
        size_t num_pages = (new_sz + pagesz - 1) / pagesz;
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

    __sync_fetch_and_sub(&mem_pol->total_allocated, new_sz);
} /*}}}*/

static void release_fine(void* addr, size_t sz)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    MIR_ASSERT(sz > 0);
    release_coarse(addr, sz);
} /*}}}*/

static void release_system(void* addr, size_t sz)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    MIR_ASSERT(sz > 0);
    void* new_addr = (void*)((char*)addr - sizeof(struct mem_header_t));
    free(new_addr);
} /*}}}*/

static void release_local(void* addr, size_t sz)
{ /*{{{*/
    MIR_ASSERT(addr != NULL);
    MIR_ASSERT(sz > 0);
    release_coarse(addr, sz);
} /*}}}*/

static void reset_common()
{ /*{{{*/
} /*}}}*/

static void reset_coarse()
{ /*{{{*/
    reset_node();
} /*}}}*/

void mir_mem_pol_config(const char* pol_name)
{ /*{{{*/
    MIR_ASSERT(pol_name != NULL);
    MIR_ASSERT(strlen(pol_name) > 0);

    if (0 == strcmp(pol_name, "fine")) {
        mem_pol->allocate = allocate_fine;
        mem_pol->release = release_fine;
    }
    else if (0 == strcmp(pol_name, "coarse")) {
        mem_pol->allocate = allocate_coarse;
        mem_pol->release = release_coarse;
        mem_pol->reset = reset_coarse;
    }
    else if (0 == strcmp(pol_name, "system")) {
        mem_pol->allocate = allocate_system;
        mem_pol->release = release_system;
    }
    else if (0 == strcmp(pol_name, "local")) {
        mem_pol->allocate = allocate_local;
        mem_pol->release = release_local;
    }
    else {
        MIR_ABORT(MIR_ERROR_STR "Invalid MIR_CONF memory allocation policy %s!\n", pol_name);
    }

    MIR_DEBUG(MIR_DEBUG_STR "Memory allocation policy changed to %s\n", pol_name);
} /*}}}*/

void mir_mem_pol_create()
{ /*{{{*/
    // Allocate mem_pol structure
    mem_pol = mir_malloc_int(sizeof(struct mir_mem_pol_t));
    MIR_ASSERT(mem_pol != NULL);
    // Statistics
    mem_pol->total_allocated = 0;
    // Node for coarse allocation
    // This will be properly set during mir_mem_pol_init
    mem_pol->node = 0;
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
} /*}}}*/

void mir_mem_pol_init()
{ /*{{{*/
#ifdef MIR_MEM_POL_RESTRICT
    mem_pol->node = runtime->arch->node_of(runtime->workers[0].cpu_id);
#endif
} /*}}}*/

void mir_mem_pol_destroy()
{ /*{{{*/
    MIR_DEBUG(MIR_DEBUG_STR "Stopping memory distributer ...\n");
    MIR_DEBUG(MIR_DEBUG_STR "Total unfreed mem_pol memory=%" MIR_FORMSPEC_UL " bytes\n", mem_pol->total_allocated);
    mir_lock_destroy(&mem_pol->lock);
    mir_free_int(mem_pol, sizeof(struct mir_mem_pol_t));
} /*}}}*/

void* mir_mem_pol_allocate(size_t sz)
{ /*{{{*/
    return mem_pol->allocate(sz);
    /*void* block =  mem_pol->allocate(sz);*/
    /*struct mir_mem_node_dist_t* dist = mir_mem_node_dist_create();*/
    /*mir_mem_node_dist_destroy(dist);*/
    /*mir_mem_get_dist(dist, block, sz, NULL);*/
    /*print_dist(dist);*/
    /*return block;*/
} /*}}}*/

void mir_mem_pol_release(void* addr, size_t sz)
{ /*{{{*/
    mem_pol->release(addr, sz);
} /*}}}*/

void mir_mem_pol_reset()
{ /*{{{*/
    mem_pol->reset();
} /*}}}*/
#endif
