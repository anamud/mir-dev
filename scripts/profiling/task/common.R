suppressMessages(library(dplyr, quietly=TRUE))
library(ggplot2, quietly=TRUE)
library(reshape2, quietly=TRUE)
library(scales, quietly=TRUE)
library(RColorBrewer, quietly=TRUE)
options("scipen"=999) # big number of digits

# Timing functions
tic <- function(gcFirst = TRUE, type=c("elapsed", "user.self", "sys.self"))
{
  type <- match.arg(type)
  assign(".type", type, envir=baseenv())
  if(gcFirst) gc(FALSE)
  tic <- proc.time()[type]         
  assign(".tic", tic, envir=baseenv())
  invisible(tic)
}

toc <- function(message)
{
  type <- get(".type", envir=baseenv())
  toc <- proc.time()[type]
  tic <- get(".tic", envir=baseenv())
  print(sprintf("%s: %f sec", message, toc - tic))
  invisible(toc)
}

# Useful functions
as.numeric.factor <- function(x) {as.numeric(levels(x))[x]}

# Plotting helpers
bar_plotter <- function(df, log=F, xt, yt, mt=" ", tilt45=F)
{
    colnames(df) <- c("subject", "variable")
    df.melt <- melt(df, id.vars="subject")
    
    if(tilt45 == T)
    {
    barp_t <- theme(axis.text.x = element_text(angle = 45, hjust = 1))
    } else {
    barp_t <- theme()
    }
    barp <- ggplot(df.melt[complete.cases(df.melt),], aes(x=subject,y=value)) + 
      geom_bar(stat="identity", width = 0.8, position = position_dodge(width = 0.8)) 
    if(log == T) {
      barp_l <- labs(x=xt, y=paste(yt,"(log scale)"),title=mt)
      barp <- barp + scale_y_log10() + barp_l + barp_t
    } else {
      barp_l <- labs(x=xt, y=yt,title=mt)
      barp <- barp + scale_y_continuous(labels=scientific) + barp_l + barp_t
    }          
    print(barp)
}

box_plotter <- function(v, xt, yt, mt=" ")
{
  v_comp <- v[complete.cases(v)] 
  v_comp <- v_comp[is.finite(v_comp)]
  v_comp <- v_comp[!is.nan(v_comp)]
  
  if(length(v_comp) > 0) {
    v.melt <- melt(v)
    boxp <- ggplot(v.melt, aes(y=value,x=factor(0))) + 
        geom_boxplot() +
        labs(x=xt,y=yt,title=mt) +
        scale_y_continuous(labels=scientific) +
        coord_flip()
    print(boxp)
  } else { 
      print("Cannot plot. No complete cases.")
  }
}

