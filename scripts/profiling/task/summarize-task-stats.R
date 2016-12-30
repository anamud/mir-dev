# Clear workspace
rm(list=ls())

# Strings as factors
options(stringsAsFactors = F)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
suppressMessages(library(pastecs))
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Read arguments
Rstudio_mode <- F
if (Rstudio_mode) {
    parsed <- list(data="task-stats.processed",
                   extend=F,
                   forloop=F,
                   verbose=T,
                   timing=F)
} else {
    option_list <- list(
                        make_option(c("-d","--data"), help = "Processed task profiling data.", metavar="FILE"),
                        make_option(c("--extend"), action="store_true", default=FALSE, help="Extensive summary."),
                        make_option(c("--forloop"), action="store_true", default=FALSE, help="Profiling data obtained from for-loop program."),
                        make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                        make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                        make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))

    parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

    if (!exists("data", where=parsed)) {
        my_print("Error: Invalid arguments. Check help (-h).")
        quit("no", 1)
    }
}

# Read data
if (parsed$verbose) my_print(paste("Reading file", parsed$data))
task_stats <- as_data_frame(read.csv(parsed$data, header=TRUE))

# Remove background task
task_stats <- task_stats[!is.na(task_stats$parent),]
#task_stats <- task_stats[!task_stats$parent==0,]

if (parsed$forloop) {
    # Remove idle task without children
    task_stats <- task_stats[!(task_stats$outline_function == "idle_task" & task_stats$num_children == 0),]

    # Remove chunk_start and chunk_continuation tasks
    # Metada can be NA. Handle that!
    #task_stats <- task_stats[!(!is.na(task_stats$metadata) & task_stats$metadata == "chunk_start"),]
    #task_stats <- task_stats[!(!is.na(task_stats$metadata) & task_stats$metadata == "chunk_continuation"),]
}

