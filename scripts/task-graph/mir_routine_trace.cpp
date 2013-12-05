#include "pin.H"
#include "portability.H"
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <omp.h>

KNOB<string> KnobOutputFileSuffix(KNOB_MODE_WRITEONCE, "pintool",
    "o", "mir_call_graph", "specify mir routine trace file suffix");
KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE, "pintool",
    "i", "0", "append pid to output");
KNOB<string>   KnobRoutineNames(KNOB_MODE_WRITEONCE, "pintool",
    "s", "", "specify routines (csv) to trace");
KNOB<string>   KnobCalledRoutineNames(KNOB_MODE_WRITEONCE, "pintool",
    "c", "", "specify routines (csv) that are called from traced routines");
KNOB<BOOL>   KnobCalcMemShare(KNOB_MODE_WRITEONCE, "pintool",
    "m", "0", "calculate memory sharing (a time consuming process!)");

#define EXCLUDE_STACK_INS_FROM_MEM_FP 1
//#define GET_INS_MIX 1

//std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) 
//{
    //std::stringstream ss(s);
    //std::string item;
    //while (std::getline(ss, item, delim)) {
        //elems.push_back(item);
    //}
    //return elems;
//}

void tokenize(const std::string& str, const std::string& delimiters , std::vector<std::string>& tokens)
{/*{{{*/
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}/*}}}*/

UINT64 get_cycles() {
    unsigned a, d;
    __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
    return ((uint64_t)a) | (((uint64_t)d) << 32);
}

#define NAME_SIZE 256
typedef struct _MIR_ROUTINE_STAT_
{/*{{{*/
    UINT64 creation_time;
    UINT64 ins_count;
    UINT64 stack_read;
    UINT64 stack_write;
    UINT64 mem_read;
    UINT64 mem_write;
    std::set<VOID*> mem_fp;
#ifdef GET_INS_MIX
    UINT64 ins_mix[XED_CATEGORY_LAST];
#endif
    char name[NAME_SIZE];
    UINT64 ccr; // Computation to communication ratio
    UINT64 clr; // Computation to load ratio
    //std::vector<VOID*> mrefs_read;
    //std::vector<VOID*> mrefs_write;
    UINT64 mem_fp_sz;
    std::vector<UINT64> mem_share;
    struct _MIR_ROUTINE_STAT_ * next;
} MIR_ROUTINE_STAT;/*}}}*/

MIR_ROUTINE_STAT* g_stat_list = NULL;
MIR_ROUTINE_STAT* g_current_stat = NULL;
std::stack<MIR_ROUTINE_STAT*> g_stat_stack;

VOID MIRRoutineUpdateMemRefRead(VOID* memp)
{/*{{{*/
    if(g_current_stat)
    {
        g_current_stat->mem_read++;
        g_current_stat->mem_fp.insert(memp);
        //g_current_stat->mrefs_read.push_back(memp);
    }
}/*}}}*/

VOID MIRRoutineUpdateMemRefWrite(VOID* memp)
{/*{{{*/
    if(g_current_stat)
    {
        g_current_stat->mem_write++;
        g_current_stat->mem_fp.insert(memp);
        //g_current_stat->mrefs_write.push_back(memp);
    }
}/*}}}*/

#ifdef GET_INS_MIX
VOID MIRRoutineUpdateInsMix(INT32 index)
{/*{{{*/
    if(g_current_stat)
        g_current_stat->ins_mix[index]++;
}/*}}}*/
#endif

VOID MIRRoutineUpdateInsCount()
{/*{{{*/
    if(g_current_stat)
        g_current_stat->ins_count++;
}/*}}}*/

VOID MIRRoutineUpdateStackRead()
{/*{{{*/
    if(g_current_stat)
        g_current_stat->stack_read++;
}/*}}}*/

VOID MIRRoutineUpdateStackWrite()
{/*{{{*/
    if(g_current_stat)
        g_current_stat->stack_write++;
}/*}}}*/

VOID MIRRoutineEntry(VOID* name)
{/*{{{*/
    // Create new stat counter
    MIR_ROUTINE_STAT* stat = new MIR_ROUTINE_STAT;
    stat->creation_time = get_cycles();
    stat->ins_count = 0;
    stat->stack_read = 0;
    stat->stack_write = 0;
    stat->mem_read = 0;
    stat->mem_write = 0;
    memcpy(stat->name, name, sizeof(char) * NAME_SIZE);
#ifdef GET_INS_MIX 
    memset(&stat->ins_mix, 0, sizeof(UINT64) * XED_CATEGORY_LAST);
#endif
    stat->next = g_stat_list;
    g_stat_list = stat;
    
    // Save context
    g_stat_stack.push(g_stat_list);
    g_current_stat = g_stat_list;

    // Debug
    //std::cout << "Routine entered!\n";
}/*}}}*/

