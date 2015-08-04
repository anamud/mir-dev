# Clear workspace
rm(list=ls())

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Read arguments
option_list <- list(
make_option(c("-d","--data"), help = "Task stats.", metavar="FILE"),
make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
make_option(c("--scatter"), action="store_true", default=FALSE, help="Compute scatter."),
make_option(c("--extend"), action="store_true", default=FALSE, help="Extensive summary."),
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

# Remove non-sense data
if(parsed$timing) tic(type="elapsed")
# Remove background task
ts.data <- ts.data[!is.na(ts.data$parent),]
#ts.data <- ts.data[!ts.data$parent==0,]
if(parsed$timing) toc("Removing non-sense data")

# Summary helper
summarize_task_stats <- function(df, plot_title=" ")
{
  # Basics
  ## Number of tasks
  print("Number of tasks:")
  print(length(df$task))

  ## Count by tag
  count.tag <- df %>% group_by(tag) %>% summarise(count = n())
  print("Count by tag")
  print(data.frame(count.tag))
  bar_plotter(data.frame(count.tag), xt="Tag", yt="Count", mt=plot_title, tilt=T, tilt_angle=90)

  ## Siblings
  print("Number of siblings:")
  join.freq <- df %>% group_by(parent, joins_at) %>% summarise(count = n())
  print(summary(join.freq$count))
  box_plotter(join.freq$count, xt="", yt="Number of siblings", mt=plot_title)

  # Work
  if("work_cycles" %in% colnames(df))
  {
      print("Work:")
      print("Note: Work is the amount of computation by a task excluding runtime system calls.")
      print(summary(df$work_cycles))
      box_plotter(df$work_cycles, xt="", yt="Work cycles", mt=plot_title)

      total_work <- sum(as.numeric(df$work_cycles))
      print(paste("Total work = ", total_work))

      ## By CPU
      work.cpu <- as.table(tapply(df$work_cycles, as.numeric(df$cpu_id), FUN= function(x) {sum(as.numeric(x))} ))
      work.cpu <- data.frame(work.cpu)
      work.cpu$Var1 <- as.numeric.factor(work.cpu$Var1)
      bar_plotter(work.cpu, xt="Core", yt="Work cycles", mt=plot_title)

      ## By tag
      work.tag <- as.table(tapply(df$work_cycles, df$tag, FUN= function(x) {sum(as.numeric(x))} ))
      bar_plotter(data.frame(work.tag), xt="Tag", yt="Work cycles", mt=plot_title, tilt=T, tilt_angle=90)

      ## By tag
      if(any(is.na(df$work_cycles)))
      {
          stop("Error: Work cycles data contains NAs. Aborting!")
          quit("no", 1)
      }
      work.tag <- as.table(tapply(df$work_cycles, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
      work.tag <- data.frame(work.tag)
      colnames(work.tag) <- c("tag", "work")
      print("Mean work cycles by tag")
      print(work.tag)
      bar_plotter(work.tag, xt="Tag", yt="Mean work cycles", mt=plot_title, tilt=T, tilt_angle=90)
  }

  # Overhead
  if("overhead_cycles" %in% colnames(df))
  {
      print("Parallel overhead:")
      print("Note: Parallel overhead is the amount of computation done by a task inside runtime system calls.")
      print(summary(df$overhead_cycles))
      box_plotter(df$overhead_cycles, xt="", yt="Parallel overhead cycles", mt=plot_title)

      total_ovh <- sum(as.numeric(df$overhead_cycles))
      print(paste("Total parallel overhead = ", total_ovh))

      if("work_cycles" %in% colnames(df))
      {
          if(total_ovh > 0)
            print(paste("Total work/total parallel overhead = ", total_work/total_ovh))
      }

      ## By CPU
      ovh.cpu <- as.table(tapply(df$overhead_cycles, as.numeric(df$cpu_id), FUN= function(x) {sum(as.numeric(x))} ))
      ovh.cpu <- data.frame(ovh.cpu)
      ovh.cpu$Var1 <- as.numeric.factor(ovh.cpu$Var1)
      bar_plotter(ovh.cpu, xt="Core", yt="Parallel overhead cycles", mt=plot_title)

      ## By tag
      ovh.tag <- as.table(tapply(df$overhead_cycles, df$tag, FUN= function(x) {sum(as.numeric(x))} ))
      bar_plotter(data.frame(ovh.tag), xt="Tag", yt="Parallel overhead cycles", mt=plot_title, tilt=T, tilt_angle=90)

      ## By tag
      if(any(is.na(df$overhead_cycles)))
      {
          warning("Overhead cycles data contains NAs. Ignoring NAs to calculate mean.")
          ovh.tag <- as.table(tapply(df$overhead_cycles, df$tag, FUN= function(x) {mean(as.numeric(x), na.rm=T)} ))
      } else {
          ovh.tag <- as.table(tapply(df$overhead_cycles, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
      }
      ovh.tag <- data.frame(ovh.tag)
      colnames(ovh.tag) <- c("tag", "overhead")
      print("Mean overhead cycles by tag")
      print(ovh.tag)
      bar_plotter(ovh.tag, xt="Tag", yt="Mean parallel overhead cycles", mt=plot_title, tilt=T, tilt_angle=90)
  }

  # Parallelization benefit
  if("parallel_benefit" %in% colnames(df))
  {
      # Remove wrapper task
      df.temp <- df[!df$parent==0,]

      print("Parallelization benefit: ")
      print("Note: Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.")
      print(summary(df.temp$parallel_benefit))
      box_plotter(df.temp$parallel_benefit, xt="", yt="Parallel benefit", mt=plot_title, log=T)

      ## By tag
      if(any(is.na(df.temp$parallel_benefit)))
      {
          stop("Error: Parallel benefit data contains NAs. Aborting!")
          quit("no", 1)
      }
      pb.tag <- as.table(tapply(df.temp$parallel_benefit, df.temp$tag, FUN= function(x) {mean(as.numeric(x))} ))
      pb.tag <- data.frame(pb.tag)
      colnames(pb.tag) <- c("tag", "parallel_benefit")
      print("Mean parallel benefit by tag")
      print(pb.tag)
      bar_plotter(pb.tag, xt="Tag", yt="Mean parallel benefit", mt=plot_title, tilt=T, tilt_angle=90)
  }

  # Last tasks to finish
  if("last_to_finish" %in% colnames(df))
      bar_plotter(subset(df, last_to_finish == T, select=c(cpu_id, exec_end_instant)), xt="Core", yt="Instant last executed task ended (cycles)", mt=plot_title)

  # Deviation
  if("work_deviation" %in% colnames(df))
  {
      print("Work deviation:")
      print(summary(df$work_deviation))
      box_plotter(df$work_deviation, xt="", yt="Work deviation", mt=plot_title)

      ## By tag
      if(any(is.na(df$work_deviation)))
      {
          stop("Error: Work deviation data contains NAs. Aborting!")
          quit("no", 1)
      }
      wd.tag <- as.table(tapply(df$work_deviation, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
      wd.tag <- data.frame(wd.tag)
      colnames(wd.tag) <- c("tag", "work_deviation")
      print("Mean work deviation by tag")
      print(wd.tag)
      bar_plotter(wd.tag, xt="Tag", yt="Mean work deviation", mt=plot_title, tilt=T, tilt_angle=90)
  }

  # PAPI_RES_STL related
  if("PAPI_RES_STL_sum" %in% colnames(df) & "work_cycles" %in% colnames(df))
  {
    df$work.PAPI_RES_STL <- df$work_cycles/df$PAPI_RES_STL_sum

    print("Work to PAPI_RES_STL ratio:")
    print(summary(df$work.PAPI_RES_STL))
    box_plotter(df$work.PAPI_RES_STL, xt="", yt="Work/PAPI_RES_STL", mt=plot_title)

    # Sanity check
    if(any(is.na(df$work.PAPI_RES_STL)))
    {
      stop("Error: Work per PAPI_RES_STL cycle data contains NAs. Aborting!")
      quit("no", 1)
    }

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    work.PAPI_RES_STL.cpu <- as.table(tapply(df$work.PAPI_RES_STL, as.numeric(df$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    work.PAPI_RES_STL.cpu <- data.frame(work.PAPI_RES_STL.cpu)
    work.PAPI_RES_STL.cpu$Var1 <- as.numeric.factor(work.PAPI_RES_STL.cpu$Var1)
    bar_plotter(work.PAPI_RES_STL.cpu, xt="Core", yt="Work/PAPI_RES_STL mean", mt=plot_title)

    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    work.PAPI_RES_STL.tag <- as.table(tapply(df$work.PAPI_RES_STL, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
    bar_plotter(data.frame(work.PAPI_RES_STL.tag), xt="Tag", yt="Work/PAPI_RES_STL mean",  mt=plot_title, tilt=T, tilt_angle=90)
  }

  # Memory hierarchy utilization
  if("mem_hier_util" %in% colnames(df))
  {
    print("Memory hierarchy utilization (PAPI_RES_STL to work ratio):")
    print(summary(df$mem_hier_util))
    box_plotter(df$mem_hier_util, xt="", yt=paste("Memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"), mt=plot_title)

    # Sanity check
    if(any(is.na(df$mem_hier_util)))
    {
      stop("Error: Memory hierarchy utilization data contains NAs. Aborting!")
      quit("no", 1)
    }

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    mem_hier_util.cpu <- as.table(tapply(df$mem_hier_util, as.numeric(df$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    mem_hier_util.cpu <- data.frame(mem_hier_util.cpu)
    mem_hier_util.cpu$Var1 <- as.numeric.factor(mem_hier_util.cpu$Var1)
    bar_plotter(mem_hier_util.cpu, xt="Core", yt=paste("Mean memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"), mt=plot_title)

    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    mem_hier_util.tag <- as.table(tapply(df$mem_hier_util, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
    bar_plotter(data.frame(mem_hier_util.tag), xt="Tag", yt=paste("Mean memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"),  mt=plot_title, tilt=T, tilt_angle=90)
  }

  # Computation intensity
  if("compute_int" %in% colnames(df))
  {
    print("Compute intensity:")
    print(summary(df$compute_int))
    box_plotter(df$compute_int, xt="", yt=paste("Compute intensity", "(instruction count/memory footprint)", sep="\n"), mt=plot_title)

    # Sanity check
    if(any(is.na(df$compute_int)))
    {
      stop("Error: Compute intensity data contains NAs. Aborting!")
      quit("no", 1)
    }

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    compute_int.cpu <- as.table(tapply(df$compute_int, as.numeric(df$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    compute_int.cpu <- data.frame(compute_int.cpu)
    compute_int.cpu$Var1 <- as.numeric.factor(compute_int.cpu$Var1)
    bar_plotter(compute_int.cpu, xt="Core", yt=paste("Mean compute intensity", "(instruction count/memory footprint)", sep="\n"), mt=plot_title)

    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    compute_int.tag <- as.table(tapply(df$compute_int, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
    bar_plotter(data.frame(compute_int.tag), xt="Tag", yt=paste("Mean compute intensity", "(instruction count/memory footprint)", sep="\n"),  mt=plot_title, tilt=T, tilt_angle=90)
  }

  if(parsed$scatter)
  {
      if("cpu_id" %in% colnames(df))
      {
        print("Computing scatter ...")
        fork_nodes <- mapply(function(x, y, z) {paste('f', x, y, sep='.')}, x=df$parent, y=df$joins_at)
        fork_nodes <- unique(unlist(fork_nodes, use.names=FALSE))
        get_fork_scatter <- function(fork)
        {
            # Get fork info
            fork_split <- unlist(strsplit(fork, "\\."))
            parent <- as.numeric(fork_split[2])
            join_count <- as.numeric(fork_split[3])

            # Get cpu_id
            cpu_id <- df[df$parent == parent & df$joins_at == join_count, ]$cpu_id
            cpu_id <- cpu_id[!is.na(cpu_id)]

            # Compute scatter
            if(length(cpu_id) > 1)
                scatter <- c(dist(cpu_id))
            else
                scatter <- 0

            median(scatter)
        }
        fork_scatter <- as.vector(sapply(fork_nodes, get_fork_scatter))
        print("Scatter among siblings = median(scatter):")
        print(summary(fork_scatter))
        box_plotter(fork_scatter, xt="", yt="Sibling scatter = median(scatter)", mt=plot_title, )
      }
  }

  ## TODO: Summarize lineage depth
  #if("lineage" %in% colnames(ts.data))
  #{
  #}
}

# Summarize
if(parsed$verbose) print("Summarizing ...")
if(parsed$timing) tic(type="elapsed")
out.file <- paste(gsub(". $", "", parsed$data), ".info", sep="")
out.file.plots <- paste(gsub(". $", "", parsed$data), "-plots.pdf", sep="")
sink(out.file)
pdf(out.file.plots, width=10, height=7.5)

print("Summarizing all tasks ...")
summarize_task_stats(ts.data, "All tasks")

if(parsed$extend)
{
    if("last_to_finish" %in% colnames(ts.data))
    {
        print("Summarizing leaf tasks ...")
        ts.data.leaf <- subset(ts.data, leaf == T)
        summarize_task_stats(ts.data.leaf, "Leaf tasks")

        print("Summarizing non-leaf tasks ...")
        ts.data.non.leaf <- subset(ts.data, leaf == F)
        summarize_task_stats(ts.data.non.leaf, "Non-leaf tasks")
    }
}

junk <- dev.off()
sink()
if(parsed$verbose) print(paste("Wrote file:", out.file.plots))
if(parsed$verbose) print(paste("Wrote file:", out.file))
if(parsed$timing) toc("Summarizing")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

