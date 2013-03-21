#!/usr/bin/python2

import sys
import os
import optparse
from itertools import *
from time import gmtime, strftime

# Appconfig file offsets
acfo_creation_time = 0
acfo_destruction_time = 1
acfo_num_workers = 2

# Worker file offsets
wfo_statesnevents = 3
wfo_statesnevents_delim = ':'
wfo_statedefs = 1
wfo_eventdefs = 2

creation_time = 0
destruction_time = 0
num_workers = 0
unknown_happenings_found = False
paraver_file_suffix = ''

def parse_worker_files():
    global num_workers, creation_time, destruction_time, unknown_happenings_found, paraver_file_suffix

    # Open the prv file and write its header
    # For now, the header is hardcoded with single node and as many processors as workers
    if(paraver_file_suffix != ''):
        prv_name = paraver_file_suffix + '-paraver.prv'
    else:
        prv_name = str(creation_time) + '-paraver.prv'
    prv = open(prv_name, 'w')

    # Open the perfctrinfo file pci
    if(paraver_file_suffix != ''):
        pci_name = paraver_file_suffix + '.perfctrinfo'
    else:
        pci_name = str(creation_time) + '.perfctrinfo'
    pci = open(pci_name, 'w')

    # Open the task meta info file tmi
    if(paraver_file_suffix != ''):
        tmi_name = paraver_file_suffix + '.taskmetainfo'
    else:
        tmi_name = str(creation_time) + '.taskmetainfo'
    tmi = open(tmi_name, 'w')

    # Paraver header data
    write_date = strftime("%d/%m/%Y at %H:%M", gmtime())
    prv_run_length = destruction_time - creation_time

    paraver_header = '#Paraver (%s):%d:1(%d):1:1(%d:1)\n' %(write_date, prv_run_length, num_workers, num_workers)
    prv.write(paraver_header)

    # For each worker record, parse events and states and what not!
    for i in range(0, num_workers):

        # Read worker record
        worker_file_name = str(creation_time) + '-recorder-' + str(i) + '.rec'
        worker_file = open(worker_file_name, 'r')
        lines = worker_file.readlines()
        worker_file.close()

        if i == 0:
            # Get states and events definitions from the master worker record
            state_defs = lines[wfo_statedefs].rstrip(':\n').split(':')
            event_defs = lines[wfo_eventdefs].rstrip(':\n').split(':')
            state_dict = {}
            event_dict = {}

            # We can now write the paraver pcf file
            if(paraver_file_suffix != ''):
                pcf_name = paraver_file_suffix + '-paraver.pcf'
            else:
                pcf_name = str(creation_time) + '-paraver.pcf'
            pcf = open(pcf_name, 'w')

            # pcf state colors
            colors_paraver = ['{117,195,255}', '{0,0,255}', '{255,255,255}', '{255,0,0}', '{255,0,174}', '{179,0,0}', '{0,255,0}', '{255,255,0}', '{235,0,0}', '{0,162,0}', '{255,0,255}', '{100,100,177}', '{172,174,41}', '{255,144,26}', '{2,255,177}', '{192,224,0}', '{66,66,66}']

            colors_brewer = ['{141, 211, 199}','{ 255, 255, 179}','{ 190, 186, 218}','{ 251, 128, 114}','{ 128, 177, 211}','{ 253, 180, 98}','{ 179, 222, 105}','{ 252, 205, 229}','{ 217, 217, 217}','{ 188, 128, 189}','{ 204, 235, 197}','{ 255, 237, 111}']

            # pcf header
            pcf.write('DEFAULT_OPTIONS\n\nLEVEL               THREAD\nUNITS               NANOSEC\nLOOK_BACK           100\nSPEED               1\nFLAG_ICONS          ENABLED\nNUM_OF_STATE_COLORS 1000\nYMAX_SCALE          37\n\n\nDEFAULT_SEMANTIC\n\nTHREAD_FUNC          State As Is\n\n')

            # pcf states, colors and events defs
            pcf.write('\nSTATES\n')
            for si,sn in enumerate(state_defs):
                pcf.write('%d\t%s\n' %(si, sn))
                # Also create the state dictionary entry
                state_dict[sn]= si

            pcf.write('\nSTATES_COLOR\n')
            #for ci,co in enumerate(colors_paraver):
            for ci,co in enumerate(colors_brewer):
                pcf.write('%d\t%s\n' %(ci, co))

            for ei,en in enumerate(event_defs):
                pcf.write('\nEVENT_TYPE\n')
                pcf.write('%d\t%d\t%s\n' %(ei, 540680+ei, en))
                # Also create the event dictionary entry
                event_dict[en]= 540680+ei

            pcf.close()

        # Now we decode all events and states for this worker
        # It is also better to write into the paraver prv file within the same loop
        # So we create a state list for later sorting
        state_list = []
        for line in lines[wfo_statesnevents:]:
            happening = line.rstrip().split(wfo_statesnevents_delim)

            if happening[0] == 's':
                # Its a state type
                # Add paraver prv file state lines
                # st format: [ btime, etime, type, id, parent_id ]
                if(len(happening) > 6):
                    st = [int(happening[3])-creation_time, int(happening[4])-creation_time, state_dict[happening[5]], int(happening[1]), int(happening[2]), happening[6]]
                else:
                    st = [int(happening[3])-creation_time, int(happening[4])-creation_time, state_dict[happening[5]], int(happening[1]), int(happening[2])]
                state_list.append(st)

            elif happening[0] == 'e':
                # Its an event type
                # MIR outputs an event set. Must extract multiple events.
                try:
                    event_set = happening[4].rstrip(',').split(',')
                except IndexError:
                    print worker_file_name
                    print happening
                    raise
                for event_desc in event_set:
                    ed_split = event_desc.split('=')
                    # Add paraver prv file event line
                    event_string = '2:%d:1:1:%d:%d:%d:%s\n' %(i+1,i+1,int(happening[2])-creation_time, event_dict[ed_split[0]], ed_split[1]) 
                    prv.write(event_string)
                    if(ed_split[0] == 'DATA_CACHE_STALL'):
                        pci.write('%s\t' %(ed_split[1]) )

            else:
                unknown_happenings_found = True

        # Now we flatten the state_list
        # Flatten idea: Parent state breaks its duration based on child state duration
        # Parent(0-10), Child(4-6) becomes Parent(0-4) Child(4-6) Parent(6-10)
        # Do not want to do this recursively - Python stack limit
        # Therefore flattening might be slow!
        # Flattening begins with sorting the state_list by begin_time
        state_list.sort()
        for indi, state in enumerate(state_list):
            # state format: [ btime, etime, type, id, parent_id ]
            sid = state[3]
            btime = state[0]
            etime = state[1]
            typ = state[2]
            for indj in range(indi+1,len(state_list)):
                cbtime = state_list[indj][0] # Child state begin time
                cetime = state_list[indj][1] # Child state end time
                cpid = state_list[indj][4] # Parent state id
                if(cbtime < etime and cpid == sid):
                    state_string = '1:%d:1:1:%d:%d:%d:%d\n' %(i+1,i+1,btime,cbtime,typ) 
                    prv.write(state_string)
                    if(len(state) == 6):
                        # Write tmi string
                        tmi_str = 'worker=%d:%d:%d:%d:%s\n' %(i+1, btime, cbtime, typ, state[5])
                        tmi.write(tmi_str)
                    btime = cetime 
                else:
                    break
            # Write the last chunk
            state_string = '1:%d:1:1:%d:%d:%d:%d\n' %(i+1,i+1,btime,etime,typ) 
            prv.write(state_string)
            if(len(state) == 6):
                # Write tmi string
                tmi_str = 'worker=%d:%d:%d:%d:%s\n' %(i+1, btime, etime, typ, state[5])
                tmi.write(tmi_str)
    
    # Finally close the paraver prv file
    prv.close()

    # Finally close the pci file
    pci.write('\n')
    pci.close()

    # Finally close the tmi file
    tmi.write('\n')
    tmi.close()

