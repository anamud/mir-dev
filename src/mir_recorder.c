#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "mir_types.h"
#include "mir_defines.h"
#include "mir_utils.h"
#include "mir_memory.h"
#include "mir_recorder.h"
#include "mir_runtime.h"
#include "mir_worker.h"

const char* mir_state_name_string[MIR_RECORDER_NUM_STATES] = 
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

#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS

struct _perf_ctr_map 
{ 
    char* name; 
    int code; 
};

#ifdef __tile__

#define SPR_PERF_COUNT_CTL  0x4207
#define SPR_AUX_PERF_COUNT_CTL  0x6007
#define SPR_PERF_COUNT_0 0x4205
#define SPR_PERF_COUNT_1 0x4206
#define SPR_AUX_PERF_COUNT_0 0x6005
#define SPR_AUX_PERF_COUNT_1 0x6006
// Count=4 is hard-coded. Do not change count.
struct _perf_ctr_map perf_ctr_map[4] = {
    {"LOCAL_DRD_CNT", 0x28}
    {"REMOTE_DRD_CNT", 0x2b},
    {"LOCAL_DRD_MISS", 0x34},
    {"REMOTE_DRD_MISS", 0x37},
    /*{"DATA_CACHE_STALL", 0x10},*/
    /*{"INSTRUCTION_CACHE_STALL", 0x11},*/
    /*{"LOAD_MISS_REPLAY_TRAP", 0x1c},*/
    /*{"BUNDLES_RETIRED", 0x6},*/
    /*{"VIC_WB_CNT", 0x3b},*/
    /*{"COUNTER_ONE", 0x1},*/
};

#else

// Ignore the codes 0x0
// Add as many perf counters as MIR_RECORDER_NUM_PAPI_HWPC 
struct _perf_ctr_map perf_ctr_map[MIR_RECORDER_NUM_PAPI_HWPC] = {
    /*{"PAPI_TOT_INS", 0x0},*/
    /*{"PAPI_TOT_CYC", 0x0},*/
    /*{"PAPI_LD_INS", 0x0},*/
    {"PAPI_TLB_DM", 0x0},
    {"PAPI_RES_STL", 0x0},
    /*{"PAPI_L1_DCA", 0x0},*/
    /*{"PAPI_L1_DCM", 0x0},*/
    /*{"PAPI_L2_DCA", 0x0},*/
    /*{"PAPI_L2_DCM", 0x0},*/
};

#endif

#endif

struct mir_recorder_t* mir_recorder_create(uint16_t id)
{/*{{{*/
    struct mir_recorder_t* recorder = mir_malloc_int (sizeof(struct mir_recorder_t));
    MIR_ASSERT(recorder != NULL);

    // Open buffer file
    char buffer_file_name[MIR_LONG_NAME_LEN];
    sprintf(buffer_file_name, "%s-trace-%d.rec", MIR_RECORDER_FILE_NAME_PREFIX, id);
    recorder->buffer_file = fopen(buffer_file_name, "w");
    MIR_ASSERT(recorder->buffer_file != NULL);

    // Record id
    recorder->id = id;
    fprintf(recorder->buffer_file, "%%recorder_id=%d%%\n", id);

    // Reset state time related
    for(int i=0; i<MIR_RECORDER_NUM_STATES; i++)
        recorder->state_time[i] = 0;
    recorder->prev_state = 0;
    recorder->this_state = 0;
    recorder->this_state_trans_time = mir_get_cycles();

    // Record states and events
    for(int i=0; i<MIR_RECORDER_NUM_STATES; i++)
        fprintf(recorder->buffer_file, "%s:", mir_state_name_string[i]);
    fprintf(recorder->buffer_file, "\n");
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS
#ifdef __tile__
    for(int i=0; i<4; i++)
#else
    for(int i=0; i<MIR_RECORDER_NUM_PAPI_HWPC; i++)
#endif
        fprintf(recorder->buffer_file, "%s:", perf_ctr_map[i].name);
#else
    fprintf(recorder->buffer_file, "NA:");
#endif
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
    // Set tilera counters
    for(int i=0; i<4; i++)
        recorder->tilecounter[i] = perf_ctr_map_tilepro64[i].code;

    // Enable tilera counters
    __insn_mtspr(SPR_PERF_COUNT_CTL, recorder->tilecounter[0] | (recorder->tilecounter[1] << 16));
    __insn_mtspr(SPR_AUX_PERF_COUNT_CTL, recorder->tilecounter[2] | (recorder->tilecounter[3] << 16));
#else
    // Register thread with PAPI
    //MIR_INFORM(MIR_INFORM_STR "Registering thread %d with PAPI ... \n", id);
    int retval = PAPI_register_thread();
    MIR_ASSERT(retval == PAPI_OK);

    // Create the eventset
    recorder->EventSet = PAPI_NULL;
    retval = PAPI_create_eventset(&recorder->EventSet);
    MIR_ASSERT(retval == PAPI_OK);

