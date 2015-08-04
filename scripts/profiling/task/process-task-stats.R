# Clear workspace
rm(list=ls())

# Import
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Read arguments
option_list <- list(
                    make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
                    make_option(c("--lineage"), action="store_true", default=FALSE, help="Calculate task lineage."),
                    make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                    make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                    make_option(c("-o","--out"), default="task-stats.processed", help = "Output file name [default \"%default\"]", metavar="STRING"),
                    make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))

parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

if (!exists("data", where=parsed)) {
    my_print("Error: Invalid arguments. Check help (-h).")
    quit("no", 1)
}

# Read data
if (parsed$verbose) my_print(paste("Reading file", parsed$data))
task_stats <- read.csv(parsed$data, header=TRUE)

# Start processing
if (parsed$timing) tic(type="elapsed")
if (parsed$verbose) my_print("Processing ...")

# Remove non-sense data
if (parsed$verbose) my_print("Removing non-sense data ...")

# Remove background task
task_stats <- task_stats[!is.na(task_stats$parent),]

# Find task executed last per worker
if (parsed$verbose) my_print("Calculating last tasks to finish ...")
max_exec_end <- task_stats %>% group_by(cpu_id) %>% filter(exec_end_instant == max(exec_end_instant))
task_stats["last_to_finish"] <- F
for(task in max_exec_end$task) {
    task_stats$last_to_finish[task_stats$task == task] <- T
}

# Mark leaf tasks
if (parsed$verbose) my_print("Marking leaf tasks ...")
task_stats["leaf"] <- F
task_stats$leaf[task_stats$num_children == 0] <- T

# Calculate work cycles
# Work is the amount of computation by a task excluding runtime system calls.
if (parsed$verbose) my_print("Calculating work cycles ...")
task_stats$work_cycles <- task_stats$exec_cycles - task_stats$overhead_cycles

# Calculate parallel benefit
# Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.
if (parsed$verbose) my_print("Calculating parallel benefit ...")

calc_parallel_benefit <- function(task)
{
    task_ind <- which(task_stats$task == task)
    parent <- task_stats$parent[task_ind]
    parent_ind <- which(task_stats$task == parent)
    sibling_ind <- which(task_stats$parent == parent)
    sync_overhead_per_child <- (task_stats$overhead_cycles[parent_ind] - sum(as.numeric(task_stats$creation_cycles[sibling_ind]))) / length(task_stats$task[sibling_ind])
    task_stats$work_cycles[task_ind]/(task_stats$creation_cycles[task_ind] + sync_overhead_per_child)
}
parallel_benefit <- as.numeric(sapply(task_stats$task, calc_parallel_benefit))
task_stats["parallel_benefit"] <- parallel_benefit

# Calculate memory hierarchy utilization
if ("PAPI_RES_STL_sum" %in% colnames(task_stats)) {
    if (parsed$verbose) my_print("Calculating memory hierarchy utilization ...")
    task_stats$mem_hier_util <- task_stats$PAPI_RES_STL_sum/task_stats$work_cycles
}

# Calculate compute intensity
if ("ins_count" %in% colnames(task_stats) & "mem_fp" %in% colnames(task_stats)) {
    if (parsed$verbose) my_print("Calculating compute intensity ...")
    task_stats$compute_int <- task_stats$ins_count/task_stats$mem_fp
}

# Calculate sibling work balance
if (parsed$verbose) my_print("Calculating sibling work balance ...")
task_stats <- task_stats %>% group_by(parent,joins_at) %>% mutate(sibling_work_balance = max(work_cycles)/mean(work_cycles))

# Calculate lineage
if (parsed$lineage) {
    # Lineage = child number chain
    if (parsed$verbose) my_print("Calculating lineage ...")
    task_stats <- task_stats[order(task_stats$task),]
    task_stats["lineage"] <- "NA"
    for(task in task_stats$task) {
        i <- which(task_stats$task == task)
        parent <- task_stats$parent[i]
        child_number <- task_stats$child_number[i]
        if (parent != 0)
            task_stats$lineage[i] <- paste(task_stats$lineage[which(task_stats$task == parent)], as.character(child_number), sep="-")
        else
            task_stats$lineage[i] <- paste("R", as.character(child_number), sep="-")
    }

    # Sanity check
    if (anyDuplicated(task_stats$lineage, incomparables="NA")) {
        my_print("Error: Duplicate lineages exist. Aborting!")
        quit("no", 1)
    }
}

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

