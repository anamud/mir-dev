#ifndef MIR_DEFINES_H
#define MIR_DEFINES_H 1

// Workers
#define MIR_WORKER_MAX_COUNT 100
#define MIR_WORKER_STACK_SIZE_MULTIPLIER 10

// Task
//#define MIR_TASK_DEBUG 1
#define MIR_TASK_ID_START 0
//#define MIR_TASK_ALLOCATE_ON_STACK 1

// Queue
//#define MIR_QUEUE_DEBUG 1
#define MIR_QUEUE_MAX_CAPACITY 4096
#define MIR_QUEUE_ACT_AS_STACK 0

// Types
#define MIR_SHORT_NAME_LEN 32
#define MIR_LONG_NAME_LEN 256
#define MIR_SBUF_SIZE 32
#define MIR_BA_SBUF_SIZE 256

// Misc
#ifdef __tile__
#define MIR_FORMSPEC_UL "llu"
#else
#define MIR_FORMSPEC_UL "lu"
#endif

// Statistics
#define MIR_STATS_FILE_NAME "mir-stats"

// Debug
#define MIR_ERROR_STR "MIR_FATAL: "
#define MIR_DEBUG_STR "MIR_DBG: "
#define MIR_INFORM_STR "MIR_INFO: "

// Memory allocator
#define MIR_MEMORY_ALLOCATOR_DEBUG 1
#define MIR_PAGE_ALIGNMENT 64

// Recorder
//#define MIR_RECORDER_USE_HW_PERF_COUNTERS 1
#define MIR_RECORDER_BUFFER_MAX_SIZE (1*1024*16)
#define MIR_RECORDER_STACK_MAX_SIZE MIR_RECORDER_BUFFER_MAX_SIZE
#define MIR_RECORDER_STATE_META_DATA_MAX_SIZE 32
#define MIR_RECORDER_EVENT_META_DATA_MAX_SIZE 32

// Scheduling policy
#define MIR_SCHED_POL_DEFAULT "central"
#define MIR_SCHED_POL_INLINE_TASKS 1

// Memory allocation policy
//#define MIR_MEM_POL_LOCK_PAGES 1
// TILEPro64 specifc
//#define MIR_PAGE_NO_LOCAL_CACHING 1
//#define MIR_PAGE_NO_L1_CACHING
//#define MIR_PAGE_NO_L2_CACHING 1

#endif
