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
                   out="loop-task-stats.processed")
} else {
    option_list <- list(
                        make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
                        make_option(c("--lineage"), action="store_true", default=FALSE, help="Calculate chunk lineage."),
                        make_option(c("--nochunks"), action="store_true", default=FALSE, help="Disable chunk processing."),
                        make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                        make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                        make_option(c("-o","--out"), default="loop-task-stats.processed", help = "Output file name [default \"%default\"]", metavar="STRING"),
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

# Remove idle task without children
task_stats <- task_stats[!(tag == "idle_task" & num_children == 0),]

# Remove chunk_start and chunk_continuation tasks
# Metada can be NA. Handle that!
#task_stats <- task_stats[!(!is.na(metadata) & metadata == "chunk_start"),]
#task_stats <- task_stats[!(!is.na(metadata) & metadata == "chunk_continuation"),]

# Find task executed last per worker
if (parsed$verbose) my_print("Calculating last tasks to finish ...")
task_stats <- task_stats %>% group_by(cpu_id) %>% mutate(last_to_finish = (exec_end_instant == max(exec_end_instant)))

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

if (!parsed$nochunks) {
    # Chunk tasks
    chunk_tasks <- which(grepl(glob2rx("chunk_*"), task_stats$metadata))
    iteration_bound_chunk_tasks <- which(grepl(glob2rx("chunk_*_*"), task_stats$metadata))

    # Calculate chunk idle join point
    task_stats <- task_stats[, idle_join := joins_at]
    task_stats <- task_stats[, parent_joins_at := joins_at[match(parent, task)]]
    task_stats <- task_stats[, parent_tag := tag[match(parent, task)]]
    task_stats <- task_stats[chunk_tasks, idle_join := ifelse(parent_tag == "idle_task", joins_at, parent_joins_at)]

    # Calculate chunk lineage
    if (parsed$lineage) {
        if (parsed$verbose) my_print("Calculating chunk lineage ...")

        # For-loop graph shapes are run dependent since chunk tasks can be created by different threads across runs.
        # Therefore, lineage of chunk tasks cannot be calculated by tracing their path from the root task.

        # In a for-loop task graph, chunk tasks at the same depth with respect to the idle task
        # belong to the same for-loop instance.
        # Chunk tasks of a for-loop instance have the same join point, but not the same parent.
        # Chunks can be ordered by iteration start number.
        # Lineage is therefore calculated by concatenating joins_at with respect to the idle task
        # and iteration start number.

        task_stats <- task_stats[, chunk_lineage := "NA"]
        task_stats <- task_stats[iteration_bound_chunk_tasks, chunk_lineage := paste(idle_join, as.character(sapply(metadata, function(x) {unlist(strsplit(x, "_"))[2]})), sep="-")]

        # Check for duplicates
        if (anyDuplicated(task_stats$chunk_lineage, incomparables="NA")) {
            my_print("Error: Duplicate chunk lineages exist. Aborting!")
            quit("no", 1)
        }
    }

    if (parsed$verbose) my_print("Calculating chunk work balance statistics ...")

    # Subset
    task_stats_only_chunks <- task_stats[chunk_tasks,]

    # Calculate chunk work balance
    task_stats_only_chunks <- task_stats_only_chunks %>% group_by(idle_join) %>% mutate(chunk_work_balance = max(work_cycles)/mean(work_cycles))

    # Calculate chunk work per cpu
    task_stats_only_chunks <- task_stats_only_chunks %>% group_by(idle_join, cpu_id) %>% mutate(chunk_work_cpu = sum(work_cycles))

    # Calculate chunk per cpu
    task_stats_only_chunks <- task_stats_only_chunks %>% group_by(idle_join) %>% mutate(chunk_work_cpu_balance = max(chunk_work_cpu)/mean(unique(chunk_work_cpu)))

    # Merge back
    task_stats_only_chunks <- subset(task_stats_only_chunks, select=c(task, chunk_work_balance, chunk_work_cpu, chunk_work_cpu_balance))
    task_stats <- merge(task_stats, task_stats_only_chunks, by="task", all=T, suffixes=c("_left", "_right"))
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

# Stop processing
if (parsed$timing) toc("Processing")

# Write out processed data
out_file <- parsed$out
sink(out_file)
write.csv(task_stats, out_file, row.names=F)
sink()
my_print(paste("Wrote file:", out_file))

# Print warnings
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)
