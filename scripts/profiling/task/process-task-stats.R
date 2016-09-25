# Clear workspace
rm(list=ls())

# Strings as factors
options(stringsAsFactors = F)

# Import
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
library(optparse, quietly=TRUE)
suppressMessages(library(data.table))
suppressMessages(library(dplyr))

# Read arguments
Rstudio_mode <- F
if (Rstudio_mode) {
    parsed <- list(data="mir-task-stats",
                   lineage=T,
                   verbose=T,
                   timing=F,
                   out="task-stats.processed")
} else {
    option_list <- list(
                        make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
                        make_option(c("-e","--executable"), help = "The executable that generated the stats", metavar="EXECUTABLE"),
                        make_option(c("--lineage"), action="store_true", default=FALSE, help="Calculate task lineage."),
                        make_option(c("--linenumbers"), action="store_true", default=FALSE, help="Find the source line number for each task."),
                        make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                        make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                        make_option(c("-o","--out"), default="task-stats.processed", help = "Output file name [default \"%default\"]", metavar="STRING"),
                        make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))

    parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

    if (!exists("data", where=parsed)) {
        my_print("Error: Invalid arguments. Check help (-h).")
        quit("no", 1)
    }
}

# Read data
if (parsed$verbose) my_print(paste("Reading file", parsed$data))
task_stats <- read.csv(parsed$data, header=TRUE)
setDT(task_stats)
setkey(task_stats, task, parent)

# Start processing
if (parsed$timing) tic(type="elapsed")
if (parsed$verbose) my_print("Processing ...")

# Remove non-sense data
if (parsed$verbose) my_print("Removing non-sense data ...")

# Remove background task
task_stats <- task_stats[!is.na(parent),]

# Mark leaf tasks
if (parsed$verbose) my_print("Marking leaf tasks ...")
task_stats <- task_stats[, leaf := F]
task_stats <- task_stats[which(num_children == 0), leaf := T]

# Calculate work cycles
# Work is the amount of computation by a task excluding runtime system calls.
if (parsed$verbose) my_print("Calculating work cycles ...")
task_stats <- task_stats[, work_cycles := exec_cycles - overhead_cycles]

# Calculate memory hierarchy utilization
if ("PAPI_RES_STL_sum" %in% colnames(task_stats)) {
    if (parsed$verbose) my_print("Calculating memory hierarchy utilization ...")
    task_stats <- task_stats[, mem_hier_util := PAPI_RES_STL_sum / work_cycles]
}

# Calculate compute intensity
if ("ins_count" %in% colnames(task_stats) & "mem_fp" %in% colnames(task_stats)) {
    if (parsed$verbose) my_print("Calculating compute intensity ...")
    task_stats <- task_stats[, compute_int := ins_count / mem_fp]
}

# Calculate lineage
if (parsed$lineage) {
    # Lineage = child number chain
    if (parsed$verbose) my_print("Calculating lineage ...")
    task_stats <- task_stats[order(task_stats$task),]

    #Rprof("test.prof", line.profiling=TRUE)
    task_stats[, parent_ind := match(task_stats$parent, task_stats$task)]
    for(i in seq(1:nrow(task_stats))) {
        parent <- task_stats$parent[i]
        child_number <- task_stats$child_number[i]
        if (parent != 0) {
            parent_lineage <- task_stats$lineage[task_stats$parent_ind[i]]
            task_stats[i, lineage := paste(parent_lineage, as.character(child_number), sep="-")]
        }
        else
            task_stats[i, lineage := paste("R", as.character(child_number), sep="-")]
    }
    #Rprof(NULL)
    #summaryRprof("test.prof", lines = "both")

    # Sanity check
    if (anyDuplicated(task_stats$lineage, incomparables="NA")) {
        my_print("Error: Duplicate lineages exist. Aborting!")
        quit("no", 1)
    }
}

# Input: outline function address as a number (double)
find_line_number <- function(outline_func_addr) {
    # The existence of addr2line is assumed
    system(paste("addr2line -s -e", parsed$executable, sprintf("%x", outline_func_addr)), intern=TRUE)
}

# Find line numbers
if (parsed$linenumbers) {
    if (!exists("executable", where=parsed)) {
        my_print(paste(
            "Warning: Cannot find line numbers when the profiled executable has not been specified!",
            "Specify it using the -e option", sep="\n         "))
    }
    else {
        if (parsed$verbose) my_print("Finding line numbers ...")
        task_stats <- task_stats[, source_line := sapply(outline_function, find_line_number)] 
    }
}

# Calculate parallel benefit
if (parsed$verbose) my_print("Calculating parallel benefit ...")
task_stats$parent_overhead_cycles <- task_stats$overhead_cycles[match(task_stats$parent, task_stats$task)]
task_stats <- task_stats %>% group_by(parent) %>% mutate(sync_cycles_per_child = (parent_overhead_cycles - sum(creation_cycles))/n())
task_stats <- task_stats %>% rowwise() %>% mutate(parallel_benefit = work_cycles/(creation_cycles + sync_cycles_per_child))
task_stats <- ungroup(task_stats)

# Calculate sibling work balance
if (parsed$verbose) my_print("Calculating sibling work balance ...")
task_stats <- task_stats %>% group_by(parent,joins_at) %>% mutate(sibling_work_balance = max(work_cycles)/mean(work_cycles))

# Calculate sibling scatter
if (parsed$verbose) my_print("Calculating scatter ...")
task_stats <- task_stats %>% group_by(parent,joins_at) %>% mutate(sibling_scatter = median(c(dist(cpu_id))))

# Find task executed last per worker
if (parsed$verbose) my_print("Calculating last tasks to finish ...")
task_stats <- task_stats %>% group_by(cpu_id) %>% mutate(last_to_finish = (exec_end_instant == max(exec_end_instant)))

# Stop processing
if (parsed$timing) toc("Processing")

# Write out processed data
out_file <- parsed$out
sink(out_file)
write.csv(task_stats, out_file, row.names=F)
sink()
my_print(paste("Wrote file:", out_file))

# Warn
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)

