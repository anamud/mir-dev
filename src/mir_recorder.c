#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mir_types.h"
#include "mir_defines.h"
#include "mir_perf.h"
#include "mir_debug.h"
#include "mir_memory.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "mir_worker.h"

extern struct mir_runtime_t* runtime;

#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS

#ifdef __tile__

#include <arch/cycle.h>

#define SPR_PERF_COUNT_CTL  0x4207
#define SPR_AUX_PERF_COUNT_CTL  0x6007
#define SPR_PERF_COUNT_0 0x4205
#define SPR_PERF_COUNT_1 0x4206
#define SPR_AUX_PERF_COUNT_0 0x6005
#define SPR_AUX_PERF_COUNT_1 0x6006
#define LOCAL_DRD_CNT 0x28
#define REMOTE_DRD_CNT 0x2b 
#define LOCAL_DRD_MISS 0x34
#define REMOTE_DRD_MISS 0x37
#define DATA_CACHE_STALL 0x10
#define INSTRUCTION_CACHE_STALL 0x11
#define LOAD_MISS_REPLAY_TRAP 0x1c
#define BUNDLES_RETIRED 0x6
#define VIC_WB_CNT 0x3b
#define COUNTER_ONE 0x1
#define LR_COUNTERS 1
#define COUNTER_CONF LR_COUNTERS

// FIXME: WARNING AND NOTE! Update also in recorder_create
char* mir_event_name_string[MIR_RECORDER_EVENT_MAX_COUNT]  = 
{ /*{{{*/
     "LOCAL_DRD_MISS", 
     "REMOTE_DRD_MISS", 
     "DATA_CACHE_STALL", //"LOAD_MISS_REPLAY_TRAP",
     "LOAD_MISS_REPLAY_TRAP" //"BUNDLES_RETIRED" 
 };/*}}}*/

#else

#include "papi.h"

// FIXME: WARNING AND NOTE! Update also in recorder_create
char* mir_event_name_string[MIR_RECORDER_EVENT_MAX_COUNT] = 
{ /*{{{*/
    "PAPI_L2_DCM", 
    "PAPI_RES_STL" 
    /*"PAPI_L1_DCA",*/
    /*"PAPI_L1_DCH",*/
};/*}}}*/

#endif

#else

// FIXME: WARNING AND NOTE! Update also in recorder_create
char* mir_event_name_string[MIR_RECORDER_EVENT_MAX_COUNT] = 
{ /*{{{*/
    "NO_EVENT", 
};/*}}}*/

#endif

const char* mir_state_name_string[MIR_RECORDER_STATE_MAX_COUNT] = 
{/*{{{*/
    "TIDLE",
    "TCREATE",
    "TSCHED",
    "TEXEC",
    "TSYNC",
    "TPOP",
    "TSTEAL",
    "TMALLOC"
};/*}}}*/

struct mir_recorder_t* mir_recorder_create(uint16_t id)
{/*{{{*/
    struct mir_recorder_t* recorder = (struct mir_recorder_t*) mir_malloc_int (sizeof(struct mir_recorder_t));
    if(recorder == NULL)
        MIR_ABORT(MIR_ERROR_STR "Could not create recorder!\n");

    // Open buffer file
    char buffer_file_name[MIR_LONG_NAME_LEN];
    sprintf(buffer_file_name, "%" MIR_FORMSPEC_UL "-recorder-%d.rec", runtime->init_time, id);
    recorder->buffer_file = fopen(buffer_file_name, "w");
    if(!recorder->buffer_file)
        MIR_ABORT(MIR_ERROR_STR "Could not create recorder buffer file!\n");

    // Record id
    recorder->id = id;
    fprintf(recorder->buffer_file, "%%recorder_id=%d%%\n", id);

    // Record states and events
    for(int i=0; i<MIR_RECORDER_STATE_MAX_COUNT; i++)
        fprintf(recorder->buffer_file, "%s:", mir_state_name_string[i]);
    fprintf(recorder->buffer_file, "\n");
    for(int i=0; i<MIR_RECORDER_EVENT_MAX_COUNT; i++)
        fprintf(recorder->buffer_file, "%s:", mir_event_name_string[i]);
    fprintf(recorder->buffer_file, "\n");

    // Reset buffer heads
    recorder->state_buffer_head = 0;
    recorder->state_stack_head = 0;
    recorder->event_buffer_head = 0;

    // Reset state counter
    recorder->num_states = 0;

    // Reset event counter
    recorder->num_events = 0;

#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS

#ifdef __tile__

    // Settilera counters
    recorder->tilecounter[0] = LOCAL_DRD_MISS;
    recorder->tilecounter[1] = REMOTE_DRD_MISS;
    recorder->tilecounter[2] = DATA_CACHE_STALL; // LOAD_MISS_REPLAY_TRAP;
    recorder->tilecounter[3] = LOAD_MISS_REPLAY_TRAP; // BUNDLES_RETIRED;