# Summary helpers
summarize_task_stats <- function(df, plot_title=" ")
{
  # Basics
  print("Number of tasks:")
  print(length(df$task))

  print("Number of siblings:")
  join.freq <- df %>% group_by(parent, joins_at) %>% summarise(count = n())
  print(summary(join.freq$count))
  box_plotter(join.freq$count, xt="", yt="Number of siblings", mt=plot_title)

  # Work and overhead
  print("Work:")
  print("Note: Work is the amount of computation by a task excluding runtime system calls.")
  print(summary(df$work_cycles))
  box_plotter(df$work_cycles, xt="", yt="Work cycles", mt=plot_title)
  
  print("Parallel overhead:")
  print("Note: Parallel overhead is the amount of computation done by a task inside runtime system calls.")
  print(summary(df$overhead_cycles))
  box_plotter(df$overhead_cycles, xt="", yt="Parallel overhead cycles", mt=plot_title)
  
  total_work <- sum(as.numeric(df$work_cycles))
  print(paste("Total work = ", total_work))
  
  total_ovh <- sum(as.numeric(df$overhead_cycles))
  print(paste("Total parallel overhead = ", total_ovh))
  
  if(total_ovh > 0)
    print(paste("Total work to total parallel overhead ratio = ", total_work/total_ovh))

  print("Parallelization benefit: ")
  print("Note: Parallel benefit is the ratio of work done by a task to the average overhead incurred by its parent.")
  print(summary(df$parallel_benefit))
  box_plotter(df$parallel_benefit, xt="", yt="Parallel benefit", mt=plot_title)
  
  # By CPU
  ## Work
  work.cpu <- as.table(tapply(df$work_cycles, as.numeric(df$cpu), FUN= function(x) {sum(as.numeric(x))} ))
  work.cpu <- data.frame(work.cpu)
  work.cpu$Var1 <- as.numeric.factor(work.cpu$Var1)
  bar_plotter(work.cpu, xt="Core", yt="Work cycles", mt=plot_title)
  
  ## Overhead
  ovh.cpu <- as.table(tapply(df$overhead_cycles, as.numeric(df$cpu), FUN= function(x) {sum(as.numeric(x))} ))
  ovh.cpu <- data.frame(ovh.cpu)
  ovh.cpu$Var1 <- as.numeric.factor(ovh.cpu$Var1)
  bar_plotter(ovh.cpu, xt="Core", yt="Parallel overhead cycles", mt=plot_title)
  
  # By tag
  ## Work
  work.tag <- as.table(tapply(df$work_cycles, df$tag, FUN= function(x) {sum(as.numeric(x))} ))
  bar_plotter(data.frame(work.tag), xt="Tag", yt="Work cycles", mt=plot_title, tilt45=T)
  
  ## Overhead
  ovh.tag <- as.table(tapply(df$overhead_cycles, df$tag, FUN= function(x) {sum(as.numeric(x))} ))
  bar_plotter(data.frame(ovh.tag), xt="Tag", yt="Parallel overhead cycles", mt=plot_title, tilt45=T)

  # Last tasks to finish
  bar_plotter(subset(df, last_to_finish == T, select=c(cpu_id, exec_end)), xt="Core", yt="Work cycles of last executed task", mt=plot_title)
  
  # Deviation
  if("work_deviation" %in% colnames(df))
  {
      print("Work deviation:")
      print(summary(df$work_deviation))
      box_plotter(df$work_deviation, xt="", yt="Work deviation", mt=plot_title)

      print("Overhead deviation:")
      print(summary(df$overhead_deviation))
      box_plotter(df$overhead_deviation, xt="", yt="Parallel overhead deviation", mt=plot_title)
  }

  # PAPI_RES_STL related
  if("PAPI_RES_STL_sum" %in% colnames(df))
  {
    df$work.PAPI_RES_STL <- df$work_cycles/df$PAPI_RES_STL_sum
    
    print("Work to PAPI_RES_STL ratio:")
    print(summary(df$work.PAPI_RES_STL))
    box_plotter(df$work.PAPI_RES_STL, xt="", yt="Work to PAPI_RES_STL ratio", mt=plot_title)

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    work.PAPI_RES_STL.cpu <- as.table(tapply(df$work.PAPI_RES_STL, as.numeric(df$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    work.PAPI_RES_STL.cpu <- data.frame(work.PAPI_RES_STL.cpu)
    work.PAPI_RES_STL.cpu$Var1 <- as.numeric.factor(work.PAPI_RES_STL.cpu$Var1)
    bar_plotter(work.PAPI_RES_STL.cpu, xt="Core", yt="Work to PAPI_RES_STL mean ratio", mt=plot_title)
    
    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    work.PAPI_RES_STL.tag <- as.table(tapply(df$work.PAPI_RES_STL, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
    bar_plotter(data.frame(work.PAPI_RES_STL.tag), xt="Tag", yt="Work to PAPI_RES_STL mean ratio",  mt=plot_title, tilt45=T)
  }

  # Memory hierarchy utilization
  if("mem_hier_util" %in% colnames(df))
  {
    print("Memory hierarchy utilization (PAPI_RES_STL to work ratio):")
    print(summary(df$mem_hier_util))
    box_plotter(df$mem_hier_util, xt="", yt="Memory hierarchy utilization (PAPI_RES_STL to work ratio)", mt=plot_title)

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    mem_hier_util.cpu <- as.table(tapply(df$mem_hier_util, as.numeric(df$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    mem_hier_util.cpu <- data.frame(mem_hier_util.cpu)
    mem_hier_util.cpu$Var1 <- as.numeric.factor(mem_hier_util.cpu$Var1)
    bar_plotter(mem_hier_util.cpu, xt="Core", yt="Mean memory hierarchy utilization (PAPI_RES_STL to work ratio)", mt=plot_title)
    
    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    mem_hier_util.tag <- as.table(tapply(df$mem_hier_util, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
    bar_plotter(data.frame(mem_hier_util.tag), xt="Tag", yt="Mean memory hierarchy utilization (PAPI_RES_STL to work ratio)",  mt=plot_title, tilt45=T)
  }

  # Computation intensity
  if("compute_int" %in% colnames(df))
  {
    print("Compute intensity:")
    print(summary(df$compute_int))
    box_plotter(df$compute_int, xt="", yt="Compute intensity (instruction count to memory footprint ratio)", mt=plot_title)

    # By CPU
    ## Mean work to PAPI_RES_STL ratio
    compute_int.cpu <- as.table(tapply(df$compute_int, as.numeric(df$cpu_id), FUN= function(x) {mean(as.numeric(x))} ))
    compute_int.cpu <- data.frame(compute_int.cpu)
    compute_int.cpu$Var1 <- as.numeric.factor(compute_int.cpu$Var1)
    bar_plotter(compute_int.cpu, xt="Core", yt="Mean compute intensity (instruction count to memory footprint ratio)", mt=plot_title)
    
    # By Tag
    ## Mean work to PAPI_RES_STL ratio
    compute_int.tag <- as.table(tapply(df$compute_int, df$tag, FUN= function(x) {mean(as.numeric(x))} ))
    bar_plotter(data.frame(compute_int.tag), xt="Tag", yt="Mean compute intensity (instruction count to memory footprint ratio)",  mt=plot_title, tilt45=T)
  }
}