# Summary helper function
summarize_task_stats <- function(df)
{
    # Number of tasks
    my_print("# Number of tasks:")
    my_print(length(df$task))
    my_print()

    # Task count by outline function
    count_outline_func <- task_stats %>% group_by(outline_function) %>% summarise(count = n())
    my_print("# Task count by outline function")
    print.data.frame(count_outline_func, row.names=F)
    my_print()

    # Task siblings
    join.freq <- df %>% group_by(parent, joins_at) %>% summarise(count = n())
    my_print("# Task siblings summary:")
    my_print("# Task siblings share the same parent and join point.")
    print(stat.desc(as.numeric(join.freq$count)), row.names=F)
    my_print()

    # Work
    if ("work_cycles" %in% colnames(task_stats)) {# {{{
        my_print("# Work:")
        my_print("# Work is the amount of computation performed by a task excluding runtime system calls.")

        # Summary
        my_print("Work summary:")
        print(stat.desc(as.numeric(task_stats$work_cycles)), row.names=F)
        my_print()

        # Work by core
        work_core <- task_stats %>% group_by(cpu_id) %>% summarize(work_total = sum(as.numeric(work_cycles)),
                                                                   work_mean = mean(as.numeric(work_cycles), na.rm=T),
                                                                   work_sd = sd(as.numeric(work_cycles), na.rm=T),
                                                                   work_median = median(as.numeric(work_cycles), na.rm=T))
        my_print("Work by core:")
        print.data.frame(work_core, row.names=F)
        my_print()

        # Work by outline function
        work_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(work_total = sum(as.numeric(work_cycles)),
                                                                   work_mean = mean(as.numeric(work_cycles), na.rm=T),
                                                                   work_sd = sd(as.numeric(work_cycles), na.rm=T),
                                                                   work_median = median(as.numeric(work_cycles), na.rm=T))
        my_print("Work by outline_function:")
        print.data.frame(work_outline_function, row.names=F)
        my_print()
    }# }}}

    # Overhead
    if ("overhead_cycles" %in% colnames(task_stats)) {# {{{
        my_print("# Overhead:")
        my_print("# Overhead is the amount of computation done by a task inside runtime system calls.")

        # Summary
        my_print("Overhead summary:")
        print(stat.desc(as.numeric(task_stats$overhead_cycles)), row.names=F)
        my_print()

        if ("work_cycles" %in% colnames(task_stats)) {
            total_ovh <- sum(as.numeric(task_stats$overhead_cycles))
            if (total_ovh > 0)
                total_work <- sum(as.numeric(task_stats$work_cycles))
            my_print(paste("Total work/total parallel overhead = ", total_work/total_ovh))
            my_print()
        }

        # Overhead by core
        overhead_core <- task_stats %>% group_by(cpu_id) %>% summarize(overhead_total = sum(as.numeric(overhead_cycles)),
                                                                   overhead_mean = mean(as.numeric(overhead_cycles), na.rm=T),
                                                                   overhead_sd = sd(as.numeric(overhead_cycles), na.rm=T),
                                                                   overhead_median = median(as.numeric(overhead_cycles), na.rm=T))
        my_print("overhead by core:")
        print.data.frame(overhead_core, row.names=F)
        my_print()

        # Overhead by outline function
        overhead_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(overhead_total = sum(as.numeric(overhead_cycles)),
                                                                   overhead_mean = mean(as.numeric(overhead_cycles), na.rm=T),
                                                                   overhead_sd = sd(as.numeric(overhead_cycles), na.rm=T),
                                                                   overhead_median = median(as.numeric(overhead_cycles), na.rm=T))
        my_print("Overhead by outline_function:")
        print.data.frame(overhead_outline_function, row.names=F)
        my_print()
    }# }}}

    # Parallelization benefit
    if ("parallel_benefit" %in% colnames(task_stats)) {# {{{
        # Remove wrapper task
        task_stats_temp <- task_stats[!task_stats$parent==0,]

        my_print("# Parallelization benefit:")
        my_print("# Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.")

        # Summary
        my_print("Parallel benefit summary:")
        print(stat.desc(as.numeric(task_stats_temp$parallel_benefit)), row.names=F)
        my_print()

        # Parallel_benefit by outline function
        parallel_benefit_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(parallel_benefit_mean = mean(as.numeric(parallel_benefit), na.rm=T),
                                                                   parallel_benefit_sd = sd(as.numeric(parallel_benefit), na.rm=T),
                                                                   parallel_benefit_median = median(as.numeric(parallel_benefit), na.rm=T))
        my_print("Parallel benefit by outline_function:")
        print.data.frame(parallel_benefit_outline_function, row.names=F)
        my_print()
    }# }}}

    # Last tasks to finish
    if ("last_to_finish" %in% colnames(task_stats)) {# {{{
        last_to_finish <- task_stats %>% filter(last_to_finish == T) %>% group_by(cpu_id) %>% select(task, exec_end_instant)
        my_print("# Last tasks to finish:")
        print.data.frame(last_to_finish, row.names=F)
        my_print()
    }# }}}

    # Deviation
    if ("work_deviation" %in% colnames(task_stats)) {# {{{
        my_print("# Work deviation:")
        my_print("# Work deviation is the ratio of task work under different run scenarios. Example: Single-thread and multi-thread runs.")

        # Summary
        my_print("Work deviation summary:")
        print(stat.desc(as.numeric(task_stats$work_deviation)), row.names=F)
        my_print()

        # Work deviation by core
        work_deviation_core <- task_stats %>% group_by(cpu_id) %>% summarize(work_deviation_mean = mean(as.numeric(work_deviation), na.rm=T),
                                                                   work_deviation_sd = sd(as.numeric(work_deviation), na.rm=T),
                                                                   work_deviation_median = median(as.numeric(work_deviation), na.rm=T))
        my_print("work_deviation by core:")
        print.data.frame(work_deviation_core, row.names=F)
        my_print()

        # Work deviation by outline function
        work_deviation_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(work_deviation_mean = mean(as.numeric(work_deviation), na.rm=T),
                                                                   work_deviation_sd = sd(as.numeric(work_deviation), na.rm=T),
                                                                   work_deviation_median = median(as.numeric(work_deviation), na.rm=T))
        my_print("work_deviation by outline_function:")
        print.data.frame(work_deviation_outline_function, row.names=F)
        my_print()
    }# }}}

    # PAPI_RES_STL related
    if ("PAPI_RES_STL_sum" %in% colnames(task_stats) & "work_cycles" %in% colnames(task_stats)) {# {{{
        task_stats$work_PAPI_RES_STL <- task_stats$work_cycles/task_stats$PAPI_RES_STL_sum

        my_print("# Work to PAPI_RES_STL ratio:")

        # Summary
        my_print("Work to PAPI_RES_STL ratio summary:")
        print(stat.desc(as.numeric(task_stats$work_PAPI_RES_STL)), row.names=F)
        my_print()

        # Work per PAPI_RES_STL by core
        work_PAPI_RES_STL_cpu_id <- task_stats %>% group_by(cpu_id) %>% summarize(work_PAPI_RES_STL_mean = mean(as.numeric(work_PAPI_RES_STL), na.rm=T),
                                                                   work_PAPI_RES_STL_sd = sd(as.numeric(work_PAPI_RES_STL), na.rm=T),
                                                                   work_PAPI_RES_STL_median = median(as.numeric(work_PAPI_RES_STL), na.rm=T))
        my_print("Work per PAPI_RES_STL cycle by core:")
        print.data.frame(work_PAPI_RES_STL_cpu_id, row.names=F)
        my_print()

        # Work per PAPI_RES_STL by outline function
        work_PAPI_RES_STL_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(work_PAPI_RES_STL_mean = mean(as.numeric(work_PAPI_RES_STL), na.rm=T),
                                                                   work_PAPI_RES_STL_sd = sd(as.numeric(work_PAPI_RES_STL), na.rm=T),
                                                                   work_PAPI_RES_STL_median = median(as.numeric(work_PAPI_RES_STL), na.rm=T))
        my_print("Work per PAPI_RES_STL cycle by outline_function:")
        print.data.frame(work_PAPI_RES_STL_outline_function, row.names=F)
        my_print()
    }# }}}

    # Memory hierarchy utilization (MHU)
    if ("mem_hier_util" %in% colnames(task_stats)) {# {{{
        my_print("# Memory hierarchy utilization (PAPI_RES_STL to work ratio):")

        # Summary
        my_print("Memory hierarchy utilization summary:")
        print(stat.desc(as.numeric(task_stats$mem_hier_util)), row.names=F)
        my_print()

        # MHU by core
        mem_hier_util_cpu_id <- task_stats %>% group_by(cpu_id) %>% summarize(mem_hier_util_mean = mean(as.numeric(mem_hier_util), na.rm=T),
                                                                   mem_hier_util_sd = sd(as.numeric(mem_hier_util), na.rm=T),
                                                                   mem_hier_util_median = median(as.numeric(mem_hier_util), na.rm=T))
        my_print("Memory hierarchy utilization by core:")
        print.data.frame(mem_hier_util_cpu_id, row.names=F)
        my_print()

        # MHU by outline function
        mem_hier_util_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(mem_hier_util_mean = mean(as.numeric(mem_hier_util), na.rm=T),
                                                                   mem_hier_util_sd = sd(as.numeric(mem_hier_util), na.rm=T),
                                                                   mem_hier_util_median = median(as.numeric(mem_hier_util), na.rm=T))
        my_print("Memory hierarchy utilization by outline_function:")
        print.data.frame(mem_hier_util_outline_function, row.names=F)
        my_print()
    }# }}}

    # Compute intensity
    if ("compute_int" %in% colnames(task_stats)) {# {{{
        my_print("# Compute intensity (instruction count/memory footprint):")

        # Compute intensity summary
        my_print("Compute intensity summary:")
        print(stat.desc(as.numeric(task_stats$compute_int)), row.names=F)
        my_print()

        # Compute intensity by core
        compute_int_cpu_id <- task_stats %>% group_by(cpu_id) %>% summarize(compute_int_mean = mean(as.numeric(compute_int), na.rm=T),
                                                                   compute_int_sd = sd(as.numeric(compute_int), na.rm=T),
                                                                   compute_int_median = median(as.numeric(compute_int), na.rm=T))
        my_print("Compute intensity by core:")
        print.data.frame(compute_int_cpu_id, row.names=F)
        my_print()

        # Compute intensity by outline function
        compute_int_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(compute_int_mean = mean(as.numeric(compute_int), na.rm=T),
                                                                   compute_int_sd = sd(as.numeric(compute_int), na.rm=T),
                                                                   compute_int_median = median(as.numeric(compute_int), na.rm=T))
        my_print("Compute intensity by outline_function:")
        print.data.frame(compute_int_outline_function, row.names=F)
        my_print()
    }# }}}

    # Sibling work balance
    if ("sibling_work_balance" %in% colnames(task_stats)) {# {{{
        my_print("# Sibling work balance (max(work_cycles)/mean(work_cycles)):")

        # Summary
        my_print("Sibling work balance summary:")
        print(stat.desc(as.numeric(task_stats$sibling_work_balance)), row.names=F)
        my_print()

        # Mean sibling work balance by outline function
        # Sibling work balance by outline function
        sibling_work_balance_outline_function <- task_stats %>% group_by(outline_function) %>% summarize(sibling_work_balance_mean = mean(as.numeric(sibling_work_balance), na.rm=T),
                                                                   sibling_work_balance_sd = sd(as.numeric(sibling_work_balance), na.rm=T),
                                                                   sibling_work_balance_median = median(as.numeric(sibling_work_balance), na.rm=T))
        my_print("Sibling work balance by outline_function:")
        print.data.frame(sibling_work_balance_outline_function, row.names=F)
        my_print()
    }# }}}

    if (!parsed$forloop) {
        # Sibling scatter
        if ("sibling_scatter" %in% colnames(task_stats)) {# {{{
            my_print("# Sibling scatter:")

            # Summary
            my_print("Sibling scatter summary:")
            print(stat.desc(as.numeric(task_stats$sibling_scatter)), row.names=F)
            my_print()
        }# }}}
    } else {
        # Chunks
        if ("idle_join" %in% colnames(task_stats)) {# {{{
            # Summary
            my_print("# Chunks summary:")

            chunk_tasks <- which(grepl(glob2rx("chunk_*_*"), task_stats$metadata))
            task_stats_only_chunks <- task_stats[chunk_tasks, ]

            chunk_join_freq <- task_stats_only_chunks %>% group_by(outline_function, idle_join) %>% summarise(count = n())
            print.data.frame(chunk_join_freq, row.names=F)
            print(stat.desc(as.numeric(chunk_join_freq$count)), row.names=F)
            my_print()

            # Chunk parallel benefit
            if ("parallel_benefit" %in% colnames(task_stats_only_chunks)) {
                my_print("# Chunks parallel benefit:")

                chunk_parallel_benefit <- task_stats_only_chunks %>% group_by(outline_function, idle_join) %>% summarise(parallel_benefit_median = median(parallel_benefit, na.rm=T), parallel_benefit_mean = mean(parallel_benefit, na.rm=T), parallel_benefit_sd = sd(parallel_benefit, na.rm=T))
                print.data.frame(chunk_parallel_benefit, row.names=F)
                my_print()

                chunk_parallel_benefit <- task_stats_only_chunks %>% group_by(outline_function) %>% summarise(parallel_benefit_median = median(parallel_benefit, na.rm=T), parallel_benefit_mean = mean(parallel_benefit, na.rm=T), parallel_benefit_sd = sd(parallel_benefit, na.rm=T))
                print.data.frame(chunk_parallel_benefit, row.names=F)
                my_print()
            }

            # Chunk memory hierarchy utilization (MHU)
            if ("mem_hier_util" %in% colnames(task_stats_only_chunks)) {
                my_print("# Chunks memory hierarchy utilization (MHU):")

                chunk_MHU <- task_stats_only_chunks %>% group_by(outline_function, idle_join) %>% summarise(MHU_median = median(mem_hier_util, na.rm=T), MHU_mean = mean(mem_hier_util, na.rm=T), MHU_sd = sd(mem_hier_util, na.rm=T))
                print.data.frame(chunk_MHU, row.names=F)
                my_print()

                chunk_MHU <- task_stats_only_chunks %>% group_by(outline_function) %>% summarise(MHU_median = median(mem_hier_util, na.rm=T), MHU_mean = mean(mem_hier_util, na.rm=T), MHU_sd = sd(mem_hier_util, na.rm=T))
                print.data.frame(chunk_MHU, row.names=F)
                my_print()
            }

            # Chunk work deviation
            if ("work_deviation" %in% colnames(task_stats_only_chunks)) {
                my_print("# Chunks work deviation:")

                # Summary
                my_print("Work deviation summary:")
                print(stat.desc(as.numeric(task_stats_only_chunks$work_deviation)), row.names=F)
                my_print()

                # By outline function and idlejoin
                my_print("Work deviation by outline_function:")
                chunk_work_deviation <- task_stats_only_chunks %>% group_by(outline_function, idle_join) %>% summarise(work_deviation_median = median(work_deviation, na.rm=T), work_deviation_mean = mean(work_deviation, na.rm=T), work_deviation_sd = sd(work_deviation, na.rm=T))
                print.data.frame(chunk_work_deviation, row.names=F)
                my_print()

                chunk_work_deviation <- task_stats_only_chunks %>% group_by(outline_function) %>% summarise(work_deviation_median = median(work_deviation, na.rm=T), work_deviation_mean = mean(work_deviation, na.rm=T), work_deviation_sd = sd(work_deviation, na.rm=T))
                print.data.frame(chunk_work_deviation, row.names=F)
                my_print()
            }
        }# }}}

        # Chunk work balance
        if ("chunk_work_balance" %in% colnames(task_stats)) {# {{{
            my_print("# Chunk work balance (max(work_cycles)/mean(work_cycles)):")

            chunk_tasks <- which(grepl(glob2rx("chunk_*_*"), task_stats$metadata))
            task_stats_only_chunks <- task_stats[chunk_tasks, ]
            task_stats_only_chunks <- subset(task_stats_only_chunks, select=c(task, chunk_work_balance, outline_function, idle_join))

            # Chunk work balance summary
            my_print("Chunk work balance summary:")
            print(stat.desc(as.numeric(task_stats_only_chunks$chunk_work_balance)), row.names=F)
            my_print()

            # By outline function and idlejoin
            my_print("Chunk work balance by outline_function:")
            chunk_chunk_work_balance <- task_stats_only_chunks %>% group_by(outline_function, idle_join) %>% summarise(chunk_work_balance_median = median(chunk_work_balance, na.rm=T), chunk_work_balance_mean = mean(chunk_work_balance, na.rm=T), chunk_work_balance_sd = sd(chunk_work_balance, na.rm=T))
            print.data.frame(chunk_chunk_work_balance, row.names=F)
            my_print()

            chunk_chunk_work_balance <- task_stats_only_chunks %>% group_by(outline_function) %>% summarise(chunk_work_balance_median = median(chunk_work_balance, na.rm=T), chunk_work_balance_mean = mean(chunk_work_balance, na.rm=T), chunk_work_balance_sd = sd(chunk_work_balance, na.rm=T))
            print.data.frame(chunk_chunk_work_balance, row.names=F)
            my_print()
        }# }}}

        # Chunk work CPU balance
        if ("chunk_work_cpu_balance" %in% colnames(task_stats)) {# {{{
            my_print("# Chunk CPU work balance (max(work_cycles/core)/mean(work_cycles/core)):")

            chunk_tasks <- which(grepl(glob2rx("chunk_*_*"), task_stats$metadata))
            task_stats_only_chunks <- task_stats[chunk_tasks, ]
            task_stats_only_chunks <- subset(task_stats_only_chunks, select=c(task, chunk_work_cpu_balance, outline_function, idle_join))

            # Chunk work CPU balance summary
            my_print("Chunk work CPU balance summary:")
            print(stat.desc(as.numeric(task_stats_only_chunks$chunk_work_cpu_balance)), row.names=F)
            my_print()

            # By outline function and idlejoin
            my_print("Chunk work CPU balance by outline_function:")
            chunk_chunk_work_cpu_balance <- task_stats_only_chunks %>% group_by(outline_function, idle_join) %>% summarise(chunk_work_cpu_balance_median = median(chunk_work_cpu_balance, na.rm=T), chunk_work_cpu_balance_mean = mean(chunk_work_cpu_balance, na.rm=T), chunk_work_cpu_balance_sd = sd(chunk_work_cpu_balance, na.rm=T))
            print.data.frame(chunk_chunk_work_cpu_balance, row.names=F)
            my_print()

            chunk_chunk_work_cpu_balance <- task_stats_only_chunks %>% group_by(outline_function) %>% summarise(chunk_work_cpu_balance_median = median(chunk_work_cpu_balance, na.rm=T), chunk_work_cpu_balance_mean = mean(chunk_work_cpu_balance, na.rm=T), chunk_work_cpu_balance_sd = sd(chunk_work_cpu_balance, na.rm=T))
            print.data.frame(chunk_chunk_work_cpu_balance, row.names=F)
            my_print()
        }# }}}
    }
}