    // Enable tilera counters
    __insn_mtspr(SPR_PERF_COUNT_CTL, recorder->tilecounter[0] | (recorder->tilecounter[1] << 16));
    __insn_mtspr(SPR_AUX_PERF_COUNT_CTL, recorder->tilecounter[2] | (recorder->tilecounter[3] << 16));

#else

    // Register thread with PAPI
    //MIR_INFORM(MIR_INFORM_STR "Registering thread %d with PAPI ... \n", id);
    int retval = PAPI_register_thread();
    if ( retval != PAPI_OK )
        MIR_ABORT(MIR_ERROR_STR "PAPI_register_thread %d failed [%d]!\n", id, retval);

    // Create the eventset
    recorder->EventSet = PAPI_NULL;
    if ( (retval = PAPI_create_eventset(&recorder->EventSet)) != PAPI_OK)
        MIR_ABORT(MIR_ERROR_STR "PAPI_create_eventset failed!\n");

    // Add events to the eventset
    for(int i = 0; i < MIR_RECORDER_EVENT_MAX_COUNT; i++)
    {
        int event_code;
        //MIR_INFORM(MIR_INFORM_STR "Recorder %d getting event code for event %s ... \n", id, mir_event_name_string[i]);
        if ( (retval = PAPI_event_name_to_code(mir_event_name_string[i], &event_code)) != PAPI_OK)
            MIR_ABORT(MIR_ERROR_STR "Recorder %d PAPI_event name_to_code %s failed [%d]!\n", id, mir_event_name_string[i], retval);
        //MIR_INFORM(MIR_INFORM_STR "Recorder %d adding event with code event %x ... \n", id, event_code);
        if ( (retval = PAPI_add_event(recorder->EventSet, event_code)) != PAPI_OK)
            MIR_ABORT(MIR_ERROR_STR "Recorder %d PAPI_add_event %x failed [%d]!\n", id, event_code, retval);
    }

    // Start counting 
    PAPI_start(recorder->EventSet);
#endif

#endif

    // Pass it on!
    return recorder;
}/*}}}*/

void mir_recorder_destroy(struct mir_recorder_t* recorder)
{/*{{{*/
#if defined(MIR_RECORDER_USE_HW_PERF_COUNTERS)
#ifndef __tile__
    PAPI_unregister_thread();
#endif
#endif

    // Id 0 is the last one to go down
    if(recorder->id == 0)
    {
        // Make config file name
        char config_filename[MIR_LONG_NAME_LEN];
        sprintf(config_filename, "%" MIR_FORMSPEC_UL "-config.rec", runtime->init_time);

        // Open config file
        FILE* config_file = fopen(config_filename, "w");
        if(!config_file)
            MIR_ABORT(MIR_ERROR_STR "Cannot open recorder config file for writing!\n");

        // Write to config file
        fprintf(config_file, "creation_cycle=%" MIR_FORMSPEC_UL "\ndestruction_cycle=%" MIR_FORMSPEC_UL "\nnum_workers=%d\n", runtime->init_time, mir_get_cycles(), runtime->num_workers);

        // Close PAPI
#if defined(MIR_RECORDER_USE_HW_PERF_COUNTERS)
#ifndef __tile__
            PAPI_shutdown();
#endif
#endif
}

    fclose(recorder->buffer_file);
    mir_free_int(recorder, sizeof(struct mir_recorder_t));
}/*}}}*/

void mir_recorder_write_to_file(struct mir_recorder_t* recorder)
{/*{{{*/
    // Dump states
    for(uint64_t i=0; i<recorder->state_buffer_head; i++)
    {
        struct mir_state_t* state = &recorder->state_buffer[i];
        fprintf(recorder->buffer_file, "s:%d:%" MIR_FORMSPEC_UL ":%" MIR_FORMSPEC_UL ":%" MIR_FORMSPEC_UL ":%s:%s\n", 
                state->id, 
                state->parent_id, 
                state->begin_time, 
                state->end_time,
                mir_state_name_string[(unsigned int)(state->name)], 
                state->meta_data); 
    }

    // Dump events
    for(uint64_t i=0; i<recorder->event_buffer_head; i++)
    {
        struct mir_event_t* event = &recorder->event_buffer[i];
        fprintf(recorder->buffer_file, "e:%d:%" MIR_FORMSPEC_UL ":%d:", 
                event->id, 
                event->event_time,
                MIR_RECORDER_EVENT_MAX_COUNT);
        
        for(int j=0; j<MIR_RECORDER_EVENT_MAX_COUNT; j++)
            fprintf(recorder->buffer_file, "%s=%lld,", mir_event_name_string[j], event->values[j]);

        fprintf(recorder->buffer_file, ":%s\n", 
                event->meta_data); 
    }

    // Rewind heads
    recorder->event_buffer_head = 0;
    recorder->state_buffer_head = 0;

}/*}}}*/