    // Add events to the eventset
    for(int i = 0; i < MIR_RECORDER_NUM_PAPI_HWPC; i++)
    {
        int code;
        if((retval = PAPI_event_name_to_code(perf_ctr_map[i].name, &code)) != PAPI_OK)
            MIR_ABORT(MIR_ERROR_STR "Recorder %d PAPI_event_name_to_code %s failed [%d]!\n", id, perf_ctr_map[i].name, retval);
        if((retval = PAPI_add_event(recorder->EventSet, code)) != PAPI_OK)
            MIR_ABORT(MIR_ERROR_STR "Recorder %d PAPI_add_event %x failed [%d]!\n", id, code, retval);
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
    MIR_ASSERT(recorder != NULL);

    // Update state time
    mir_recorder_record_state_transition(recorder, 0);

    // Write state time to file
    char state_time_filename[MIR_LONG_NAME_LEN];
    sprintf(state_time_filename, "%s-state-time-%d.rec", MIR_RECORDER_FILE_NAME_PREFIX, recorder->id);

    // Open state_time_file
    FILE* state_time_file = fopen(state_time_filename, "w");
    MIR_ASSERT(state_time_file != NULL);

    // Write state time
    fprintf(state_time_file, "%d", recorder->id);
    for(int i=0; i<MIR_RECORDER_NUM_STATES; i++)
        fprintf(state_time_file, ",%" MIR_FORMSPEC_UL "", recorder->state_time[i]);
    fprintf(state_time_file, "\n");
    if(recorder->id == 0)
    {
        fprintf(state_time_file, "THREAD");
        for(int i=0; i<MIR_RECORDER_NUM_STATES; i++)
            fprintf(state_time_file, ",%s", mir_state_name_string[i]);
        fprintf(state_time_file, "\n");
    }
    fclose(state_time_file);

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
        sprintf(config_filename, "%s-trace-config.rec", MIR_RECORDER_FILE_NAME_PREFIX);

        // Open config file
        FILE* config_file = fopen(config_filename, "w");
        MIR_ASSERT(config_file != NULL);

        // Write to config file
        fprintf(config_file, "creation_cycle=%" MIR_FORMSPEC_UL "\ndestruction_cycle=%" MIR_FORMSPEC_UL "\nnum_workers=%d\n", runtime->init_time, mir_get_cycles(), runtime->num_workers);
        
        // Close config file
        fclose(config_file);

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
    MIR_ASSERT(recorder != NULL);

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
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS 
#ifdef __tile__ 
                4
#else
                MIR_RECORDER_NUM_PAPI_HWPC
#endif
#else
                1
#endif
                );
        
#ifdef MIR_RECORDER_USE_HW_PERF_COUNTERS 
#ifdef __tile__ 
        for(int j=0; j<4; j++)
#else
        for(int j=0; j<MIR_RECORDER_NUM_PAPI_HWPC; j++)
#endif
            fprintf(recorder->buffer_file, "%s=%lld,", perf_ctr_map[j].name, event->values[j]);
#else
        fprintf(recorder->buffer_file, "%s=%lld,", "NA", event->values[0]);
#endif

        fprintf(recorder->buffer_file, ":%s\n", 
                event->meta_data); 
    }

    // Rewind heads
    recorder->event_buffer_head = 0;
    recorder->state_buffer_head = 0;

}/*}}}*/

void mir_recorder_record_state_transition(struct mir_recorder_t* recorder, mir_state_name_t next_state)
{/*{{{*/
    MIR_ASSERT(recorder != NULL);
    uint64_t this_instant = mir_get_cycles();
    recorder->state_time[recorder->this_state] += (this_instant - recorder->this_state_trans_time);
    recorder->prev_state = recorder->this_state;
    recorder->this_state = next_state;
    recorder->this_state_trans_time = this_instant;
}/*}}}*/

void mir_recorder_state_begin(mir_state_name_t name) 
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Get this worker's reocrder
    struct mir_recorder_t* r = worker->recorder;
    MIR_ASSERT(r != NULL);

    // Update state time
    mir_recorder_record_state_transition(r, name);

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

void mir_recorder_state_end(const char* meta_data, uint32_t meta_data_length) 
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Get this worker's reocrder
    struct mir_recorder_t* r = worker->recorder;
    MIR_ASSERT(r != NULL);

    // Pop from state state_stack
    r->state_stack_head--;

    struct mir_state_t* state = &r->state_buffer[r->state_buffer_head];
    struct mir_state_t* ending_state = &r->state_stack[r->state_stack_head];

    // Update state time
    struct mir_state_t* next_state = &r->state_stack[(r->state_stack_head)-1];
    mir_recorder_record_state_transition(r, next_state->name);

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
    if (r->state_buffer_head == (MIR_RECORDER_BUFFER_MAX_SIZE-1))
    {
        mir_recorder_write_to_file(r);
        MIR_ASSERT(r->state_buffer_head == 0);
    }
}/*}}}*/

void mir_recorder_event(const char* meta_data, uint32_t meta_data_length)
{/*{{{*/
    // Get this worker
    struct mir_worker_t* worker = mir_worker_get_context();
    MIR_ASSERT(worker != NULL);

    // Get this worker's reocrder
    struct mir_recorder_t* r = worker->recorder;
    MIR_ASSERT(r != NULL);

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
    {
        mir_recorder_write_to_file(r);
        MIR_ASSERT(r->state_buffer_head == 0);
    }
}/*}}}*/