VOID MIRRoutineExit()
{/*{{{*/
    // Memory optimziation: Free the mem_fp set
    // We are only intersted in mem_fp size for now
    if(g_current_stat)
    {
        g_current_stat->mem_fp_sz = g_current_stat->mem_fp.size();
        g_current_stat->mem_fp.clear();
    }
    
    // Restore context
    g_stat_stack.pop();
    if(!g_stat_stack.empty())
        g_current_stat = g_stat_stack.top();

    // Debug
    //std::cout << "Routine exited!\n";
}/*}}}*/

VOID Image(IMG img, VOID *v)
{/*{{{*/
    std::string delims = ",";
    std::vector<string>::iterator it;

    // The traced routines
    std::string traced_routines_csv = KnobRoutineNames.Value();
    std::vector<std::string> traced_routines;
    tokenize(traced_routines_csv, delims, traced_routines);
    for(it = traced_routines.begin(); it != traced_routines.end(); it++)
    {
        //std::cout << "Analyzing traced routine: " << *it << std::endl;
        RTN mirRtn = RTN_FindByName(img, (*it).c_str());
        if (RTN_Valid(mirRtn))
        {/*{{{*/
            //std::cout << "Routine " << *it << " is valid" << std::endl;
            RTN_Open(mirRtn);

            // Create new stats counter for each entry and exit of mir_execute_task
            RTN_InsertCall(mirRtn, IPOINT_BEFORE, (AFUNPTR)MIRRoutineEntry, IARG_PTR, RTN_Name(mirRtn).c_str(), IARG_END);
            RTN_InsertCall(mirRtn, IPOINT_AFTER, (AFUNPTR)MIRRoutineExit, IARG_END);

            // For each instruction with routine, update entries in the stats counter
            for (INS ins = RTN_InsHead(mirRtn); INS_Valid(ins); ins = INS_Next(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateInsCount, IARG_END);
#ifdef GET_INS_MIX
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateInsMix, IARG_UINT32, INS_Category(ins), IARG_END);
#endif

                // Instrument stack accesses
                // If instruction operates using the SP or FP, its a stack operation
                if(INS_IsStackRead(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateStackRead, IARG_END);
                if(INS_IsStackWrite(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateStackWrite, IARG_END);

#ifdef EXCLUDE_STACK_INS_FROM_MEM_FP 
                if(!INS_IsStackRead(ins) && !INS_IsStackWrite(ins))
                {
#endif
                    // Get memory operands of instruction
                    UINT32 memOperands = INS_MemoryOperandCount(ins);
                    // Iterate over each memory operand of the instruction.
                    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
                    {
                        if (INS_MemoryOperandIsRead(ins, memOp))
                        {
                            INS_InsertPredicatedCall(
                                ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateMemRefRead,
                                //IARG_INST_PTR,
                                IARG_MEMORYOP_EA, memOp,
                                //IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_END);
                        }
                        // Note that in some architectures a single memory operand can be 
                        // both read and written (for instance incl (%eax) on IA-32)
                        // In that case we instrument it once for read and once for write.
                        if (INS_MemoryOperandIsWritten(ins, memOp))
                        {
                            INS_InsertPredicatedCall(
                                ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateMemRefWrite,
                                //IARG_INST_PTR,
                                IARG_MEMORYOP_EA, memOp,
                                //IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_END);
                        }
                    }
#ifdef EXCLUDE_STACK_INS_FROM_MEM_FP 
                }
#endif
            }

            RTN_Close(mirRtn);
        }/*}}}*/
    }

    // The routines called by the traced routines
    std::string called_routines_csv = KnobCalledRoutineNames.Value();
    std::vector<std::string> called_routines;
    tokenize(called_routines_csv, delims, called_routines);
    for(it = called_routines.begin(); it != called_routines.end(); it++)
    {
        //std::cout << "Analyzing called routine: " << *it << std::endl;
        RTN mirRtn = RTN_FindByName(img, (*it).c_str());
        if (RTN_Valid(mirRtn))
        {/*{{{*/
            //std::cout << "Routine " << *it << " is valid" << std::endl;
            RTN_Open(mirRtn);

            // For each instruction of the routine, update entries in the stats counter
            for (INS ins = RTN_InsHead(mirRtn); INS_Valid(ins); ins = INS_Next(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateInsCount, IARG_END);
#ifdef GET_INS_MIX
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateInsMix, IARG_UINT32, INS_Category(ins), IARG_END);
#endif

                // Instrument stack accesses
                // If instruction operates using the SP or FP, its a stack operation
                if(INS_IsStackRead(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateStackRead, IARG_END);
                if(INS_IsStackWrite(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateStackWrite, IARG_END);

#ifdef EXCLUDE_STACK_INS_FROM_MEM_FP 
                if(!INS_IsStackRead(ins) && !INS_IsStackWrite(ins))
                {
#endif
                    // Get memory operands of instruction
                    UINT32 memOperands = INS_MemoryOperandCount(ins);
                    // Iterate over each memory operand of the instruction.
                    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
                    {
                        if (INS_MemoryOperandIsRead(ins, memOp))
                        {
                            INS_InsertPredicatedCall(
                                ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateMemRefRead,
                                //IARG_INST_PTR,
                                IARG_MEMORYOP_EA, memOp,
                                //IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_END);
                        }
                        // Note that in some architectures a single memory operand can be 
                        // both read and written (for instance incl (%eax) on IA-32)
                        // In that case we instrument it once for read and once for write.
                        if (INS_MemoryOperandIsWritten(ins, memOp))
                        {
                            INS_InsertPredicatedCall(
                                ins, IPOINT_BEFORE, (AFUNPTR)MIRRoutineUpdateMemRefWrite,
                                //IARG_INST_PTR,
                                IARG_MEMORYOP_EA, memOp,
                                //IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_END);
                        }
                    }
#ifdef EXCLUDE_STACK_INS_FROM_MEM_FP 
                }
#endif
            }

            RTN_Close(mirRtn);
        }/*}}}*/
    }
}/*}}}*/

VOID MIRRoutineUpdateMemFp(MIR_ROUTINE_STAT * stat)
{/*{{{*/
    // Mem footprint count
    // Uniquify read and write refs to get sets R and W
    // Union of R and W is the footprint
    //std::set<VOID*> mem_read_ref_set(stat->mrefs_read.begin(), stat->mrefs_read.end());
    //std::set<VOID*> mem_write_ref_set(stat->mrefs_write.begin(), stat->mrefs_write.end());
    //std::insert_iterator< std::set<VOID*> > ins_it(stat->mem_fp, stat->mem_fp.begin());
    //std::set_union(mem_read_ref_set.begin(), mem_read_ref_set.end(), mem_write_ref_set.begin(), mem_write_ref_set.end(), ins_it);

    // Also update computation to (commuincation,load) ratios
    //UINT64 communication = stat->mrefs_read.size() + stat->mrefs_write.size();
    UINT64 communication = stat->mem_read + stat->mem_write;
    if(communication == 0)
        stat->ccr = stat->ins_count;
    else
        stat->ccr = (int)((double)(stat->ins_count) / communication + 0.5);
    UINT64 load = stat->mem_read;
    if(load == 0)
        stat->clr = stat->ins_count;
    else
        stat->clr = (int)((double)(stat->ins_count) / load + 0.5);
    // std::cout << communication << ", " << load << ", " << stat->ins_count << std::endl;
}/*}}}*/

VOID MIRRoutineUpdateMemShare(MIR_ROUTINE_STAT * this_stat, size_t cutoff)
{/*{{{*/
    // Mem footprint share
    // Intersection of this instance's memory footprint with all other instances within cutoff
    // Cutoff is used to limit number of intersections computed
    size_t sz = 0;
    for (MIR_ROUTINE_STAT* stat = g_stat_list; sz <= cutoff; stat = stat->next, sz++)
    {
        if(sz != cutoff)
        {
            std::set<VOID*> mem_fp_intersection;
            std::insert_iterator< std::set<VOID*> > ins_it(mem_fp_intersection, mem_fp_intersection.begin());
            std::set_intersection(stat->mem_fp.begin(), stat->mem_fp.end(), this_stat->mem_fp.begin(), this_stat->mem_fp.end(), ins_it);
            this_stat->mem_share.push_back(mem_fp_intersection.size());
        }
        else
        {
            this_stat->mem_share.push_back(this_stat->mem_fp.size());
        }
    }
}/*}}}*/

//// Anonymous memory map range
//typedef struct _anon_map_range
//{
    //UINT64 start;
    //UINT64 end;
//} anon_map_range;

VOID Fini(INT32 code, VOID *v)
{/*{{{*/
    std::cout << "Finalizing ..." << std::endl;
    std::string filename;

    // Dump /proc/<pid>/maps
    filename.clear();
    filename = KnobOutputFileSuffix.Value();
    if( KnobPid )
        filename += "." + decstr( getpid_portable() );
    filename += ".mmap";
    std::cout << "Writing memory map (/proc/<pid>/maps) to file: " << filename << " ..." << std::endl;
    char cmd[256];
    sprintf(cmd, "cat /proc/%d/maps > %s", getpid_portable(), filename.c_str());
    system(cmd);

    // Get anonympus page maps
    //filename.clear();
    //filename = "/proc/" + decstr(getpid_portable()) + "/maps";
    //std::ifstream maps_file;
    //maps_file.open(filename.c_str());
    //std::string maps_line;
    //std::vector<anon_map_range> vamr;
    //while(std::getline(maps_file, maps_line))
    //{
        //std::string delim = " ";
        //std::vector<std::string> columns;
        //tokenize(maps_line, delim, columns);
        //// Anonymous mappings have one five columns
        //// FIXME: Fragile! Might break.
        //if(columns.size() == 5) 
        //{
            //std::vector<std::string> ranges;
            //std::string delim = "-";
            //tokenize(columns[0], delim, ranges);
            ////std::cout << ranges[0] << delim << ranges[1] << std::endl;
            //anon_map_range amr;
            //amr.start = atoi(ranges[0].c_str());
            //amr.end = atoi(ranges[1].c_str());
            //vamr.push_back(amr);
        //}
    //}
    //maps_file.close();

    // Update instances in parallel
    std::cout << "Updating statistics in parallel ..." << std::endl;
    UINT64 num_instances = 0;
#pragma omp parallel
{
#pragma omp single
{
    std::cout << "Updating memory footprint ..." << std::endl;
    for (MIR_ROUTINE_STAT* stat = g_stat_list; stat; stat = stat->next)
    {
        num_instances++;
//#pragma omp task
        {
            // Update memory footprint
            MIRRoutineUpdateMemFp(stat);
        }
    }
//#pragma omp taskwait
    if(KnobCalcMemShare)
    {
        std::cout << "Updating memory sharing ..." << std::endl;
        std::cout << "Using " << omp_get_num_threads() << " threads" << std::endl;
        size_t cutoff = 0;
        for (MIR_ROUTINE_STAT* stat = g_stat_list; stat; stat = stat->next, cutoff++)
        {
#pragma omp task
            {
                // Update memory sharing
                MIRRoutineUpdateMemShare(stat, cutoff);
            }
        }
#pragma omp taskwait
    }
}
}

    // Open detail file
    filename.clear();
    filename = KnobOutputFileSuffix.Value(); // + "." + KnobRoutineName.Value();
    if( KnobPid )
        filename += "." + decstr( getpid_portable() );
    filename += ".detail.csv";
    std::ofstream out;
    out.open(filename.c_str());
    std::cout << "Writing detail to file: " << filename << " ..." << std::endl;
    // Write detail as csv
#ifdef GET_INS_MIX
    out << "creation_time,ins_count,stack_read,stack_write,mem_fp,ccr,clr,mem_read,mem_write,name,[ins_mix]" << std::endl;
#else
    out << "creation_time,ins_count,stack_read,stack_write,mem_fp,ccr,clr,mem_read,mem_write,name" << std::endl;
#endif
    for (MIR_ROUTINE_STAT* stat = g_stat_list; stat; stat = stat->next)
    {
        out << stat->creation_time << "," 
            << stat->ins_count << ","
            << stat->stack_read<< ","
            << stat->stack_write<< ","
            << stat->mem_fp_sz << ","
            << stat->ccr << ","
            << stat->clr << ","
            //<< stat->mrefs_read.size() << ","
            //<< stat->mrefs_write.size() << std::endl;
            << stat->mem_read << ","
            << stat->mem_write << ","
            << stat->name;
#ifdef GET_INS_MIX 
        out << ",[";
        for(int c=0; c<XED_CATEGORY_LAST; c++)
        {
            out << stat->ins_mix[c] << ",";
        }
        out << "]";
#endif
        out << std::endl;
    }

    if(KnobCalcMemShare)
    {
        // Open memory share file
        filename.clear();
        filename = KnobOutputFileSuffix.Value(); // + "." + KnobRoutineNames.Value();
        if( KnobPid )
            filename += "." + decstr( getpid_portable() );
        filename += ".mem_share.csv";
        out.open(filename.c_str());
        std::cout << "Writing memory sharing detail to file: " << filename << " ..." << std::endl;
        // Write mem share per task to file
        for (MIR_ROUTINE_STAT* stat = g_stat_list; stat; stat = stat->next)
        {
            UINT64 count = num_instances;
            std::vector<UINT64>::iterator it;
            const char *padding = "";
            for(it = stat->mem_share.begin(); it != stat->mem_share.end(); it++)
            {
                out << padding << *it;
                padding = ",";
                count--;
            }
            while(count!=0)
            {
                out << ",";
                count--;
            }
            out << std::endl;
        }
        // Close file
        out.close();
    }

}/*}}}*/

INT32 Usage()
{/*{{{*/
    cerr << "This tool produces a trace of calls specified using the below syntax." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    // Initialize pin & symbol manager
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}/*}}}*/