void MIR_RECORDER_STATE_BEGIN(mir_state_name_t name) 
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();

    // Get this worker's reocrder
    struct mir_recorder_t* r = worker->recorder;
    if(!r)
        return;

    // Increase state count
    r->num_states++;

    // Push to state stack
    struct mir_state_t* state = &r->state_stack[r->state_stack_head];
    state->name = name;
    state->begin_time = mir_get_cycles();
    state->id = r->num_states;
    if(r->state_stack_head > 0)
        state->parent_id = r->state_stack[r->state_stack_head -1].id; 
    else
        state->parent_id = 0;

    // Increment state_stack head
    r->state_stack_head++;
    MIR_ASSERT(r->state_stack_head < MIR_RECORDER_STACK_MAX_SIZE);
}/*}}}*/

void MIR_RECORDER_STATE_END(const char* meta_data, uint32_t meta_data_length) 
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();

    // Get this worker's reocrder
    struct mir_recorder_t* r = worker->recorder;
    if(!r)
        return;

    // Pop from state state_stack
    r->state_stack_head--;

    struct mir_state_t* state = &r->state_buffer[r->state_buffer_head];
    struct mir_state_t* ending_state = &r->state_stack[r->state_stack_head];

    // Copy from state state_stack to state_buffer
    state->end_time = mir_get_cycles();
    state->name = ending_state->name;
    state->begin_time = ending_state->begin_time;
    state->id = ending_state->id;
    state->parent_id = ending_state->parent_id;
    MIR_ASSERT(meta_data_length < MIR_RECORDER_STATE_META_DATA_MAX_SIZE);

    if(meta_data != NULL)
    {
        // Copy meta_data as null-terminated string
        memcpy(state->meta_data, meta_data, meta_data_length);
        state->meta_data[meta_data_length] = '\0';
    }
    else
    {
        state->meta_data[0] = '\0';
    }

    // Increament state_buffer head
    (r->state_buffer_head)++;
    
    // If buffer is near full, dump!
    //MIR_ASSERT(r->state_buffer_head < MIR_RECORDER_BUFFER_MAX_SIZE);
    if (r->state_buffer_head == (MIR_RECORDER_BUFFER_MAX_SIZE-1))
        mir_recorder_write_to_file(r);
}/*}}}*/

void MIR_RECORDER_EVENT(const char* meta_data, uint32_t meta_data_length)
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();

    // Get this worker's reocrder
    struct mir_recorder_t* r = worker->recorder;
    if(!r)
        return;

    // Increase event count
    r->num_events++;

    // Add to event_buffer
    struct mir_event_t* event = &r->event_buffer[r->event_buffer_head];
    event->event_time = mir_get_cycles();
    event->id = r->num_events;

#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifndef __tile__
    // Add event set values 
    PAPI_read(r->EventSet, event->values);
    PAPI_reset(r->EventSet);
    PAPI_start(r->EventSet);
#else
    // Read tilera counter values
    event->values[0] = __insn_mfspr(SPR_PERF_COUNT_0);
    event->values[1] = __insn_mfspr(SPR_PERF_COUNT_1);
    event->values[2] = __insn_mfspr(SPR_AUX_PERF_COUNT_0);
    event->values[3] = __insn_mfspr(SPR_AUX_PERF_COUNT_1);

    // Clear counters
    __insn_mtspr(SPR_PERF_COUNT_0, 0);
    __insn_mtspr(SPR_PERF_COUNT_1, 0);
    __insn_mtspr(SPR_AUX_PERF_COUNT_0, 0);
    __insn_mtspr(SPR_AUX_PERF_COUNT_1, 0);

    // Setup counters on tilera again
    __insn_mtspr(SPR_PERF_COUNT_CTL, r->tilecounter[0] | (r->tilecounter[1] << 16));
    __insn_mtspr(SPR_AUX_PERF_COUNT_CTL, r->tilecounter[2] | (r->tilecounter[3] << 16));
#endif
#else
    // FIXME: WARNING AND NOTE! 0 == MIR_RECORDER_EVENT_MAX_COUNT
    event->values[0] = 0;
#endif

    // Copy event meta data
    if(meta_data != NULL)
    {
        MIR_ASSERT(meta_data_length < MIR_RECORDER_EVENT_META_DATA_MAX_SIZE);
        // Copy meta_data as null-terminated string
        memcpy(event->meta_data, meta_data, meta_data_length);
        event->meta_data[meta_data_length] = '\0';
    }
    else
    {
        event->meta_data[0] = '\0';
    }

    // Increment event_buffer head
    (r->event_buffer_head)++;

    // If buffer is near full, dump!
    //MIR_ASSERT(r->event_buffer_head < MIR_RECORDER_BUFFER_MAX_SIZE);
    if (r->event_buffer_head == (MIR_RECORDER_BUFFER_MAX_SIZE-1))
        mir_recorder_write_to_file(r);
}/*}}}*/
