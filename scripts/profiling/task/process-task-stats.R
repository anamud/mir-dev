# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Read arguments
option_list <- list(
make_option(c("--lineage"), action="store_true", default=FALSE, help="Calculate task lineage."),
make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
make_option(c("--compare"), help = "Task stats for comparison.", metavar="FILE"),
make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."),
make_option(c("--summary"), action="store_true", default=TRUE, help="Summarize [default]."),
make_option(c("--no-summary"), action="store_false", dest="summary", help="Do not summarize."))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("data", where=parsed))
{
    print("Error: Invalid arguments. Check help (-h).")
    quit("no", 1)
}
ts.file <- parsed$data
verbo <- parsed$verbose
sumry <- parsed$summary
calc_lineage <- parsed$lineage
if(exists("compare", where=parsed)) ts.compare.file <- parsed$compare

# Read data
if(verbo) print(paste("Reading file", ts.file))
ts.data <- read.csv(ts.file, header=TRUE)

# Process
if(verbo) tic(type="elapsed")
if(verbo) print("Processing ...")

## Find task executed last per worker
if(verbo) print("Calculating last tasks to finish ...")
max.exec.end <- ts.data %>% group_by(cpu_id) %>% filter(exec_end == max(exec_end))
ts.data["last_to_finish"] <- F
for(task in max.exec.end$task)
{
    ts.data$last_to_finish[ts.data$task == task] <- T
}

## Mark leaf tasks
if(verbo) print("Marking leaf tasks ...")
ts.data["leaf"] <- F
ts.data$leaf[ts.data$num_children == 0] <- T

## Calc work cycles
### Work is the amount of computation by a task excluding runtime system calls.
if(verbo) print("Calculating work cycles ...")
ts.data$work_cycles <- ts.data$exec_cycles - ts.data$overhead_cycles

## Calc parallel benefit
### Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.
if(verbo) print("Calculating parallel benefit ...")
calc_parallel_benefit <- function(t)
{
    t.p <- ts.data$parent[ts.data$task == t]
    t.p.o <- ts.data$overhead_cycles[ts.data$task == t.p]
    t.p.nc <- length(ts.data$task[ts.data$parent == t.p])
    t.p.o.c <- t.p.o/t.p.nc
    ts.data$work_cycles[ts.data$task == t]/t.p.o.c
}
parallel_benefit <- as.numeric(sapply(ts.data$task, calc_parallel_benefit))
ts.data["parallel_benefit"] <- parallel_benefit

## Calc memory hierarchy metrics
### Memory hierarchy utilization
if("PAPI_RES_STL_sum" %in% colnames(ts.data))
{
    if(verbo) print("Calculating memory hierarchy utilization ...")
    ts.data$mem_hier_util <- ts.data$PAPI_RES_STL_sum/ts.data$work_cycles
}

### Compute intensity
if("ins_count" %in% colnames(ts.data) & "mem_fp" %in% colnames(ts.data))
{
    if(verbo) print("Calculating compute intensity ...")
    ts.data$compute_int <- ts.data$ins_count/ts.data$mem_fp
}

## Calculate lineage
if(calc_lineage)
{
    # Lineage = child number chain
    if(verbo) print("Calculating lineage ...")
    ts.data.sub <- subset(ts.data, select=c("task", "parent", "child_number"))
    ts.data.sub <- ts.data.sub[order(ts.data.sub$task),]
    ts.data.sub["lineage"] <- "NA"
    for(task in ts.data.sub$task)
    {
        parent <- ts.data.sub$parent[ts.data.sub$task == task]
        child_number <- ts.data.sub$child_number[ts.data.sub$task == task]
        if(parent != 0)
            ts.data.sub$lineage[ts.data.sub$task == task] <- paste(ts.data.sub$lineage[ts.data.sub$task == parent], as.character(child_number), sep="-")
        else
            ts.data.sub$lineage[ts.data.sub$task == task] <- paste("R", as.character(child_number), sep="-")
    }
    # Sanity check
    if(anyDuplicated(ts.data.sub$lineage))
    {
        print("Error: Duplicate lineages exist. Aborting!")
        quit("no", 1)
    }
    # Write out
    out.file <- paste(gsub(". $", "", ts.file), ".lineage", sep="")
    write.csv(ts.data.sub, out.file, row.names=F)
    if(verbo) print(paste("Wrote file:", out.file))
}

if(exists("compare", where=parsed)) 
{
    if(verbo) print("Comparing task stats ...")
    # Read comparison task stats
    ts.comp.data <- read.csv(ts.compare.file, header=TRUE)
    # Subset and merge work deviation data
    ts.comp.data <- subset(ts.comp.data, select=c(lineage,work_cycles,overhead_cycles))
    ts.data <- merge(ts.data, ts.comp.data, by="lineage", suffixes=c("",".1"))
    # Calculate deviation
    ts.data$work_deviation <- ts.data$work_cycles/ts.data$work_cycles.1
    ts.data$overhead_deviation <- ts.data$overhead_cycles/ts.data$overhead_cycles.1
}
if(verbo) toc("Processing")

# Write out processed data
out.file <- paste(gsub(". $", "", ts.file), ".processed", sep="")
sink(out.file)
write.csv(ts.data, out.file, row.names=F)
sink()
if(verbo) print(paste("Wrote file:", out.file))

# Summarize
if(sumry)
{
    if(verbo) tic(type="elapsed")
    out.file <- paste(gsub(". $", "", ts.file), ".info", sep="")
    sink(out.file)

    if(verbo) print("Summarizing all tasks ...")
    summarize_task_stats(ts.data, "All tasks")

    if(verbo) print("Summarizing leaf tasks ...")
    ts.data.leaf <- subset(ts.data, leaf == T)
    summarize_task_stats(ts.data.leaf, "Leaf tasks")

    if(verbo) print("Summarizing non-leaf tasks ...")
    ts.data.non.leaf <- subset(ts.data, leaf == F)
    summarize_task_stats(ts.data.non.leaf, "Non-leaf tasks")

    sink()
    if(verbo) print(paste("Wrote file:", out.file))
    if(verbo) toc("Summarizing")
}

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

