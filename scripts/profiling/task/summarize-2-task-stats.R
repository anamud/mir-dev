# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Read arguments
option_list <- list(
make_option(c("--left"), help = "Task stats.", metavar="FILE"),
make_option(c("--right"), help = "Task stats.", metavar="FILE"),
make_option(c("--leftname"), help = "Task stats name.", default="left", metavar="STRING"),
make_option(c("--rightname"), help = "Task stats name.", default="right", metavar="STRING"),
make_option(c("-o", "--out"), help = "Output file name suffix.", default="summary-2", metavar="STRING"),
make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
make_option(c("--scatter"), action="store_true", default=FALSE, help="Compute scatter."),
make_option(c("--extend"), action="store_true", default=FALSE, help="Extensive summary."),
make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("left", where=parsed) & !exists("right", where=parsed))
{
    print("Error: Invalid arguments. Check help (-h).")
    quit("no", 1)
}

# Read data
if(parsed$verbose) print(paste("Reading file", parsed$left))
if(parsed$verbose) print(paste("Reading file", parsed$right))
ts.data1 <- read.csv(parsed$left, header=TRUE)
ts.data2 <- read.csv(parsed$right, header=TRUE)

# Remove non-sense data
if(parsed$timing) tic(type="elapsed")
# Remove background task 
ts.data1 <- ts.data1[!is.na(ts.data1$parent),]
ts.data2 <- ts.data2[!is.na(ts.data2$parent),]
if(parsed$timing) toc("Removing non-sense data")