# Start summarizing
if (parsed$timing) tic(type="elapsed")

my_print("Summarizing all tasks ...")

# Set output files
if (!Rstudio_mode) {
    out_file <- "task-stats.info"
    sink(out_file)
}

summarize_task_stats(task_stats)

# Write to file
if (!Rstudio_mode) {
    sink()
    if (parsed$verbose) my_print(paste("Wrote file:", out_file))
}

# Extensive summary
if (parsed$extend) {
    if ("last_to_finish" %in% colnames(task_stats)) {
        my_print("Summarizing leaf tasks ...")

        # Subset
        task_stats_leaf <- subset(task_stats, leaf == T)

        # Set output files
        if (!Rstudio_mode) {
            out_file <- "task-stats-leaf-tasks.info"
            sink(out_file)
        }

        summarize_task_stats(task_stats_leaf)

        # Write to file
        if (!Rstudio_mode) {
            sink()
            if (parsed$verbose) my_print(paste("Wrote file:", out_file))
        }

        my_print("Summarizing non-leaf tasks ...")

        # Subset
        task_stats_non_leaf <- subset(task_stats, leaf == F)

        # Set output files
        if (!Rstudio_mode) {
            out_file <- "task-stats-non-leaf-tasks.info"
            sink(out_file)
        }

        summarize_task_stats(task_stats_non_leaf)

        # Write to file
        if (!Rstudio_mode) {
            sink()
            if (parsed$verbose) my_print(paste("Wrote file:", out_file))
        }
    }
}

if (parsed$timing) toc("Summarizing")

# Warn
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)

