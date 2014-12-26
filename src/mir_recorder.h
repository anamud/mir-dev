#ifndef MIR_RECORDER_H
#define MIR_RECORDER_H 1

#include <stdint.h>
#include <stdlib.h>
#include "mir_types.h"

BEGIN_C_DECLS 

// FIXME: WARNING AND NOTE! Update both state_name and state_name_string
#define MIR_RECORDER_NUM_STATES 8
enum mir_state_name_t
{/*{{{*/
    MIR_STATE_TIDLE = 0,
    MIR_STATE_TCREATE = 1,
    MIR_STATE_TSCHED = 2,
    MIR_STATE_TEXEC = 3,
    MIR_STATE_TSYNC = 4,
    MIR_STATE_TPOP = 5,
    MIR_STATE_TSTEAL = 6,
    MIR_STATE_TMALLOC = 7 //Ensure this is MIR_RECORDER_NUM_STATES - 1
};/*}}}*/
typedef enum mir_state_name_t mir_state_name_t;

struct mir_state_t
{/*{{{*/
    mir_state_name_t name;
    uint64_t begin_time;
    uint64_t end_time;
    uint16_t id;
    uint64_t parent_id;
    char meta_data[MIR_RECORDER_STATE_META_DATA_MAX_SIZE];
};/*}}}*/

#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifndef __tile__ 
#define MIR_RECORDER_NUM_PAPI_HWPC 2
#endif
#endif

struct mir_event_t
{/*{{{*/
    uint64_t event_time;
    uint16_t id;
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifdef __tile__ 
    long long values[4];
#else
    long long values[MIR_RECORDER_NUM_PAPI_HWPC];
#endif
#else
    long long values[1];
#endif
    char meta_data[MIR_RECORDER_EVENT_META_DATA_MAX_SIZE];
};/*}}}*/

struct mir_recorder_t 
{/*{{{*/
    uint64_t num_states;
    uint64_t num_events;
    uint16_t id;
    FILE* buffer_file;
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifdef __tile__
    // Tilera HWPC
    int tilecounter[4];
#else
    // PAPI HWPC
    int EventSet;
#endif
#endif

    // For recording time spent in states
    uint64_t state_time[MIR_RECORDER_NUM_STATES];
    uint64_t this_state_trans_time;
    mir_state_name_t prev_state, this_state;

    // For recording states, I use a stack and a buffer
    struct mir_state_t state_buffer[MIR_RECORDER_BUFFER_MAX_SIZE];
    uint64_t state_buffer_head;
    struct mir_state_t state_stack[MIR_RECORDER_STACK_MAX_SIZE];
    uint64_t state_stack_head;

    // For recording events, I use a buffer
    struct mir_event_t event_buffer[MIR_RECORDER_BUFFER_MAX_SIZE];
    uint64_t event_buffer_head;
};/*}}}*/

struct mir_recorder_t* mir_recorder_create(uint16_t id);

void mir_recorder_destroy(struct mir_recorder_t* recorder);

void mir_recorder_write_to_file(struct mir_recorder_t* recorder);

void mir_recorder_record_state_transition(struct mir_recorder_t* recorder, mir_state_name_t next_state);

void MIR_RECORDER_STATE_BEGIN(mir_state_name_t name);

void MIR_RECORDER_STATE_END(const char* meta_data, uint32_t meta_data_length);

void MIR_RECORDER_EVENT(const char* meta_data, uint32_t meta_data_length);

END_C_DECLS 
#endif 
