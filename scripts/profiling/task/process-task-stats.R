# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Read arguments
option_list <- list(
make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
make_option(c("--lineage"), action="store_true", default=FALSE, help="Calculate task lineage."),
make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("data", where=parsed))
{
    my_print("Error: Invalid arguments. Check help (-h).")
    quit("no", 1)
}

# Read data
if(parsed$verbose) my_print(paste("Reading file", parsed$data))
ts_data <- read.csv(parsed$data, header=TRUE)

# Process
if(parsed$timing) tic(type="elapsed")
if(parsed$verbose) my_print("Processing ...")

# Remove non-sense data
if(parsed$verbose) my_print("Removing non-sense data ...")
# Remove background task 
ts_data <- ts_data[!is.na(ts_data$parent),]

## Find task executed last per worker
if(parsed$verbose) my_print("Calculating last tasks to finish ...")
max_exec_end <- ts_data %>% group_by(cpu_id) %>% filter(exec_end_instant == max(exec_end_instant))
ts_data["last_to_finish"] <- F
for(task in max_exec_end$task)
{
    ts_data$last_to_finish[ts_data$task == task] <- T
}

## Mark leaf tasks
if(parsed$verbose) my_print("Marking leaf tasks ...")
ts_data["leaf"] <- F
ts_data$leaf[ts_data$num_children == 0] <- T

## Calc work cycles
### Work is the amount of computation by a task excluding runtime system calls.
if(parsed$verbose) my_print("Calculating work cycles ...")
ts_data$work_cycles <- ts_data$exec_cycles - ts_data$overhead_cycles

## Calc parallel benefit
### Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.
if(parsed$verbose) my_print("Calculating parallel benefit ...")
calc_parallel_benefit <- function(t)
{
    i <- which(ts_data$task == t)
    t_p <- ts_data$parent[i]
    j <- which(ts_data$task == t_p)
    k <- which(ts_data$parent == t_p)
    t_p_o_c <- ts_data$overhead_cycles[j] / length(ts_data$task[k])
    ts_data$work_cycles[i]/t_p_o_c
}
parallel_benefit <- as.numeric(sapply(ts_data$task, calc_parallel_benefit))
ts_data["parallel_benefit"] <- parallel_benefit

## Calc memory hierarchy metrics
### Memory hierarchy utilization
if("PAPI_RES_STL_sum" %in% colnames(ts_data))
{
    if(parsed$verbose) my_print("Calculating memory hierarchy utilization ...")
    ts_data$mem_hier_util <- ts_data$PAPI_RES_STL_sum/ts_data$work_cycles
}

### Compute intensity
if("ins_count" %in% colnames(ts_data) & "mem_fp" %in% colnames(ts_data))
{
    if(parsed$verbose) my_print("Calculating compute intensity ...")
    ts_data$compute_int <- ts_data$ins_count/ts_data$mem_fp
}

## Calculate lineage
if(parsed$lineage)
{
    # Lineage = child number chain
    if(parsed$verbose) my_print("Calculating lineage ...")
    ts_data <- ts_data[order(ts_data$task),]
    ts_data["lineage"] <- "NA"
    for(task in ts_data$task)
    {
        i <- which(ts_data$task == task)
        parent <- ts_data$parent[i]
        child_number <- ts_data$child_number[i]
        if(parent != 0)
            ts_data$lineage[i] <- paste(ts_data$lineage[which(ts_data$task == parent)], as.character(child_number), sep="-")
        else
            ts_data$lineage[i] <- paste("R", as.character(child_number), sep="-")
    }
    # Sanity check
    if(anyDuplicated(ts_data$lineage))
    {
        my_print("Error: Duplicate lineages exist. Aborting!")
        quit("no", 1)
    }
}
if(parsed$timing) toc("Processing")

# Write out processed data
out_file <- paste(gsub(". $", "", parsed$data), ".processed", sep="")
sink(out_file)
write.csv(ts_data, out_file, row.names=F)
sink()
my_print(paste("Wrote file:", out_file))

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    my_print(wa)

