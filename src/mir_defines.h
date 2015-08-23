#ifndef MIR_DEFINES_H
#define MIR_DEFINES_H 1

// Workers
#define MIR_WORKER_MAX_COUNT 100
#define MIR_WORKER_STACK_SIZE_MULTIPLIER 10
#define MIR_WORKER_EXP_BOFF_RESET 1
#define MIR_WORKER_EXP_BOFF_SCALE 2
#define MIR_WORKER_EXP_BOFF_ROOF 128
// Set to 0 for no backoff
#define MIR_WORKER_BACKOFF_DURING_SYNC 1
#define MIR_WORKER_BACKOFF_DURING_BARRIER_WAIT 1
#define MIR_WORKER_EXPLICIT_BIND

// Task
//#define MIR_TASK_DEBUG
#define MIR_TASK_DEFAULT_NAME "NO_NAME"
#define MIR_IDLE_TASK_NAME "idle_task"
//#define MIR_TASK_ALLOCATE_ON_STACK
#define MIR_TASK_ID_START 0
#define MIR_TASKWAIT_ID_START 0
#define MIR_TASK_FIXED_DATA_SIZE
// Uncomment below define statement only if MIR_TASK_FIXED_DATA_SIZE is also defined
#define MIR_TASK_DATA_MAX_SIZE 256

// Queue
//#define MIR_QUEUE_DEBUG
//#define MIR_QUEUE_MAX_CAPACITY 8192
#define MIR_QUEUE_MAX_CAPACITY 32768
//#define MIR_QUEUE_MAX_CAPACITY 65536

// Types
#define MIR_SHORT_NAME_LEN 32
#define MIR_LONG_NAME_LEN 256
#define MIR_SBUF_SIZE 64

// Statistics
#define MIR_WORKER_STATS_FILE_NAME "mir-worker-stats"
#define MIR_TASK_STATS_FILE_NAME "mir-task-stats"

// Memory allocator
#define MIR_MEMORY_ALLOCATOR_DEBUG
#define MIR_PAGE_ALIGNMENT 64

// Recorder
#define MIR_RECORDER_BUFFER_MAX_SIZE (1 * 1024 * 256)
#define MIR_RECORDER_STACK_MAX_SIZE MIR_RECORDER_BUFFER_MAX_SIZE
#define MIR_RECORDER_STATE_META_DATA_MAX_SIZE MIR_SBUF_SIZE
#define MIR_RECORDER_EVENT_META_DATA_MAX_SIZE MIR_SBUF_SIZE
#define MIR_RECORDER_FILE_NAME_PREFIX "mir-recorder"

// Scheduling policy
#define MIR_SCHED_POL_DEFAULT "central-stack"

// Dynamic inlining
#define MIR_INLINE_TASK_IF_QUEUE_FULL
// Creation inline: 0 = never, 1 = always, >1 = inlined if num tasks waiting per worker exceeds
#define MIR_INLINE_TASK_DURING_CREATION 0

// Memory allocation policy
#define MIR_MEM_POL_CACHE_NODES
#define MIR_MEM_POL_RESTRICT
//#define MIR_MEM_POL_FAULT_IN_PAGES
//#define MIR_MEM_POL_LOCK_PAGES
// TILEPro64 specifc
//#define MIR_PAGE_NO_LOCAL_CACHING
//#define MIR_PAGE_NO_L1_CACHING
//#define MIR_PAGE_NO_L2_CACHING

// Shared memory
#define MIR_OFP_SHM_SIZE 16
// DO NOT EDIT
#define MIR_OFP_SHM_KEY 31415926
// DO NOT EDIT
#define MIR_OFP_SHM_SIGREAD '*'

// Context tracking
#define MIR_CONTEXT_ENTER do{ __asm volatile ("mov %%bx, %%bx" ::: ); }while(0)
#define MIR_CONTEXT_EXIT do{ __asm volatile ("mov %%cx, %%cx" ::: ); }while(0)
#define MIR_CONTEXT_EXIT_VAL(val) do{ __asm volatile ("mov %%cx, %%cx" ::: ); return (val); }while(0)

// Architecture
// DO NOT EDIT
#define MIR_IMPOSSIBLE_CPU_ID 299792458
#define MIR_ALWAYS_AVAILABLE_CPU_ID 0

#endif