# Summary helper
summarize_task_stats <- function(df1, df2, df.l=c("left", "right"), plot_title=" ")
{
  ## Count by tag
  count.tag1 <- df1 %>% group_by(tag) %>% summarise(count = n())
  count.tag2 <- df2 %>% group_by(tag) %>% summarise(count = n())
  count.tag12 <- merge(count.tag1, count.tag2, by="tag", all=T)
  bar_2_plotter(count.tag12, xt="Tag", yt="Count", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)

  ## Siblings
  join.freq1 <- df1 %>% group_by(parent, joins_at) %>% summarise(count = n())
  join.freq2 <- df2 %>% group_by(parent, joins_at) %>% summarise(count = n())
  box_2_plotter(join.freq1$count, join.freq2$count, xt="", yt="Number of siblings", mt=plot_title, lt=df.l)

  # Work
  if("work_cycles" %in% colnames(df1) & "work_cycles" %in% colnames(df2) )
  {
      # Summary
      box_2_plotter(df1$work_cycles, df2$work_cycles, xt="", yt="Work cycles", mt=plot_title, lt=df.l, log=T)

      ### By CPU
      work.cpu1 <- as.data.frame(as.table(tapply(df1$work_cycles, as.numeric(df1$cpu_id), FUN= function(x) {sum(as.numeric(x))} )))
      colnames(work.cpu1) <- c("cpu", "work")
      work.cpu2 <- as.data.frame(as.table(tapply(df2$work_cycles, as.numeric(df2$cpu_id), FUN= function(x) {sum(as.numeric(x))} )))
      colnames(work.cpu2) <- c("cpu", "work")
      work.cpu12 <- merge(work.cpu1, work.cpu2, by="cpu", all=T)
      bar_2_plotter(work.cpu12, xt="Core", yt="Total work cycles", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)

      ## By tag
      work.tag1 <- as.data.frame(as.table(tapply(df1$work_cycles, df1$tag, FUN= function(x) {sum(as.numeric(x))} )))
      colnames(work.tag1) <- c("tag", "work")
      work.tag2 <- as.data.frame(as.table(tapply(df2$work_cycles, df2$tag, FUN= function(x) {sum(as.numeric(x))} )))
      colnames(work.tag2) <- c("tag", "work")
      work.tag12 <- merge(work.tag1, work.tag2, by="tag", all=T)
      bar_2_plotter(data.frame(work.tag12), xt="Tag", yt="Total work cycles", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)
      
      ### By tag
      if(any(is.na(df1$work_cycles)) | any(is.na(df2$work_cycles)))
      {
          stop("Error: Work cycles data contains NAs. Aborting!")
          quit("no", 1)
      }
      work.tag1 <- as.table(tapply(df1$work_cycles, df1$tag, FUN= function(x) {mean(as.numeric(x))} ))
      work.tag1 <- data.frame(work.tag1)
      colnames(work.tag1) <- c("tag", "work")
      work.tag2 <- as.table(tapply(df2$work_cycles, df2$tag, FUN= function(x) {mean(as.numeric(x))} ))
      work.tag2 <- data.frame(work.tag2)
      colnames(work.tag2) <- c("tag", "work")
      work.tag12 <- merge(work.tag1, work.tag2, by="tag", all=T)
      bar_2_plotter(work.tag12, xt="Tag", yt="Mean work cycles", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)
  }
  
  # Overhead
  if("overhead_cycles" %in% colnames(df))
  {
      # Summary
      box_2_plotter(df1$overhead_cycles, df2$overhead_cycles, xt="", yt="Parallel overhead cycles", mt=plot_title, lt=df.l , log=T)
      
      ## By CPU
      ovh.cpu1 <- as.table(tapply(df1$overhead_cycles, as.numeric(df1$cpu_id), FUN= function(x) {sum(as.numeric(x))} ))
      ovh.cpu1 <- data.frame(ovh.cpu1)
      colnames(ovh.cpu1) <- c("cpu", "ovh")
      ovh.cpu2 <- as.table(tapply(df2$overhead_cycles, as.numeric(df2$cpu_id), FUN= function(x) {sum(as.numeric(x))} ))
      ovh.cpu2 <- data.frame(ovh.cpu2)
      colnames(ovh.cpu2) <- c("cpu", "ovh")
      ovh.cpu12 <- merge(ovh.cpu1, ovh.cpu2, by="cpu", all=T)
      bar_2_plotter(ovh.cpu12, xt="Core", yt="Total parallel overhead cycles", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)
      
      ## By tag
      ovh.tag1 <- as.data.frame(as.table(tapply(df1$overhead_cycles, df1$tag, FUN= function(x) {sum(as.numeric(x))} )))
      colnames(ovh.tag1) <- c("tag", "ovh")
      ovh.tag2 <- as.data.frame(as.table(tapply(df2$overhead_cycles, df2$tag, FUN= function(x) {sum(as.numeric(x))} )))
      colnames(ovh.tag2) <- c("tag", "ovh")
      ovh.tag12 <- merge(ovh.tag1, ovh.tag2, by="tag", all=T)
      bar_2_plotter(ovh.tag12, xt="Tag", yt="Total parallel overhead cycles", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)

      ## By tag
      if(any(is.na(df1$overhead_cycles)))
      {
          warning("Overhead cycles data contains NAs. Ignoring NAs to calculate mean.")
          ovh.tag1 <- as.table(tapply(df1$overhead_cycles, df1$tag, FUN= function(x) {mean(as.numeric(x), na.rm=T)} ))
      } else {
          ovh.tag1 <- as.table(tapply(df1$overhead_cycles, df1$tag, FUN= function(x) {mean(as.numeric(x))} ))
      }
      ovh.tag1 <- data.frame(ovh.tag1)
      colnames(ovh.tag1) <- c("tag", "overhead")
      if(any(is.na(df2$overhead_cycles)))
      {
          warning("Overhead cycles data contains NAs. Ignoring NAs to calculate mean.")
          ovh.tag2 <- as.table(tapply(df2$overhead_cycles, df2$tag, FUN= function(x) {mean(as.numeric(x), na.rm=T)} ))
      } else {
          ovh.tag2 <- as.table(tapply(df2$overhead_cycles, df2$tag, FUN= function(x) {mean(as.numeric(x))} ))
      }
      ovh.tag2 <- data.frame(ovh.tag2)
      colnames(ovh.tag2) <- c("tag", "overhead")
      ovh.tag12 <- merge(ovh.tag1, ovh.tag2, by="tag", all=T)
      bar_2_plotter(ovh.tag12, xt="Tag", yt="Mean parallel overhead cycles", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90, log=T)
  }

  # Parallelization benefit
  if("parallel_benefit" %in% colnames(df1) & "parallel_benefit" %in% colnames(df2))
  {
      # Remove wrapper task 
      df.temp1 <- df1[!df1$parent==0,]
      df.temp2 <- df2[!df2$parent==0,]

      # Summary
      box_2_plotter(df.temp1$parallel_benefit, df.temp2$parallel_benefit, xt="", yt="Parallel benefit", mt=plot_title, log=T, lt=df.l)

      ## By tag
      if(any(is.na(df.temp1$parallel_benefit)))
      {
          stop("Error: Parallel benefit data contains NAs. Aborting!")
          quit("no", 1)
      }
      pb.tag1 <- as.table(tapply(df.temp1$parallel_benefit, df.temp1$tag, FUN= function(x) {mean(as.numeric(x))} ))
      pb.tag1 <- data.frame(pb.tag1)
      colnames(pb.tag1) <- c("tag", "parallel_benefit")
      if(any(is.na(df.temp2$parallel_benefit)))
      {
          stop("Error: Parallel benefit data contains NAs. Aborting!")
          quit("no", 1)
      }
      pb.tag2 <- as.table(tapply(df.temp2$parallel_benefit, df.temp2$tag, FUN= function(x) {mean(as.numeric(x))} ))
      pb.tag2 <- data.frame(pb.tag2)
      colnames(pb.tag2) <- c("tag", "parallel_benefit")
      pb.tag12 <- merge(pb.tag1, pb.tag2, by="tag", all=T)
      bar_2_plotter(pb.tag12, xt="Tag", yt="Mean parallel benefit", mt=plot_title, log=T, lt=df.l, tilt=T, tilt_angle=90)
  }

  # Last tasks to finish
  if("last_to_finish" %in% colnames(df1) & "last_to_finish" %in% colnames(df2))
  {
      last1 <- subset(df1, last_to_finish == T, select=c(cpu_id, exec_end_instant))
      last2 <- subset(df2, last_to_finish == T, select=c(cpu_id, exec_end_instant))
      last12 <- merge(last1, last2, by="cpu_id", all=T)
      bar_2_plotter(last12, xt="Core", yt="Instant last executed task ended (cycles)", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
  }
  
  # Deviation
  if("work_deviation" %in% colnames(df1) & "work_deviation" %in% colnames(df2))
  {
      # Summary
      box_2_plotter(df1$work_deviation, df2$work_deviation, xt="", yt="Work deviation", mt=plot_title, lt=df.l)

      ## By tag
      if(any(is.na(df1$work_deviation)))
      {
          stop("Error: Work deviation data contains NAs. Aborting!")
          quit("no", 1)
      }
      wd.tag1 <- as.table(tapply(df1$work_deviation, df1$tag, FUN= function(x) {mean(as.numeric(x))} ))
      wd.tag1 <- data.frame(wd.tag1)
      colnames(wd.tag1) <- c("tag", "work_deviation")
      if(any(is.na(df2$work_deviation)))
      {
          stop("Error: Work deviation data contains NAs. Aborting!")
          quit("no", 1)
      }
      wd.tag2 <- as.table(tapply(df2$work_deviation, df2$tag, FUN= function(x) {mean(as.numeric(x))} ))
      wd.tag2 <- data.frame(wd.tag2)
      colnames(wd.tag2) <- c("tag", "work_deviation")
      wd.tag12 <- merge(wd.tag1, wd.tag2, by="tag", all=T)
      bar_2_plotter(wd.tag12, xt="Tag", yt="Mean work deviation", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)    
  }
 
  # PAPI_RES_STL related
  if("PAPI_RES_STL_sum" %in% colnames(df) & "work_cycles" %in% colnames(df) & "PAPI_RES_STL_sum" %in% colnames(df2) & "work_cycles" %in% colnames(df2))
  {
    # Summary
    df1$work.PAPI_RES_STL <- df1$work_cycles/df1$PAPI_RES_STL_sum
    df2$work.PAPI_RES_STL <- df2$work_cycles/df2$PAPI_RES_STL_sum
    box_2_plotter(df1$work.PAPI_RES_STL, df2$work.PAPI_RES_STL, xt="", yt="Work/PAPI_RES_STL", mt=plot_title, lt=df.l)

    # Sanity check
    if(any(is.na(df1$work.PAPI_RES_STL)))
    {
      stop("Error: Work per PAPI_RES_STL cycle data contains NAs. Aborting!")
      quit("no", 1)
    }
    if(any(is.na(df2$work.PAPI_RES_STL)))
    {
      stop("Error: Work per PAPI_RES_STL cycle data contains NAs. Aborting!")
      quit("no", 1)
    }

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    work.PAPI_RES_STL.cpu1 <- as.table(tapply(df1$work.PAPI_RES_STL, as.numeric(df1$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    work.PAPI_RES_STL.cpu1 <- data.frame(work.PAPI_RES_STL.cpu1)
    colnames(work.PAPI_RES_STL.cpu1) <- c("cpu", "work.papi_res_stl")
    work.PAPI_RES_STL.cpu2 <- as.table(tapply(df2$work.PAPI_RES_STL, as.numeric(df2$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    work.PAPI_RES_STL.cpu2 <- data.frame(work.PAPI_RES_STL.cpu2)
    colnames(work.PAPI_RES_STL.cpu2) <- c("cpu", "work.papi_res_stl")
    work.PAPI_RES_STL.cpu12 <- merge(work.PAPI_RES_STL.cpu1, work.PAPI_RES_STL.cpu2, by="cpu", all=T)
    bar_2_plotter(work.PAPI_RES_STL.cpu12, xt="Core", yt="Work/PAPI_RES_STL mean", mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
    
    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    work.PAPI_RES_STL.tag1 <- as.data.frame(as.table(tapply(df1$work.PAPI_RES_STL, df1$tag, FUN= function(x) {mean(as.numeric(x))} )))
    colnames(work.PAPI_RES_STL.tag1) <- c("tag", "work.papi_res_stl")
    work.PAPI_RES_STL.tag2 <- as.data.frame(as.table(tapply(df2$work.PAPI_RES_STL, df2$tag, FUN= function(x) {mean(as.numeric(x))} )))
    colnames(work.PAPI_RES_STL.tag2) <- c("tag", "work.papi_res_stl")
    work.PAPI_RES_STL.tag12 <- merge(work.PAPI_RES_STL.tag1, work.PAPI_RES_STL.tag2, by="tag", all=T)
    bar_2_plotter(work.PAPI_RES_STL.tag12, xt="Tag", yt="Work/PAPI_RES_STL mean",  mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
  }

  # Memory hierarchy utilization
  if("mem_hier_util" %in% colnames(df1) & "mem_hier_util" %in% colnames(df2))
  {
    # Summary
    box_2_plotter(df1$mem_hier_util, df2$mem_hier_util, xt="", yt=paste("Memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"), mt=plot_title, lt=df.l)

    # Sanity check
    if(any(is.na(df1$mem_hier_util)))
    {
      stop("Error: Memory hierarchy utilization data contains NAs. Aborting!")
      quit("no", 1)
    }
    if(any(is.na(df2$mem_hier_util)))
    {
      stop("Error: Memory hierarchy utilization data contains NAs. Aborting!")
      quit("no", 1)
    }

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    mem_hier_util.cpu1 <- as.table(tapply(df1$mem_hier_util, as.numeric(df1$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    mem_hier_util.cpu1 <- data.frame(mem_hier_util.cpu1)
    colnames(mem_hier_util.cpu1) <- c("cpu", "mem_hier_util")
    mem_hier_util.cpu2 <- as.table(tapply(df2$mem_hier_util, as.numeric(df2$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    mem_hier_util.cpu2 <- data.frame(mem_hier_util.cpu2)
    colnames(mem_hier_util.cpu2) <- c("cpu", "mem_hier_util")
    mem_hier_util.cpu12 <- merge(mem_hier_util.cpu1, mem_hier_util.cpu2, by="cpu", all=T)
    bar_2_plotter(mem_hier_util.cpu12, xt="Core", yt=paste("Mean memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"), mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
    
    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    mem_hier_util.tag1 <- as.data.frame(as.table(tapply(df1$mem_hier_util, df1$tag, FUN= function(x) {mean(as.numeric(x))} )))
    colnames(mem_hier_util.tag1) <- c("tag", "mem_hier_util")
    mem_hier_util.tag2 <- as.data.frame(as.table(tapply(df2$mem_hier_util, df2$tag, FUN= function(x) {mean(as.numeric(x))} )))
    colnames(mem_hier_util.tag2) <- c("tag", "mem_hier_util")
    mem_hier_util.tag12 <- merge(mem_hier_util.tag1, mem_hier_util.tag2, by="tag", all=T)
    bar_2_plotter(mem_hier_util.tag12, xt="Tag", yt=paste("Mean memory hierarchy utilization","(PAPI_RES_STL/work)",sep="\n"),  mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
  }

  # Computation intensity
  if("compute_int" %in% colnames(df1) & "compute_int" %in% colnames(df2))
  {
    box_2_plotter(df1$compute_int, df2$compute_int, xt="", yt=paste("Compute intensity", "(instruction count/memory footprint)", sep="\n"), mt=plot_title, lt=df.l)

    # Sanity check
    if(any(is.na(df1$compute_int)))
    {
      stop("Error: Compute intensity data contains NAs. Aborting!")
      quit("no", 1)
    }
    if(any(is.na(df2$compute_int)))
    {
      stop("Error: Compute intensity data contains NAs. Aborting!")
      quit("no", 1)
    }

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    compute_int.cpu1 <- as.table(tapply(df1$compute_int, as.numeric(df1$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    compute_int.cpu1 <- data.frame(compute_int.cpu1)
    colnames(compute_int.cpu1) <- c("cpu", "compute_int")
    compute_int.cpu2 <- as.table(tapply(df2$compute_int, as.numeric(df2$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    compute_int.cpu2 <- data.frame(compute_int.cpu2)
    colnames(compute_int.cpu2) <- c("cpu", "compute_int")
    compute_int.cpu12 <- merge(compute_int.cpu1, compute_int.cpu2, by="cpu", all=T) 
    bar_2_plotter(compute_int.cpu12, xt="Core", yt=paste("Mean compute intensity", "(instruction count/memory footprint)", sep="\n"), mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
    
    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    compute_int.tag1 <- as.data.frame(as.table(tapply(df1$compute_int, df1$tag, FUN= function(x) {mean(as.numeric(x))} )))
    colnames(compute_int.tag1) <- c("tag", "compute_int")
    compute_int.tag2 <- as.data.frame(as.table(tapply(df2$compute_int, df2$tag, FUN= function(x) {mean(as.numeric(x))} )))
    colnames(compute_int.tag2) <- c("tag", "compute_int")
    compute_int.tag12 <- merge(compute_int.tag1, compute_int.tag2, by="tag", all=T)
    bar_2_plotter(compute_int.tag12, xt="Tag", yt=paste("Mean compute intensity", "(instruction count/memory footprint)", sep="\n"),  mt=plot_title, lt=df.l, tilt=T, tilt_angle=90)
  }

  if(parsed$scatter)
  {
      if("cpu_id" %in% colnames(df1) & "cpu_id" %in% colnames(df2))
      {
        print("Computing scatter ...")
        get_fork_scatter_df <- function(df)
        {
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
            fork_scatter
        }
        fork_scatter1 <- get_fork_scatter_df(df1)
        fork_scatter2 <- get_fork_scatter_df(df2)
        box_2_plotter(fork_scatter1, fork_scatter2, xt="", yt="Sibling scatter = median(scatter)", mt=plot_title, )
      }
  }
 
#   ## TODO: Summarize lineage depth
#   #if("lineage" %in% colnames(ts.data))
#   #{
#   #}
}

# Summarize
if(parsed$verbose) print("Summarizing ...")
if(parsed$timing) tic(type="elapsed")
out.file.plots <- paste(gsub(". $", "", parsed$out), "-plots.pdf", sep="")
pdf(out.file.plots)

print("Summarizing all tasks ...")
summarize_task_stats(df1=ts.data1, df2=ts.data2, df.l=c(parsed$leftname, parsed$rightname), plot_title="All tasks")

if(parsed$extend)
{
    if("last_to_finish" %in% colnames(ts.data1) & "last_to_finish" %in% colnames(ts.data2))
    {
        print("Summarizing leaf tasks ...")
        ts.data1.leaf <- subset(ts.data1, leaf == T)
        ts.data2.leaf <- subset(ts.data2, leaf == T)
        summarize_task_stats(df1=ts.data1.leaf, df2=ts.data2.leaf, df.l=c(parsed$leftname, parsed$rightname), plot_title="Leaf tasks")

        print("Summarizing non-leaf tasks ...")
        ts.data1.non.leaf <- subset(ts.data1, leaf == F)
        ts.data2.non.leaf <- subset(ts.data2, leaf == F)
        summarize_task_stats(df1=ts.data1.non.leaf, df2=ts.data2.non.leaf, df.l=c(parsed$leftname, parsed$rightname), plot_title="Non-leaf tasks")
    }
}

junk <- dev.off()
if(parsed$verbose) print(paste("Wrote file:", out.file.plots))
if(parsed$timing) toc("Summarizing")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

