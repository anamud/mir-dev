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
                   verbose=T,
                   timing=F)
} else {
    option_list <- list(
                        make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
                        make_option(c("--extend"), action="store_true", default=FALSE, help="Extensive summary."),
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

# Summary helper function
summarize_task_stats <- function(df, plot_title=" ")
{
    # Number of tasks
    my_print("# Number of tasks:")
    my_print(length(df$task))
    my_print()

    # Task count by outline function
    count_outline_func <- task_stats %>% group_by(outline_function) %>% summarise(count = n())
    my_print("# Task count by outline function")
    print(data.frame(count_outline_func), row.names=F)
    my_print()
    bar_plotter(data.frame(count_outline_func), xt="Outline function", yt="Count", mt=plot_title, tilt=T, tilt_angle=90)

    # Task siblings
    join.freq <- df %>% group_by(parent, joins_at) %>% summarise(count = n())
    my_print("# Task siblings summary:")
    my_print("# Task siblings share the same parent and join point.")
    print(stat.desc(as.numeric(join.freq$count)), row.names=F)
    my_print()
    box_plotter(join.freq$count, xt="", yt="Number of siblings", mt=plot_title)

    # Work
    if ("work_cycles" %in% colnames(task_stats)) {# {{{
        my_print("# Work:")
        my_print("# Work is the amount of computation performed by a task excluding runtime system calls.")

        # Summary
        my_print("Work summary:")
        print(stat.desc(as.numeric(task_stats$work_cycles)), row.names=F)
        my_print()
        box_plotter(task_stats$work_cycles, xt="", yt="Work cycles", mt=plot_title)

        # Total work by core
        work_core <- as.table(tapply(task_stats$work_cycles, as.numeric(task_stats$cpu_id), FUN= function(x) {sum(as.numeric(x))} ))
        work_core <- data.frame(work_core)
        work_core$Var1 <- as.numeric.factor(work_core$Var1)
        colnames(work_core) <- c("core", "work")
        my_print("Work by core:")
        print(work_core, row.names=F)
        my_print()
        bar_plotter(work_core, xt="Core", yt="Work cycles", mt=plot_title)

        # Total work by outline function
        work_outline_func <- as.table(tapply(task_stats$work_cycles, task_stats$outline_function, FUN= function(x) {sum(as.numeric(x))} ))
        bar_plotter(data.frame(work_outline_func), xt="Outline function", yt="Work cycles", mt=plot_title, tilt=T, tilt_angle=90)

        # Mean work by outline function
        if (any(is.na(task_stats$work_cycles))) {
            stop("Error: Work cycles data contains NAs. Aborting!")
            quit("no", 1)
        }
        work_outline_func <- as.table(tapply(task_stats$work_cycles, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        work_outline_func <- data.frame(work_outline_func)
        colnames(work_outline_func) <- c("outline_function", "work")
        my_print("Mean work by outline function:")
        print(work_outline_func, row.names=F)
        my_print()
        bar_plotter(work_outline_func, xt="Outline function", yt="Mean work cycles", mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # Overhead
    if ("overhead_cycles" %in% colnames(task_stats)) {# {{{
        my_print("# Overhead:")
        my_print("# Overhead is the amount of computation done by a task inside runtime system calls.")

        # Summary
        my_print("Overhead summary:")
        print(stat.desc(as.numeric(task_stats$overhead_cycles)), row.names=F)
        my_print()
        box_plotter(task_stats$overhead_cycles, xt="", yt="Parallel overhead cycles", mt=plot_title)

        if ("work_cycles" %in% colnames(task_stats)) {
            total_ovh <- sum(as.numeric(task_stats$overhead_cycles))
            if (total_ovh > 0)
                total_work <- sum(as.numeric(task_stats$work_cycles))
                my_print(paste("Total work/total parallel overhead = ", total_work/total_ovh))
                my_print()
        }

        # Overhead by core
        ovh_core <- as.table(tapply(task_stats$overhead_cycles, as.numeric(task_stats$cpu_id), FUN= function(x) {sum(as.numeric(x))} ))
        ovh_core <- data.frame(ovh_core)
        ovh_core$Var1 <- as.numeric.factor(ovh_core$Var1)
        colnames(ovh_core) <- c("core", "overhead")
        my_print("Overhead by core:")
        print(ovh_core, row.names=F)
        my_print()
        bar_plotter(ovh_core, xt="Core", yt="Overhead cycles", mt=plot_title)

        # Overhead by outline function
        ovh_outline_func <- as.table(tapply(task_stats$overhead_cycles, task_stats$outline_function, FUN= function(x) {sum(as.numeric(x))} ))
        bar_plotter(data.frame(ovh_outline_func), xt="Outline function", yt="Overhead cycles", mt=plot_title, tilt=T, tilt_angle=90)

        # Mean overhead by outline function
        if (any(is.na(task_stats$overhead_cycles)))
        {
            warning("Overhead data contains NAs. Ignoring NAs to calculate mean.")
            ovh_outline_func <- as.table(tapply(task_stats$overhead_cycles, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x), na.rm=T)} ))
        } else {
            ovh_outline_func <- as.table(tapply(task_stats$overhead_cycles, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        }
        ovh_outline_func <- data.frame(ovh_outline_func)
        colnames(ovh_outline_func) <- c("outline_function", "overhead")
        my_print("Mean overhead by outline function:")
        print(ovh_outline_func, row.names=F)
        my_print()
        bar_plotter(ovh_outline_func, xt="Outline function", yt="Mean overhead cycles", mt=plot_title, tilt=T, tilt_angle=90)
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
        box_plotter(task_stats_temp$parallel_benefit, xt="", yt="Parallel benefit", mt=plot_title, log=T)

        # Mean parallel benefit by outline function
        if (any(is.na(task_stats_temp$parallel_benefit))) {
            stop("Error: Parallel benefit data contains NAs. Aborting!")
            quit("no", 1)
        }
        pb_outline_func <- as.table(tapply(task_stats_temp$parallel_benefit, task_stats_temp$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        pb_outline_func <- data.frame(pb_outline_func)
        colnames(pb_outline_func) <- c("outline_function", "parallel_benefit")
        my_print("Mean parallel benefit by outline function:")
        print(pb_outline_func, row.names=F)
        my_print()
        bar_plotter(pb_outline_func, xt="Outline function", yt="Mean parallel benefit", mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # Last tasks to finish
    if ("last_to_finish" %in% colnames(task_stats)) {# {{{
        bar_plotter(subset(task_stats, last_to_finish == T, select=c(cpu_id, exec_end_instant)), xt="Core", yt="Instant last executed task ended (cycles)", mt=plot_title)
    }# }}}

    # Deviation
    if ("work_deviation" %in% colnames(task_stats)) {# {{{
        my_print("# Work deviation:")
        my_print("# Work deviation is the ratio of task work under different run scenarios. Example: Single-thread and multi-thread runs.")

        # Summary
        my_print("Work deviation summary:")
        print(stat.desc(as.numeric(task_stats$work_deviation)), row.names=F)
        my_print()
        box_plotter(task_stats$work_deviation, xt="", yt="Work deviation", mt=plot_title)

        # Mean deviation by outline function
        if (any(is.na(task_stats$work_deviation))) {
            stop("Error: Work deviation data contains NAs. Aborting!")
            quit("no", 1)
        }
        wd_outline_func <- as.table(tapply(task_stats$work_deviation, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        wd_outline_func <- data.frame(wd_outline_func)
        colnames(wd_outline_func) <- c("outline_function", "work_deviation")
        my_print("Mean work deviation by outline function:")
        print(wd_outline_func, row.names=F)
        my_print()
        bar_plotter(wd_outline_func, xt="Outline function", yt="Mean work deviation", mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # PAPI_RES_STL related
    if ("PAPI_RES_STL_sum" %in% colnames(task_stats) & "work_cycles" %in% colnames(task_stats)) {# {{{
        task_stats$work_PAPI_RES_STL <- task_stats$work_cycles/task_stats$PAPI_RES_STL_sum

        my_print("# Work to PAPI_RES_STL ratio:")

        # Summary
        my_print("Work to PAPI_RES_STL ratio summary:")
        print(stat.desc(as.numeric(task_stats$work_PAPI_RES_STL)), row.names=F)
        my_print()
        box_plotter(task_stats$work_PAPI_RES_STL, xt="", yt="Work/PAPI_RES_STL", mt=plot_title)

        # Mean work to PAPI_RES_STL ratio by core
        if (any(is.na(task_stats$work_PAPI_RES_STL))) {
            stop("Error: Work per PAPI_RES_STL cycle data contains NAs. Aborting!")
            quit("no", 1)
        }
        work_PAPI_RES_STL_core <- as.table(tapply(task_stats$work_PAPI_RES_STL, as.numeric(task_stats$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
        work_PAPI_RES_STL_core <- data.frame(work_PAPI_RES_STL_core)
        work_PAPI_RES_STL_core$Var1 <- as.numeric.factor(work_PAPI_RES_STL_core$Var1)
        colnames(work_PAPI_RES_STL_core) <- c("core", "work_PAPI_RES_STL")
        my_print("Mean work to PAPI_RES_STL by core:")
        print(work_PAPI_RES_STL_core, row.names=F)
        my_print()
        bar_plotter(work_PAPI_RES_STL_core, xt="Core", yt="Work/PAPI_RES_STL mean", mt=plot_title)

        # Mean work to PAPI_RES_STL ratio by outline function
        work_PAPI_RES_STL_outline_func <- as.table(tapply(task_stats$work_PAPI_RES_STL, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        work_PAPI_RES_STL_outline_func <- data.frame(work_PAPI_RES_STL_outline_func)
        colnames(work_PAPI_RES_STL_outline_func) <- c("outline_function", "work_PAPI_RES_STL")
        my_print("Mean work to PAPI_RES_STL by outline function:")
        print(work_PAPI_RES_STL_outline_func, row.names=F)
        my_print()
        bar_plotter(data.frame(work_PAPI_RES_STL_outline_func), xt="Outline function", yt="Work/PAPI_RES_STL mean",  mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # Memory hierarchy utilization
    if ("mem_hier_util" %in% colnames(task_stats)) {# {{{
        my_print("# Memory hierarchy utilization (PAPI_RES_STL to work ratio):")

        # Summary
        my_print("Memory hierarchy utilization summary:")
        print(stat.desc(as.numeric(task_stats$mem_hier_util)), row.names=F)
        my_print()
        box_plotter(task_stats$mem_hier_util, xt="", yt=paste("Memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"), mt=plot_title)

        # Mean work to PAPI_RES_STL ratio by core
        if (any(is.na(task_stats$mem_hier_util))) {
            stop("Error: Memory hierarchy utilization data contains NAs. Aborting!")
            quit("no", 1)
        }
        mem_hier_util_core <- as.table(tapply(task_stats$mem_hier_util, as.numeric(task_stats$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
        mem_hier_util_core <- data.frame(mem_hier_util_core)
        mem_hier_util_core$Var1 <- as.numeric.factor(mem_hier_util_core$Var1)
        colnames(mem_hier_util_core) <- c("core", "mem_hier_util")
        my_print("Mean memory hierarchy utilization to PAPI_RES_STL by core:")
        print(mem_hier_util_core, row.names=F)
        my_print()
        bar_plotter(mem_hier_util_core, xt="Core", yt=paste("Mean memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"), mt=plot_title)

        # Mean work to PAPI_RES_STL ratio by outline function
        mem_hier_util_outline_func <- as.table(tapply(task_stats$mem_hier_util, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        mem_hier_util_outline_func <- data.frame(mem_hier_util_outline_func)
        colnames(mem_hier_util_outline_func) <- c("outline_function", "mem_hier_util")
        my_print("Mean memory hierarchy utilization to PAPI_RES_STL by outline function:")
        print(mem_hier_util_outline_func, row.names=F)
        my_print()
        bar_plotter(data.frame(mem_hier_util_outline_func), xt="Outline function", yt=paste("Mean memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"),  mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # Computation intensity
    if ("compute_int" %in% colnames(task_stats)) {# {{{
        my_print("# Compute intensity (instruction count/memory footprint):")

        # Computation intensity summary
        my_print("Compute intensity summary:")
        print(stat.desc(as.numeric(task_stats$compute_int)), row.names=F)
        my_print()
        box_plotter(task_stats$compute_int, xt="", yt=paste("Compute intensity", "(instruction count/memory footprint)", sep="\n"), mt=plot_title)

        # Mean compute intensity by core
        if (any(is.na(task_stats$compute_int))) {
            stop("Error: Compute intensity data contains NAs. Aborting!")
            quit("no", 1)
        }
        compute_int_core <- as.table(tapply(task_stats$compute_int, as.numeric(task_stats$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
        compute_int_core <- data.frame(compute_int_core)
        compute_int_core$Var1 <- as.numeric.factor(compute_int_core$Var1)
        colnames(compute_int_core) <- c("core", "compute_intensity")
        my_print("Mean compute_intensity by core:")
        print(compute_int_core, row.names=F)
        my_print()
        bar_plotter(compute_int_core, xt="Core", yt=paste("Mean compute intensity", "(instruction count/memory footprint)", sep="\n"), mt=plot_title)

        # Mean compute intensity by outline function
        compute_int_outline_func <- as.table(tapply(task_stats$compute_int, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        compute_int_outline_func <- data.frame(compute_int_outline_func)
        colnames(compute_int_outline_func) <- c("outline_function", "compute_intensity")
        my_print("Mean compute intensity by outline function:")
        print(compute_int_outline_func, row.names=F)
        my_print()
        bar_plotter(data.frame(compute_int_outline_func), xt="Outline function", yt=paste("Mean compute intensity", "(instruction count/memory footprint)", sep="\n"),  mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # Sibling work balance
    if ("sibling_work_balance" %in% colnames(task_stats)) {# {{{
        my_print("# Sibling work balance (max(work_cycles)/mean(work_cycles)):")

        # Summary
        my_print("Sibling work balance summary:")
        print(stat.desc(as.numeric(task_stats$sibling_work_balance)), row.names=F)
        my_print()
        box_plotter(task_stats$sibling_work_balance, xt="", yt=paste("Sibling work balance", "max(work_cycles)/mean(work_cycles)", sep="\n"), mt=plot_title)

        # Mean sibling work balance by outline function
        if (any(is.na(task_stats$sibling_work_balance))) {
            stop("Error: Sibling work balance data contains NAs. Aborting!")
            quit("no", 1)
        }
        sibling_work_balance_outline_func <- as.table(tapply(task_stats$sibling_work_balance, task_stats$outline_function, FUN= function(x) {mean(as.numeric(x))} ))
        sibling_work_balance_outline_func <- data.frame(sibling_work_balance_outline_func)
        colnames(sibling_work_balance_outline_func) <- c("outline_function", "sibling_work_balance")
        my_print("Mean sibling work balance by outline function:")
        print(sibling_work_balance_outline_func, row.names=F)
        my_print()
        bar_plotter(data.frame(sibling_work_balance_outline_func), xt="Outline function", yt=paste("Mean sibling work balance", sep="\n"),  mt=plot_title, tilt=T, tilt_angle=90)
    }# }}}

    # Sibling scatter
    if ("sibling_scatter" %in% colnames(task_stats)) {# {{{
        my_print("# Sibling scatter:")

        # Summary
        my_print("Sibling scatter summary:")
        print(stat.desc(as.numeric(task_stats$sibling_scatter)), row.names=F)
        my_print()
        box_plotter(task_stats$sibling_scatter, xt="", yt=paste("Sibling scatter", "", sep="\n"), mt=plot_title)
    }# }}}
}

# Start summarizing
if (parsed$timing) tic(type="elapsed")

my_print("Summarizing all tasks ...")

# Set output files
if (!Rstudio_mode) {
    out_file <- "task-stats.info"
    out_file_plots <- "task-stats-summary-plots.pdf"
    sink(out_file)
    pdf(out_file_plots, width=10, height=7.5)
}

summarize_task_stats(task_stats, "All tasks")

# Write to file
if (!Rstudio_mode) {
    junk <- dev.off()
    sink()
    if (parsed$verbose) my_print(paste("Wrote file:", out_file_plots))
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
            out_file_plots <- "task-stats-leaf-tasks-summary-plots.pdf"
            sink(out_file)
            pdf(out_file_plots, width=10, height=7.5)
        }

        summarize_task_stats(task_stats_leaf, "Leaf tasks")

        # Write to file
        if (!Rstudio_mode) {
            junk <- dev.off()
            sink()
            if (parsed$verbose) my_print(paste("Wrote file:", out_file_plots))
            if (parsed$verbose) my_print(paste("Wrote file:", out_file))
        }

        my_print("Summarizing non-leaf tasks ...")

        # Subset
        task_stats_non_leaf <- subset(task_stats, leaf == F)

        # Set output files
        if (!Rstudio_mode) {
            out_file <- "task-stats-non-leaf-tasks.info"
            out_file_plots <- "task-stats-non-leaf-tasks-summary-plots.pdf"
            sink(out_file)
            pdf(out_file_plots, width=10, height=7.5)
        }

        summarize_task_stats(task_stats_non_leaf, "Non-leaf tasks")

        # Write to file
        if (!Rstudio_mode) {
            junk <- dev.off()
            sink()
            if (parsed$verbose) my_print(paste("Wrote file:", out_file_plots))
            if (parsed$verbose) my_print(paste("Wrote file:", out_file))
        }
    }
}

if (parsed$timing) toc("Summarizing")

# Warn
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)