def parse_input_file(file_name):
    global num_workers, creation_time, destruction_time

    # Read appconfig file
    appconfig_file = open(file_name, 'r')
    lines = appconfig_file.readlines()
    appconfig_file.close()

    # Decode creation time, destruction_time, num workers and worker map
    creation_time = int(lines[acfo_creation_time].split('=').pop().rstrip())
    destruction_time = int(lines[acfo_destruction_time].split('=').pop().rstrip())
    num_workers =  int(lines[acfo_num_workers].split('=').pop().rstrip())

def print_stats():
    global num_workers, creation_time, destruction_time 
    print 'creation_time = %d, destruction_time = %d, num_worker = %d' %(creation_time, destruction_time, num_workers)

def set_paraver_file_suffix(suffix):
    global paraver_file_suffix
    paraver_file_suffix = suffix

def main():
    p = optparse.OptionParser(description='Converts mir recorder files to paraver format (vite). Takes appconfig.rec file as input. Optional args: paraver_name_suffix, no print', prog='mirtoparaver', version='mirtoparaver v0.1', usage='mirtoparaver *.appconfig.rec')
    options, arguments = p.parse_args()
    if len(arguments) == 1:
        print 'Parsing file: ' + arguments[0]
        set_paraver_file_suffix('');
        parse_input_file(arguments[0])
        parse_worker_files()
        print_stats()
    elif len(arguments) == 2:
        print 'Parsing file: ' + arguments[0]
        set_paraver_file_suffix(arguments[1]);
        parse_input_file(arguments[0])
        parse_worker_files()
        print_stats()
    elif len(arguments) == 3:
        #print 'Parsing file: ' + arguments[0]
        set_paraver_file_suffix(arguments[1]);
        parse_input_file(arguments[0])
        parse_worker_files()
        #print_stats()
    else:
        p.print_help()

if __name__ == '__main__':
    main()
