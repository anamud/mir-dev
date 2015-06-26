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
    print("Error: Invalid arguments. Check help (-h).")
    quit("no", 1)
}

# Read data
if(parsed$verbose) print(paste("Reading file", parsed$data))
ts.data <- read.csv(parsed$data, header=TRUE)

# Process
if(parsed$timing) tic(type="elapsed")
if(parsed$verbose) print("Processing ...")

# Remove non-sense data
if(parsed$verbose) print("Removing non-sense data ...")
# Remove background task 
ts.data <- ts.data[!is.na(ts.data$parent),]

## Find task executed last per worker
if(parsed$verbose) print("Calculating last tasks to finish ...")
max.exec.end <- ts.data %>% group_by(cpu_id) %>% filter(exec_end_instant == max(exec_end_instant))
ts.data["last_to_finish"] <- F
for(task in max.exec.end$task)
{
    ts.data$last_to_finish[ts.data$task == task] <- T
}

## Mark leaf tasks
if(parsed$verbose) print("Marking leaf tasks ...")
ts.data["leaf"] <- F
ts.data$leaf[ts.data$num_children == 0] <- T

## Calc work cycles
### Work is the amount of computation by a task excluding runtime system calls.
if(parsed$verbose) print("Calculating work cycles ...")
ts.data$work_cycles <- ts.data$exec_cycles - ts.data$overhead_cycles

## Calc parallel benefit
### Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.
if(parsed$verbose) print("Calculating parallel benefit ...")
calc_parallel_benefit <- function(t)
{
    i <- which(ts.data$task == t)
    t.p <- ts.data$parent[i]
    j <- which(ts.data$task == t.p)
    k <- which(ts.data$parent == t.p)
    t.p.o.c <- ts.data$overhead_cycles[j] / length(ts.data$task[k])
    ts.data$work_cycles[i]/t.p.o.c
}
parallel_benefit <- as.numeric(sapply(ts.data$task, calc_parallel_benefit))
ts.data["parallel_benefit"] <- parallel_benefit

## Calc memory hierarchy metrics
### Memory hierarchy utilization
if("PAPI_RES_STL_sum" %in% colnames(ts.data))
{
    if(parsed$verbose) print("Calculating memory hierarchy utilization ...")
    ts.data$mem_hier_util <- ts.data$PAPI_RES_STL_sum/ts.data$work_cycles
}

### Compute intensity
if("ins_count" %in% colnames(ts.data) & "mem_fp" %in% colnames(ts.data))
{
    if(parsed$verbose) print("Calculating compute intensity ...")
    ts.data$compute_int <- ts.data$ins_count/ts.data$mem_fp
}

## Calculate lineage
if(parsed$lineage)
{
    # Lineage = child number chain
    if(parsed$verbose) print("Calculating lineage ...")
    ts.data <- ts.data[order(ts.data$task),]
    ts.data["lineage"] <- "NA"
    for(task in ts.data$task)
    {
        i <- which(ts.data$task == task)
        parent <- ts.data$parent[i]
        child_number <- ts.data$child_number[i]
        if(parent != 0)
            ts.data$lineage[i] <- paste(ts.data$lineage[which(ts.data$task == parent)], as.character(child_number), sep="-")
        else
            ts.data$lineage[i] <- paste("R", as.character(child_number), sep="-")
    }
    # Sanity check
    if(anyDuplicated(ts.data$lineage))
    {
        print("Error: Duplicate lineages exist. Aborting!")
        quit("no", 1)
    }
}
if(parsed$timing) toc("Processing")

# Write out processed data
out.file <- paste(gsub(". $", "", parsed$data), ".processed", sep="")
sink(out.file)
write.csv(ts.data, out.file, row.names=F)
sink()
if(parsed$verbose) print(paste("Wrote file:", out.file))

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

