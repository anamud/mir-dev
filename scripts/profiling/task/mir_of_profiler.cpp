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
#include <assert.h>
#include <sys/shm.h>

KNOB<string> KnobOutputFileSuffix(KNOB_MODE_WRITEONCE, "pintool",
    "o", "mir-ofp", "specify output file suffix");
KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE, "pintool",
    "i", "0", "append pid to output");
KNOB<string>   KnobFunctionNames(KNOB_MODE_WRITEONCE, "pintool",
    "s", "", "specify outline functions (csv)");
KNOB<string>   KnobCalledFunctionNames(KNOB_MODE_WRITEONCE, "pintool",
    "c", "", "specify functions called (csv) from outline functions");
KNOB<BOOL>   KnobCalcMemShare(KNOB_MODE_WRITEONCE, "pintool",
    "m", "0", "calculate memory sharing (NOTE: a time consuming process!)");

#define EXCLUDE_STACK_INS_FROM_MEM_FP 1
#define GET_INS_MIX 1

// MIR shared memory connection
// DO NOT CHANGE DEFINED VALUES WITHOUT MAKING A SIMILAR CHANGE IN MIR
#define MIR_SHM_KEY 31415926
#define MIR_SHM_SIZE 16
#define MIR_SHM_SIGREAD '*'
bool g_shmat_done = false;
int g_shmid;
char* g_shm;

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
typedef struct _MIR_FUNCTION_STAT_
{/*{{{*/
    UINT64 id;
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
    std::vector<UINT64> task_create_instant;
    std::vector<UINT64> task_wait_instant;
    struct _MIR_FUNCTION_STAT_ * next;
} MIR_FUNCTION_STAT;/*}}}*/

MIR_FUNCTION_STAT* g_stat_list = NULL;
MIR_FUNCTION_STAT* g_current_stat = NULL;
std::stack<MIR_FUNCTION_STAT*> g_stat_stack;

VOID MIROutlineFunctionUpdateMemRefRead(VOID* memp)
{/*{{{*/
    if(g_current_stat)
    {
        g_current_stat->mem_read++;
        g_current_stat->mem_fp.insert(memp);
        //g_current_stat->mrefs_read.push_back(memp);
    }
}/*}}}*/

VOID MIROutlineFunctionUpdateMemRefWrite(VOID* memp)
{/*{{{*/
    if(g_current_stat)
    {
        g_current_stat->mem_write++;
        g_current_stat->mem_fp.insert(memp);
        //g_current_stat->mrefs_write.push_back(memp);
    }
}/*}}}*/

#ifdef GET_INS_MIX
VOID MIROutlineFunctionUpdateInsMix(INT32 index)
{/*{{{*/
    if(g_current_stat)
        g_current_stat->ins_mix[index]++;
}/*}}}*/
#endif

VOID MIROutlineFunctionUpdateInsCount()
{/*{{{*/
    if(g_current_stat)
        g_current_stat->ins_count++;
}/*}}}*/

VOID MIROutlineFunctionUpdateStackRead()
{/*{{{*/
    if(g_current_stat)
        g_current_stat->stack_read++;
}/*}}}*/

VOID MIROutlineFunctionUpdateStackWrite()
{/*{{{*/
    if(g_current_stat)
        g_current_stat->stack_write++;
}/*}}}*/

VOID MIROutlineFunctionEntry(VOID* name)
{/*{{{*/
    // Attach to MIR shared memory
    if(g_shmat_done == false)
    {
        g_shmat_done = true;

        if ((g_shmid = shmget(MIR_SHM_KEY, MIR_SHM_SIZE, 0666)) < 0) 
        {
            std::cerr << "Call to shmget failed!" << std::endl;
            exit(1);
        }

        g_shm = (char*) shmat(g_shmid, NULL, 0);
        if (g_shm == NULL) 
        {
            std::cerr << "No shared memory. Call to shmat returned NULL!" << std::endl;
            exit(1);
        }
    }

    // Read task id written by MIR
    char buf[MIR_SHM_SIZE];
    for(int i=0; i<MIR_SHM_SIZE; i++)
        buf[i] = g_shm[i];
    //std::cout << "MIR shared memory content: " << buf << std::endl;
    // Signal MIR that reading is complete
    *g_shm = MIR_SHM_SIGREAD;

    // Create new stat counter
    MIR_FUNCTION_STAT* stat = new MIR_FUNCTION_STAT;
    stat->id = atoi(buf);
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

    g_stat_stack.push(g_stat_list);
    g_current_stat = g_stat_list;

    // Debug
    //std::cout << "Function entered!\n";
}/*}}}*/

VOID MIROutlineFunctionExit()
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
    else
        g_current_stat = NULL;

    // Debug
    //std::cout << "Function exited!\n";
}/*}}}*/

VOID MIRTaskCreateBefore()
{/*{{{*/
    //std::cout << "Entering task create!" << std::endl;
    if(g_current_stat)
        g_current_stat->task_create_instant.push_back(g_current_stat->ins_count);
}/*}}}*/

VOID MIRTaskWaitAfter()
{/*{{{*/
    //std::cout << "Exited task wait!" << std::endl;
    if(g_current_stat)
        g_current_stat->task_wait_instant.push_back(g_current_stat->ins_count);
}/*}}}*/

VOID Image(IMG img, VOID *v)
{/*{{{*/
    std::string delims = ",";
    std::vector<string>::iterator it;

    // The outline functions
    std::string outline_functions_csv = KnobFunctionNames.Value();
    std::vector<std::string> outline_functions;
    tokenize(outline_functions_csv, delims, outline_functions);
    assert(outline_functions.size() != 0);
    for(it = outline_functions.begin(); it != outline_functions.end(); it++)
    {
        //std::cout << "Analyzing outline function: " << *it << std::endl;
        RTN mirRtn = RTN_FindByName(img, (*it).c_str());
        if (RTN_Valid(mirRtn))
        {/*{{{*/
            //std::cout << "Function " << *it << " is valid" << std::endl;
            RTN_Open(mirRtn);

            // Create new stats counter for each entry and exit of mir_execute_task
            RTN_InsertCall(mirRtn, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionEntry, IARG_PTR, RTN_Name(mirRtn).c_str(), IARG_END);
            RTN_InsertCall(mirRtn, IPOINT_AFTER, (AFUNPTR)MIROutlineFunctionExit, IARG_END);

            // For each instruction with function, update entries in the stats counter
            for (INS ins = RTN_InsHead(mirRtn); INS_Valid(ins); ins = INS_Next(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateInsCount, IARG_END);
#ifdef GET_INS_MIX
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateInsMix, IARG_UINT32, INS_Category(ins), IARG_END);
#endif

                // Instrument stack accesses
                // If instruction operates using the SP or FP, its a stack operation
                if(INS_IsStackRead(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateStackRead, IARG_END);
                if(INS_IsStackWrite(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateStackWrite, IARG_END);

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
                                ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateMemRefRead,
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
                                ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateMemRefWrite,
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

    // The functions called by the outline functions
    std::string called_functions_csv = KnobCalledFunctionNames.Value();
    std::vector<std::string> called_functions;
    tokenize(called_functions_csv, delims, called_functions);
    assert(called_functions.size() != 0);
    for(it = called_functions.begin(); it != called_functions.end(); it++)
    {
        //std::cout << "Analyzing called function: " << *it << std::endl;
        RTN mirRtn = RTN_FindByName(img, (*it).c_str());
        if (RTN_Valid(mirRtn))
        {/*{{{*/
            //std::cout << "Function " << *it << " is valid" << std::endl;
            RTN_Open(mirRtn);

            // For each instruction of the function, update entries in the stats counter
            for (INS ins = RTN_InsHead(mirRtn); INS_Valid(ins); ins = INS_Next(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateInsCount, IARG_END);
#ifdef GET_INS_MIX
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateInsMix, IARG_UINT32, INS_Category(ins), IARG_END);
#endif

                // Instrument stack accesses
                // If instruction operates using the SP or FP, its a stack operation
                if(INS_IsStackRead(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateStackRead, IARG_END);
                if(INS_IsStackWrite(ins))
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateStackWrite, IARG_END);

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
                                ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateMemRefRead,
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
                                ins, IPOINT_BEFORE, (AFUNPTR)MIROutlineFunctionUpdateMemRefWrite,
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

    // Task create 
    RTN mirTaskCreateRtn = RTN_FindByName(img, "mir_task_create");
    if (RTN_Valid(mirTaskCreateRtn))
    {
        RTN_Open(mirTaskCreateRtn);
        RTN_InsertCall(mirTaskCreateRtn, IPOINT_BEFORE, (AFUNPTR)MIRTaskCreateBefore, IARG_END);
        RTN_Close(mirTaskCreateRtn);
    }
    
    // Task wait
    RTN mirTaskWaitRtn = RTN_FindByName(img, "mir_task_wait");
    if (RTN_Valid(mirTaskWaitRtn))
    {
        RTN_Open(mirTaskWaitRtn);
        RTN_InsertCall(mirTaskWaitRtn, IPOINT_AFTER, (AFUNPTR)MIRTaskWaitAfter, IARG_END);
        RTN_Close(mirTaskWaitRtn);
    }
}/*}}}*/

VOID MIROutlineFunctionUpdateMemFp(MIR_FUNCTION_STAT * stat)
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

VOID MIROutlineFunctionUpdateMemShare(MIR_FUNCTION_STAT * this_stat, size_t cutoff)
{/*{{{*/
    // Mem footprint share
    // Intersection of this instance's memory footprint with all other instances within cutoff
    // Cutoff is used to limit number of intersections computed
    size_t sz = 0;
    for (MIR_FUNCTION_STAT* stat = g_stat_list; sz <= cutoff; stat = stat->next, sz++)
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

VOID Fini(INT32 code, VOID *v)
{/*{{{*/
    std::cout << "Finalizing ..." << std::endl;
    std::string filename;

    // Dump /proc/<pid>/maps
    filename.clear();
    filename = KnobOutputFileSuffix.Value();
    if( KnobPid )
        filename += "." + decstr( getpid_portable() );
    filename += "-mem-map";
    std::cout << "Writing memory map (/proc/<pid>/maps) to file: " << filename << " ..." << std::endl;
    char cmd[256];
    sprintf(cmd, "cat /proc/%d/maps > %s", getpid_portable(), filename.c_str());
    system(cmd);

    // Update instances in parallel
    std::cout << "Updating statistics in parallel ..." << std::endl;
    UINT64 num_instances = 0;
#pragma omp parallel
{
#pragma omp single
{
    std::cout << "Updating memory footprint ..." << std::endl;
    for (MIR_FUNCTION_STAT* stat = g_stat_list; stat; stat = stat->next)
    {
        num_instances++;
//#pragma omp task
        {
            // Update memory footprint
            MIROutlineFunctionUpdateMemFp(stat);
        }
    }
//#pragma omp taskwait
    if(KnobCalcMemShare)
    {
        std::cout << "Updating memory sharing ..." << std::endl;
        std::cout << "Using " << omp_get_num_threads() << " threads" << std::endl;
        size_t cutoff = 0;
        for (MIR_FUNCTION_STAT* stat = g_stat_list; stat; stat = stat->next, cutoff++)
        {
#pragma omp task
            {
                // Update memory sharing
                MIROutlineFunctionUpdateMemShare(stat, cutoff);
            }
        }
#pragma omp taskwait
    }
}
}

    // Open call graph file
    filename.clear();
    filename = KnobOutputFileSuffix.Value(); // + "." + KnobFunctionName.Value();
    if( KnobPid )
        filename += "." + decstr( getpid_portable() );
    filename += "-instructions";
    std::ofstream out;
    out.open(filename.c_str());
    std::cout << "Writing call graph information to file: " << filename << " ..." << std::endl;
    // Write as csv
    const char* fileheader = "task,ins_count,stack_read,stack_write,mem_fp,ccr,clr,mem_read,mem_write,outl_func";
#ifdef GET_INS_MIX
    string ins_catg = ",";
    for(unsigned int c=0; c<XED_CATEGORY_LAST; c++)
    {
        if(c!=0)
        {
            ins_catg += ",";
            ins_catg += xed_category_enum_t2str((const xed_category_enum_t)c);
        }
        else
            ins_catg += xed_category_enum_t2str((const xed_category_enum_t)c);
    }
    out << fileheader << ins_catg << std::endl;
#else
    out << fileheader << std::endl;
#endif
    for (MIR_FUNCTION_STAT* stat = g_stat_list; stat; stat = stat->next)
    {
        out << stat->id << "," 
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
        out << ",";
        for(int c=0; c<XED_CATEGORY_LAST; c++)
        {
            if(c!=0)
                out << "," << stat->ins_mix[c];
            else
                out << stat->ins_mix[c];
        }
#endif
        out << std::endl;
    }
    // Close file
    out.close();

    if(KnobCalcMemShare)
    {
        // Open memory share file
        filename.clear();
        filename = KnobOutputFileSuffix.Value(); // + "." + KnobFunctionNames.Value();
        if( KnobPid )
            filename += "." + decstr( getpid_portable() );
        filename += "-mem-share";
        out.open(filename.c_str());
        std::cout << "Writing memory sharing information to file: " << filename << " ..." << std::endl;
        // Write mem share per task to file
        for (MIR_FUNCTION_STAT* stat = g_stat_list; stat; stat = stat->next)
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

    // Write task create and task wait instants
    filename.clear();
    filename = KnobOutputFileSuffix.Value(); // + "." + KnobFunctionName.Value();
    if( KnobPid )
        filename += "." + decstr( getpid_portable() );
    filename += "-events";
    std::cout << "Writing task create and wait invocation information to file: " << filename << " ..." << std::endl;
    out.open(filename.c_str());
    out << "task,ins_count,[create],[wait]" << std::endl;
    for (MIR_FUNCTION_STAT* stat = g_stat_list; stat; stat = stat->next)
    {
        out << stat->id << "," << stat->ins_count << ",[";
        std::vector<UINT64>::iterator it;
        for(it = stat->task_create_instant.begin(); it != stat->task_create_instant.end(); it++)
        {
            out << *it << ",";
        }
        out << "],[";
        for(it = stat->task_wait_instant.begin(); it != stat->task_wait_instant.end(); it++)
        {
            out << *it << ",";
        }
        out << "]";
        out << std::endl;
    }
    // Close file
    out.close();
}/*}}}*/

INT32 Usage()
{/*{{{*/
    cerr << "This PIN tool profiles calls to task outlined functions. Usage:" << endl;
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
