# Clean slate
rm(list=ls())

## Treat warnings as errors
#options(warn=2)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Library
library(igraph, quietly=TRUE)
suppressMessages(library(gdata, quietly=TRUE, warn.conflicts=FALSE))

# Graph element sizes
join_size <- 10
fork_size <- join_size
start_size <- 15
end_size <- start_size
task_size <- 30
task_size_mult <- 10
task_size_bins <- 10
fork_size_mult <- 10
fork_size_bins <- 10

# Graph element shapes
task_shape <- "rectangle"
fork_shape <- "circle"
join_shape <- fork_shape
start_shape <- fork_shape
end_shape <- start_shape

# Parse args
library(optparse, quietly=TRUE)
option_list <- list(
make_option(c("-d","--data"), help = "Task performance data file.", metavar="FILE"),
make_option(c("-p","--palette"), default="color", help = "Color palette for graph elements [default \"%default\"]."),
make_option(c("-o","--out"), default="task-graph", help = "Output file suffix [default \"%default\"].", metavar="STRING"),
make_option(c("-t", "--tree"), action="store_true", default=FALSE, help="Plot task graph as tree."),
make_option(c("--cplengthonly"), action="store_true", default=FALSE, help="Calculate critical path length only. Skip critical path enumeration."),
make_option(c("--analyze"), action="store_true", default=FALSE, help="Analyze task graph for problems."),
make_option(c("--config"), default="task-graph-analysis.cfg", help = "Analysis configuration file [default \"%default\"].", metavar="FILE"),
make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."),
make_option(c("--timing"), action="store_true", default=FALSE, help="Print processing time."))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("data", where=parsed))
{
    print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}

if(parsed$verbose) print("Initializing ...")
# Read data
if(parsed$timing) tic(type="elapsed")
tg.data <- read.csv(parsed$data, header=TRUE)
if(parsed$timing) toc("Read data")

# Information output
tg.info.out <- paste(gsub(". $", "", parsed$out), ".info", sep="")
sink(tg.info.out)
sink()

# Remove non-sense data
if(parsed$timing) tic(type="elapsed")
# Remove background task
tg.data <- tg.data[!is.na(tg.data$parent),]
if(parsed$timing) toc("Removing non-sense data")

# Set colors
if(parsed$timing) tic(type="elapsed")
join_color <- "#FF7F50"  # coral
fork_color <- "#2E8B57"  # seagreen
task_color <- "#4682B4" #steelblue
other_color <- "#DEB887" # burlywood
create_edge_color <- fork_color
sync_edge_color <- join_color
scope_edge_color <- "#000000"
cont_edge_color <- "#000000"
color_fun <- colorRampPalette(c("blue", "red"))
if(parsed$palette == "color") {
} else if(parsed$palette == "gray") {
    join_color <- "#D3D3D3"  # light gray
    fork_color <- "#D3D3D3"  # light gray
    task_color <- "#6B6B6B"  # gray42
    other_color <- "#D3D3D3" # light gray
    create_edge_color <- "#000000"
    sync_edge_color <- "#000000"
    scope_edge_color <-  "#000000"
    cont_edge_color <- "#000000"
    color_fun <- colorRampPalette(c("gray10", "gray90"))
} else {
    print("Unsupported color format. Supported formats: color, gray. Defaulting to color.")
}

# Task color binning
task_color_bins <- 10
task_color_pal <- color_fun(task_color_bins)
fork_color_bins <- 10
fork_color_pal <- color_fun(fork_color_bins)
if(parsed$timing) toc("Setting colors")

if(parsed$verbose) print("Creating graph ...")
# Create node lists
if(parsed$timing) tic(type="elapsed")
if(!parsed$tree)
{
    # Create join nodes list
    join_nodes <- mapply(function(x, y, z) {paste('j', x, y, sep='.')}, x=tg.data$parent, y=tg.data$joins_at)
    join_nodes_unique <- unique(unlist(join_nodes, use.names=FALSE))
}
# Create parent nodes list
parent_nodes_unique <- unique(tg.data$parent)

# Create fork nodes list
fork_nodes <- mapply(function(x, y, z) {paste('f', x, y, sep='.')}, x=tg.data$parent, y=tg.data$joins_at)
fork_nodes_unique <- unique(unlist(fork_nodes, use.names=FALSE))
if(parsed$timing) toc("Node list creation")

# Create graph
if(parsed$timing) tic(type="elapsed")
if(!parsed$tree)
{
tg <- graph.empty(directed=TRUE) + vertices('E',
                                            unique(c(join_nodes_unique,
                                                     fork_nodes_unique,
                                                     parent_nodes_unique,
                                                     tg.data$task)))
} else {
tg <- graph.empty(directed=TRUE) + vertices('E',
                                            unique(c(fork_nodes_unique,
                                                     parent_nodes_unique,
                                                     tg.data$task)))
}
if(parsed$timing) toc("Graph creation")

# Connect parent fork to task
if(parsed$verbose) print("Connecting nodes ...")
if(parsed$timing) tic(type="elapsed")
tg[from=fork_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=fork_nodes, to=tg.data$task, attr='color'] <- create_edge_color
if(parsed$timing) toc("Connect parent fork to task")

# Connect parent task to first fork
if(parsed$timing) tic(type="elapsed")
first_forks_index <- which(grepl("f.[0-9]+.0$", fork_nodes_unique))
parent_first_forks <- as.vector(sapply(fork_nodes_unique[first_forks_index], function(x) {gsub('f.(.*)\\.+.*','\\1', x)}))
first_forks <- fork_nodes_unique[first_forks_index]
tg[to=first_forks, from=parent_first_forks, attr='kind'] <- 'scope'
tg[to=first_forks, from=parent_first_forks, attr='color'] <- scope_edge_color
if("ins_count" %in% colnames(tg.data))
{
    tg[to=first_forks, from=parent_first_forks, attr='ins_count'] <- as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$ins_count)
    tg[to=first_forks, from=parent_first_forks, attr='weight'] <- -as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$ins_count)
} else {
    if("work_cycles" %in% colnames(tg.data))
    {
        tg[to=first_forks, from=parent_first_forks, attr='work_cycles'] <- as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$work_cycles)
        tg[to=first_forks, from=parent_first_forks, attr='weight'] <- -as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$work_cycles)
    }
}
if(parsed$timing) toc("Connect parent to first fork")

if(!parsed$tree)
{
    # Connect leaf task to join
    if(parsed$timing) tic(type="elapsed")
    leaf_tasks <- tg.data$task[tg.data$leaf == T]
    leaf_join_nodes <- join_nodes[match(leaf_tasks, tg.data$task)]
    tg[from=leaf_tasks, to=leaf_join_nodes, attr='kind'] <- 'sync'
    tg[from=leaf_tasks, to=leaf_join_nodes, attr='color'] <- sync_edge_color
    if("ins_count" %in% colnames(tg.data))
    {
        tg[from=leaf_tasks, to=leaf_join_nodes, attr='ins_count'] <- as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$ins_count)
        tg[from=leaf_tasks, to=leaf_join_nodes, attr='weight'] <- -as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$ins_count)
    } else {
        if("work_cycles" %in% colnames(tg.data))
        {
            tg[from=leaf_tasks, to=leaf_join_nodes, attr='work_cycles'] <- as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$work_cycles)
            tg[from=leaf_tasks, to=leaf_join_nodes, attr='weight'] <- -as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$work_cycles)
        }
    }
    if(parsed$timing) toc("Connect leaf task to join")

    # Connect join to next fork
    if(parsed$timing) tic(type="elapsed")
    #Rprof("profile-jointonext.out")
    find_next_fork <- function(node)
    {
      #print(paste('Processing node',node, sep=" "))

      # Get node info
      node_split <- unlist(strsplit(node, "\\."))
      parent <- as.numeric(node_split[2])
      join_count <- as.numeric(node_split[3])

      # Find next fork
      next_fork <- paste('f', as.character(parent), as.character(join_count+1), sep=".")
      if(is.na(match(next_fork, fork_nodes_unique)) == F)
      {
        # Connect to next fork
        next_fork <- next_fork
      } else {
        # Next fork is part of grandfather
        parent_index <- match(parent, tg.data$task)
        gfather <- tg.data[parent_index,]$parent
        gfather_join <- paste('j', as.character(gfather), as.character(tg.data[parent_index,]$joins_at), sep=".")

        if(is.na(match(gfather_join, join_nodes_unique)) == F)
        {
          # Connect to grandfather's join
          next_fork <- gfather_join
        } else {
          # Connect to end node
          next_fork <- 'E'
        }
      }
      next_fork
    }
    next_forks <- as.vector(sapply(join_nodes_unique, find_next_fork))
    tg[from=join_nodes_unique, to=next_forks, attr='kind'] <- 'continue'
    tg[from=join_nodes_unique, to=next_forks, attr='color'] <- cont_edge_color
    #Rprof(NULL)
    if(parsed$timing) toc("Connect join to next fork")
} else {
    # Connect fork to next fork
    if(parsed$timing) tic(type="elapsed")
    #Rprof("profile-forktonext.out")
    find_next_fork <- function(node)
    {
      #print(paste('Processing node',node, sep=" "))

      # Get node info
      node_split <- unlist(strsplit(node, "\\."))
      parent <- as.numeric(node_split[2])
      join_count <- as.numeric(node_split[3])

      # Find next fork
      next_fork <- paste('f', as.character(parent), as.character(join_count+1), sep=".")
      if(is.na(match(next_fork, fork_nodes_unique)) == F)
      {
        # Connect to next fork
        next_fork <- next_fork
      } else {
        # Connect to myself (self-loop)
        next_fork <- node
      }
      next_fork
    }
    next_forks <- as.vector(sapply(fork_nodes_unique, find_next_fork))
    tg[from=fork_nodes_unique, to=next_forks, attr='kind'] <- 'continue'
    tg[from=fork_nodes_unique, to=next_forks, attr='color'] <- cont_edge_color
    #Rprof(NULL)
    if(parsed$timing) toc("Connect fork to next fork")

    # Connext E to last fork of task 0
    if(parsed$timing) tic(type="elapsed")
    get_join_count <- function(node)
    {
      # Get node info
      node_split <- unlist(strsplit(node, "\\."))
      parent <- as.numeric(node_split[2])
      join_count <- as.numeric(node_split[3])
      join_count
    }
    fork_nodes_of_zero <- fork_nodes_unique[which(grepl("f.0.[0-9]+$", fork_nodes_unique))]
    largest_join_count_of_zero <- max(as.vector(sapply(fork_nodes_of_zero, get_join_count)))
    tg[from=paste("f.0.",largest_join_count_of_zero,sep=""), to='E', attr='kind'] <- 'continue'
    tg[from=paste("f.0.",largest_join_count_of_zero,sep=""), to='E', attr='color'] <- cont_edge_color
    if(parsed$timing) toc("Connect last fork of 0 to node E")
}

# Set attributes
if(parsed$verbose) print("Setting attributes ...")
if(parsed$timing) tic(type="elapsed")
# Common vertex attributes
V(tg)$label <- V(tg)$name
# Set task vertex attributes
task_index <- match(as.character(tg.data$task), V(tg)$name)
# Set annotations
for (annot in colnames(tg.data))
{
    values <- as.character(tg.data[,annot])
    tg <- set.vertex.attribute(tg, name=annot, index=task_index, value=values)
}
if(parsed$timing) toc("Task attribute setting")

# Set size
if(parsed$timing) tic(type="elapsed")
# Constants
tg <- set.vertex.attribute(tg, name='size', index=task_index, value=task_size)
tg <- set.vertex.attribute(tg, name='width', index=task_index, value=task_size)
tg <- set.vertex.attribute(tg, name='height', index=task_index, value=task_size)
tg <- set.vertex.attribute(tg, name='shape', index=task_index, value=task_shape)
# Attributes scaled to size
size_scaled <- c("ins_count", "work_cycles", "overhead_cycles", "exec_cycles")
for(attrib in size_scaled)
{
    if(attrib %in% colnames(tg.data))
    {
        # Set size
        attrib_unique <- unique(tg.data[,attrib])
        if(length(attrib_unique) == 1) p_task_size <- task_size_mult
        else p_task_size <- task_size_mult * as.numeric(cut(tg.data[,attrib], task_size_bins))
        annot_name <- paste(attrib, "_to_size", sep="")
        tg <- set.vertex.attribute(tg, name=annot_name, index=task_index, value=p_task_size)
        # Set height
        attrib_val <- tg.data[,attrib]
        attrib_val_norm <- 1 + ((attrib_val - min(attrib_val)) / (max(attrib_val) - min(attrib_val)))
        annot_name <- paste(attrib, "_to_height", sep="")
        tg <- set.vertex.attribute(tg, name=annot_name, index=task_index, value=attrib_val_norm*task_size)
    }
}
if(parsed$timing) toc("Task size calculation")

# Set color
if(parsed$timing) tic(type="elapsed")
# Constants
tg <- set.vertex.attribute(tg, name='color', index=task_index, value=task_color)
# Scale attributes to color
attrib_color_scaled <- c("mem_fp", "compute_int", "PAPI_RES_STL_sum", "mem_hier_util", "work_deviation", "overhead_deviation", "parallel_benefit")
for(attrib in attrib_color_scaled)
{
    if(attrib %in% colnames(tg.data))
    {
        # Set color in proportion to attrib
        attrib_unique <- unique(tg.data[,attrib])
        if(length(attrib_unique) == 1) p_task_color <- task_color_pal[1]
        else p_task_color <- task_color_pal[as.numeric(cut(tg.data[,attrib], task_color_bins))]
        annot_name <- paste(attrib, "_to_color", sep="")
        tg <- set.vertex.attribute(tg, name=annot_name, index=task_index, value=p_task_color)
        # Write colors for reference
        tg.file.out <- paste(gsub(". $", "", parsed$out), annot_name, sep=".")
        if(length(attrib_unique) == 1)
        {
            write.csv(data.frame(value=attrib_unique, color=p_task_color), tg.file.out, row.names=F)
        } else {
            v <- unique(cut(tg.data[,attrib], task_color_bins))
            write.csv(data.frame(value=v, color=task_color_pal[as.numeric(v)]), tg.file.out, row.names=F)
        }
        if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
    }
}
# Set attributes to distinct color
attrib_color_distinct <- c("cpu_id", "outl_func", "tag")
for(attrib in attrib_color_distinct)
{
    if(attrib %in% colnames(tg.data))
    {
        # Map distinct color to attrib
        attrib_val <- as.character(tg.data[,attrib])
        unique_attrib_val <- unique(attrib_val)
        attrib_color <- rainbow(length(unique_attrib_val))
        annot_name <- paste(attrib, "_to_color", sep="")
        tg <- set.vertex.attribute(tg, name=annot_name, index=task_index, value=attrib_color[match(attrib_val, unique_attrib_val)])
        # Write colors for reference
        tg.file.out <- paste(gsub(". $", "", parsed$out), annot_name, sep=".")
        write.csv(data.frame(value=unique_attrib_val, color=attrib_color), tg.file.out, row.names=F)
        if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
    }
}
if(parsed$timing) toc("Task color calculation")

if(parsed$timing) tic(type="elapsed")
# Set label and color of 'task 0'
start_index <- V(tg)$name == '0'
tg <- set.vertex.attribute(tg, name='color', index=start_index, value=other_color)
tg <- set.vertex.attribute(tg, name='label', index=start_index, value='S')
tg <- set.vertex.attribute(tg, name='size', index=start_index, value=start_size)
tg <- set.vertex.attribute(tg, name='shape', index=start_index, value=start_shape)

# Set label and color of 'task E'
end_index <- V(tg)$name == "E"
tg <- set.vertex.attribute(tg, name='color', index=end_index, value=other_color)
tg <- set.vertex.attribute(tg, name='label', index=end_index, value='E')
tg <- set.vertex.attribute(tg, name='size', index=end_index, value=end_size)
tg <- set.vertex.attribute(tg, name='shape', index=end_index, value=end_shape)

# Set fork vertex attributes
fork_nodes_index <- startsWith(V(tg)$name, 'f')
tg <- set.vertex.attribute(tg, name='size', index=fork_nodes_index, value=fork_size)
tg <- set.vertex.attribute(tg, name='color', index=fork_nodes_index, value=fork_color)
tg <- set.vertex.attribute(tg, name='label', index=fork_nodes_index, value='^')
tg <- set.vertex.attribute(tg, name='shape', index=fork_nodes_index, value=fork_shape)
if(parsed$timing) toc("Misc. attribute calculation")

# Set fork balance in terms of exec_cycles
if("exec_cycles" %in% colnames(tg.data))
{
    if(parsed$timing) tic(type="elapsed")
    #Rprof("profile-fork-bal-ec.out", line.profiling=TRUE)
    get_fork_bal <- function(fork)
    {
      # Get fork info
      fork_split <- unlist(strsplit(fork, "\\."))
      parent <- as.numeric(fork_split[2])
      join_count <- as.numeric(fork_split[3])

      # Get exec_cycles
      exec_cycles <- tg.data[tg.data$parent == parent & tg.data$joins_at == join_count, ]$exec_cycles
      exec_cycles <- exec_cycles[!is.na(exec_cycles)]

      # Compute balance
      bal <- max(exec_cycles)/mean(exec_cycles)

      bal
    }
    fork_bal_ec <- as.vector(sapply(V(tg)$name[fork_nodes_index], get_fork_bal))
    tg <- set.vertex.attribute(tg, name='exec_balance', index=fork_nodes_index, value=fork_bal_ec*fork_size)

    fork_bal_ec_unique <- unique(fork_bal_ec)
    if(length(fork_bal_ec_unique) == 1) p_fork_size <- fork_size_mult
    else p_fork_size <- fork_size_mult * as.numeric(cut(fork_bal_ec, fork_size_bins))
    tg <- set.vertex.attribute(tg, name='exec_balance_to_size', index=fork_nodes_index, value=p_fork_size)
    #Rprof(NULL)

    sink(tg.info.out, append=T)
    print("Load balance among siblings = max(exec_cycles)/mean(exec_cycles):")
    print(summary(fork_bal_ec))
    sink()
    tg.info.plot.out <- paste(gsub(". $", "", parsed$out), "-sibling-balance-ec-plot.pdf", sep="")
    pdf(tg.info.plot.out)
    box_plotter(fork_bal_ec, xt="", yt="Sibling load balance = max(exec_cycles)/mean(exec_cycles)")
    junk <- dev.off()
    if(parsed$verbose) print(paste("Wrote file:", tg.info.plot.out))
    if(parsed$timing) toc("Sibling load balance calculation (execution cycles)")
}

# Set fork balance in terms of work_cycles
if("work_cycles" %in% colnames(tg.data))
{
    if(parsed$timing) tic(type="elapsed")
    # Set fork balance
    get_fork_bal <- function(fork)
    {
      # Get fork info
      fork_split <- unlist(strsplit(fork, "\\."))
      parent <- as.numeric(fork_split[2])
      join_count <- as.numeric(fork_split[3])

      # Get work_cycles
      work_cycles <- tg.data[tg.data$parent == parent & tg.data$joins_at == join_count, ]$work_cycles
      work_cycles <- work_cycles[!is.na(work_cycles)]

      # Compute balance
      bal <- max(work_cycles)/mean(work_cycles)

      bal
    }
    fork_bal_wc <- as.vector(sapply(V(tg)[fork_nodes_index]$name, get_fork_bal))
    tg <- set.vertex.attribute(tg, name='work_balance', index=fork_nodes_index, value=fork_bal_wc)

    fork_bal_wc_unique <- unique(fork_bal_wc)
    if(length(fork_bal_wc_unique) == 1) p_fork_size <- fork_size_mult
    else p_fork_size <- fork_size_mult * as.numeric(cut(fork_bal_wc, fork_size_bins))
    tg <- set.vertex.attribute(tg, name='work_balance_to_size', index=fork_nodes_index, value=p_fork_size)

    sink(tg.info.out, append=T)
    print("Load balance among siblings = max(work_cycles)/mean(work_cycles):")
    print(summary(fork_bal_wc))
    sink()
    tg.info.plot.out <- paste(gsub(". $", "", parsed$out), "-sibling-balance-wc-plot.pdf", sep="")
    pdf(tg.info.plot.out)
    box_plotter(fork_bal_wc, xt="", yt="Sibling load balance = max(work_cycles)/mean(work_cycles)")
    junk <- dev.off()
    if(parsed$verbose) print(paste("Wrote file:", tg.info.plot.out))
    if(parsed$timing) toc("Sibling load balance calculation (work cycles)")
}

# Set fork scatter
if("cpu_id" %in% colnames(tg.data))
{
    if(parsed$timing) tic(type="elapsed")
    # Set fork scatter
    get_fork_scatter <- function(fork)
    {
      # Get fork info
      fork_split <- unlist(strsplit(fork, "\\."))
      parent <- as.numeric(fork_split[2])
      join_count <- as.numeric(fork_split[3])

      # Get cpu_id
      cpu_id <- tg.data[tg.data$parent == parent & tg.data$joins_at == join_count, ]$cpu_id
      cpu_id <- cpu_id[!is.na(cpu_id)]

      # Compute scatter
      if(length(cpu_id) > 1)
          scatter <- c(dist(cpu_id))
      else
          scatter <- 0

      median(scatter)
    }
    fork_scatter <- as.vector(sapply(V(tg)[fork_nodes_index]$name, get_fork_scatter))
    tg <- set.vertex.attribute(tg, name='scatter', index=fork_nodes_index, value=fork_scatter)

    fork_scatter_unique <- unique(fork_scatter)
    if(length(fork_scatter_unique) == 1) p_fork_size <- fork_size_mult
    else p_fork_size <- fork_size_mult * as.numeric(cut(fork_scatter, fork_size_bins))
    tg <- set.vertex.attribute(tg, name='scatter_to_size', index=fork_nodes_index, value=p_fork_size)

    if(length(fork_scatter_unique) == 1) p_fork_color <- fork_color_pal[1]
    else p_fork_color <- fork_color_pal[as.numeric(cut(fork_scatter, fork_color_bins))]
    tg <- set.vertex.attribute(tg, name='scatter_to_color', index=fork_nodes_index, value=p_fork_color)

    sink(tg.info.out, append=T)
    print("Scatter among siblings = median(scatter):")
    print(summary(fork_scatter))
    sink()
    tg.info.plot.out <- paste(gsub(". $", "", parsed$out), "-fork-scatter-plot.pdf", sep="")
    pdf(tg.info.plot.out)
    box_plotter(fork_scatter, xt="", yt="Sibling scatter = median(scatter)")
    junk <- dev.off()
    if(parsed$verbose) print(paste("Wrote file:", tg.info.plot.out))
    if(parsed$timing) toc("Fork scatter calculation")
}

# Set join vertex attributes
if(!parsed$tree)
{
    if(parsed$timing) tic(type="elapsed")
    join_nodes_index <- startsWith(V(tg)$name, 'j')
    tg <- set.vertex.attribute(tg, name='size', index=join_nodes_index, value=join_size)
    tg <- set.vertex.attribute(tg, name='color', index=join_nodes_index, value=join_color)
    tg <- set.vertex.attribute(tg, name='label', index=join_nodes_index, value='*')
    tg <- set.vertex.attribute(tg, name='shape', index=join_nodes_index, value=join_shape)
    if(parsed$timing) toc("Join attribute setting")
}

# Set edge attributes
if(parsed$timing) tic(type="elapsed")
if("ins_count" %in% colnames(tg.data))
{
    tg <- set.edge.attribute(tg, name="weight", index=which(is.na(E(tg)$weight)), value=0)
    tg <- set.edge.attribute(tg, name="ins_count", index=which(is.na(E(tg)$ins_count)), value=0)
} else {
    if("work_cycles" %in% colnames(tg.data))
    {
        tg <- set.edge.attribute(tg, name="weight", index=which(is.na(E(tg)$weight)), value=0)
        tg <- set.edge.attribute(tg, name="work_cycles", index=which(is.na(E(tg)$work_cycles)), value=0)
    }
}
if(parsed$timing) toc("Edge attribute setting")

# Check if graph has bad structure
if(parsed$verbose) print("Checking for bad structure ...")
if(parsed$timing) tic(type="elapsed")
if(is.element(0, degree(tg, fork_nodes_index, mode = c("in"))))
{
  print("Warning! One or more fork nodes have zero degree since one or more tasks in the program performed empty synchronization.")
  print("Aborting on error!")
  quit("no", 1)
}
if(is.element(0, degree(tg, fork_nodes_index, mode = c("out"))))
{
  print("Warning! One or more fork nodes have zero degree since one or more tasks in the program performed empty synchronization.")
  print("Aborting on error!")
  quit("no", 1)
}
if(!parsed$tree)
{
    if(is.element(0, degree(tg, join_nodes_index, mode = c("in"))))
    {
      print("Warning! One or more join nodes have zero degree since one or more tasks in the program performed empty synchronization.")
      print("Aborting on error!")
      quit("no", 1)
    }
    if(is.element(0, degree(tg, join_nodes_index, mode = c("out"))))
    {
      print("Warning! One or more join nodes have zero degree since one or more tasks in the program performed empty synchronization.")
      print("Aborting on error!")
      quit("no", 1)
    }
}
if(parsed$timing) toc("Checking for bad structure")

if("ins_count" %in% colnames(tg.data) && !parsed$tree)
{
    if(parsed$verbose) print("Calculating critical path ...")
    if(parsed$timing) tic(type="elapsed")
    # Simplify - DO NOT USE. Fucks up the critical path analysis.
    #tg <- simplify(tg, edge.attr.comb=toString)

    # Get critical path
    #Rprof("profile-critpathcalc.out")
    if(parsed$cplengthonly)
    {
      # Get critical path length
      sp <- shortest.paths(tg, v=start_index, to=end_index, mode="out")
      lpl <- -as.numeric(sp)
    } else {
      lntg <- length(V(tg))
      pb <- txtProgressBar(min = 0, max = lntg, style = 3)
      ctr <- 0
      # Topological sort
      tsg <- topological.sort(tg)
      # Set root path attributes
      V(tg)[tsg[1]]$rdist <- 0
      V(tg)[tsg[1]]$depth <- 0
      V(tg)[tsg[1]]$rpath <- tsg[1]
      # Get data frame of graph object
      vgdf <- get.data.frame(tg, what="vertices")
      # Get longest paths from root
      for(node in tsg[-1])
      {
        # Get distance from node's predecessors
        ni <- incident(tg, node, mode="in")
        w <- E(tg)$ins_count[ni]
        # Get distance from root to node's predecessors
        nn <- neighbors(tg, node, mode="in")
        d <- vgdf$rdist[nn]
        # Add distances (assuming one-one corr.)
        wd <- w+d
        # Set node's distance from root to max of added distances
        mwd <- max(wd)
        vgdf$rdist[node] <- mwd
        # Set node's path from root to path of max of added distances
        mwdn <- as.vector(nn)[match(mwd,wd)]
        nrp <- list(c(unlist(vgdf$rpath[mwdn]), node))
        vgdf$rpath[node] <- nrp
        # Set node's depth as one greater than the largest depth its predecessors
        vgdf$depth[node] <- max(vgdf$depth[nn]) + 1
        if(parsed$verbose) {ctr <- ctr + 1; setTxtProgressBar(pb, ctr);}
      }
      ## Longest path is the largest root distance
      lpl <- max(vgdf$rdist)
      # Enumerate longest path
      lpm <- unlist(vgdf$rpath[match(lpl,vgdf$rdist)])
      vgdf$on_crit_path <- 0
      vgdf$on_crit_path[lpm] <- 1
      # Set back on graph
      tg <- set.vertex.attribute(tg, name="on_crit_path", index=V(tg), value=vgdf$on_crit_path)
      tg <- set.vertex.attribute(tg, name="rdist", index=V(tg), value=vgdf$rdist)
      tg <- set.vertex.attribute(tg, name="depth", index=V(tg), value=vgdf$depth)
      critical_edges <- E(tg)[V(tg)[on_crit_path==1] %--% V(tg)[on_crit_path==1]]
      tg <- set.edge.attribute(tg, name="on_crit_path", index=critical_edges, value=1)
      if(parsed$verbose) {ctr <- ctr + 1; setTxtProgressBar(pb, ctr);}
      close(pb)
    }
    #Rprof(NULL)

    # Calculate and write info
    sink(tg.info.out, append=T)
    print("Unit = Instructions")
    print("span (critical path)")
    print(lpl)
    print("work")
    work <- sum(as.numeric(tg.data$ins_count))
    print(work)
    print("parallelism")
    print(work/lpl)
    sink()

    if(!parsed$cplengthonly)
    {
        # Clear rpath since dot/table writing complains
        tg <- remove.vertex.attribute(tg,"rpath")

        # Calc shape
        tgdf <- get.data.frame(tg, what="vertices")
        tgdf <- tgdf[!is.na(as.numeric(tgdf$label)),]
        tg_shape <- hist(tgdf$rdist, breaks=sum(as.numeric(tg.data$ins_count))/(length(unique(tg.data$cpu_id))*mean(tg.data$ins_count)), plot=F)

        # Write out shape
        tg.file.out <- paste(gsub(". $", "", parsed$out), "-shape.pdf", sep="")
        pdf(tg.file.out)
        plot(tg_shape, freq=T, xlab="Distance from START in instruction count", ylab="Tasks", main="Instantaneous task parallelism", col="white")
        abline(h = length(unique(tg.data$cpu_id)), col = "blue", lty=2)
        abline(h = work/lpl , col = "red", lty=1)
        legend("top", legend = c("Number of cores", "Exposed task parallelism"), fill = c("blue", "red"))
        dev.off()
        if(parsed$verbose) print(paste("Wrote file:", tg.file.out))

        # Calc shape based on depth
        tg_shape_depth <- tgdf %>% group_by(depth) %>% summarise(count = n())

        # Write out shape based on depth
        tg.file.out <- paste(gsub(". $", "", parsed$out), "-shape-depth.pdf", sep="")
        pdf(tg.file.out)
        plot(tg_shape_depth, freq=T, xlab="Distance from START in node count", ylab="Tasks", main="Instantaneous task parallelism", col="white")
        lines(tg_shape_depth, type="p")
        lines(tg_shape_depth, type="h")
        abline(h = length(unique(tg.data$cpu_id)), col = "blue", lty=2)
        abline(h = work/lpl , col = "red", lty=1)
        legend("top", legend = c("Number of cores", "Exposed task parallelism"), fill = c("blue", "red"))
        dev.off()
        if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
    }
    if(parsed$timing) toc("Critical path calculation (based on instructions)")
} else if("work_cycles" %in% colnames(tg.data) && !parsed$tree) {
    if(parsed$verbose) print("Calculating critical path ...")
    if(parsed$timing) tic(type="elapsed")

    # Simplify - DO NOT USE. Fucks up the critical path analysis.
    #tg <- simplify(tg, edge.attr.comb=toString)

    # Get critical path
    #Rprof("profile-critpathcalc.out")
    if(parsed$cplengthonly)
    {
      # Get critical path length
      sp <- shortest.paths(tg, v=start_index, to=end_index, mode="out")
      lpl <- -as.numeric(sp)
    } else {
      lntg <- length(V(tg))
      pb <- txtProgressBar(min = 0, max = lntg, style = 3)
      ctr <- 0
      # Topological sort
      tsg <- topological.sort(tg)
      # Set root path attributes
      V(tg)[tsg[1]]$rdist <- 0
      V(tg)[tsg[1]]$depth <- 0
      V(tg)[tsg[1]]$rpath <- tsg[1]
      # Get data frame of graph object
      vgdf <- get.data.frame(tg, what="vertices")
      # Get longest paths from root
      for(node in tsg[-1])
      {
        # Get distance from node's predecessors
        ni <- incident(tg, node, mode="in")
        w <- E(tg)$work_cycles[ni]
        # Get distance from root to node's predecessors
        nn <- neighbors(tg, node, mode="in")
        d <- vgdf$rdist[nn]
        # Add distances (assuming one-one corr.)
        wd <- w+d
        # Set node's distance from root to max of added distances
        mwd <- max(wd)
        vgdf$rdist[node] <- mwd
        # Set node's path from root to path of max of added distances
        mwdn <- as.vector(nn)[match(mwd,wd)]
        nrp <- list(c(unlist(vgdf$rpath[mwdn]), node))
        vgdf$rpath[node] <- nrp
        # Set node's depth as one greater than the largest depth its predecessors
        vgdf$depth[node] <- max(vgdf$depth[nn]) + 1
        if(parsed$verbose) {ctr <- ctr + 1; setTxtProgressBar(pb, ctr);}
      }
      ## Longest path is the largest root distance
      lpl <- max(vgdf$rdist)
      # Enumerate longest path
      lpm <- unlist(vgdf$rpath[match(lpl,vgdf$rdist)])
      vgdf$on_crit_path <- 0
      vgdf$on_crit_path[lpm] <- 1
      # Set back on graph
      tg <- set.vertex.attribute(tg, name="on_crit_path", index=V(tg), value=vgdf$on_crit_path)
      tg <- set.vertex.attribute(tg, name="rdist", index=V(tg), value=vgdf$rdist)
      tg <- set.vertex.attribute(tg, name="depth", index=V(tg), value=vgdf$depth)
      critical_edges <- E(tg)[V(tg)[on_crit_path==1] %--% V(tg)[on_crit_path==1]]
      tg <- set.edge.attribute(tg, name="on_crit_path", index=critical_edges, value=1)
      if(parsed$verbose) {ctr <- ctr + 1; setTxtProgressBar(pb, ctr);}
      close(pb)
    }
    #Rprof(NULL)

    # Calculate and write info
    sink(tg.info.out, append=T)
    print("Unit = Cycles")
    print("span (critical path)")
    print(lpl)
    print("work")
    work <- sum(as.numeric(tg.data$work_cycles))
    print(work)
    print("parallelism")
    print(work/lpl)
    sink()

    if(!parsed$cplengthonly)
    {
        # Clear rpath since dot/table writing complains
        tg <- remove.vertex.attribute(tg,"rpath")

        # Calc shape
        tgdf <- get.data.frame(tg, what="vertices")
        tgdf <- tgdf[!is.na(as.numeric(tgdf$label)),]
        tg_shape <- hist(tgdf$rdist, breaks=sum(as.numeric(tg.data$work_cycles))/(length(unique(tg.data$cpu_id))*mean(tg.data$work_cycles)), plot=F)

        # Write out shape
        tg.file.out <- paste(gsub(". $", "", parsed$out), "-shape.pdf", sep="")
        pdf(tg.file.out)
        plot(tg_shape, freq=T, xlab="Distance from START in work cycles", ylab="Tasks", main="Instantaneous task parallelism", col="white")
        abline(h = length(unique(tg.data$cpu_id)), col = "blue", lty=2)
        abline(h = work/lpl , col = "red", lty=1)
        dev.off()
        if(parsed$verbose) print(paste("Wrote file:", tg.file.out))

        # Calc shape based on depth
        tg_shape_depth <- tgdf %>% group_by(depth) %>% summarise(count = n())

        # Write out shape based on depth
        tg.file.out <- paste(gsub(". $", "", parsed$out), "-shape-depth.pdf", sep="")
        pdf(tg.file.out)
        plot(tg_shape_depth, xlab="Distance from START in node count", ylab="Tasks", main="Instantaneous task parallelism", col="white")
        lines(tg_shape_depth, type="p")
        lines(tg_shape_depth, type="h")
        abline(h = length(unique(tg.data$cpu_id)), col = "blue", lty=2)
        abline(h = work/lpl , col = "red", lty=1)
        dev.off()
        if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
    }
    if(parsed$timing) toc("Critical path calculation (based on cycles)")
} else {
    if(parsed$verbose) print("Simplifying graph ...")
    if(parsed$timing) tic(type="elapsed")
    tg <- simplify(tg, remove.multiple=T, remove.loops=T)
    if(parsed$timing) toc("Simplify")
}

if(parsed$verbose) print("Writing graph files ...")
## Write dot file
#if(parsed$timing) tic(type="elapsed")
#tg.file.out <- paste(gsub(". $", "", parsed$out), ".dot", sep="")
#res <- write.graph(tg, file=tg.file.out, format="dot")
#if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
#if(parsed$timing) toc("Write dot")

# Write gml file
if(parsed$timing) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", parsed$out), ".graphml", sep="")
res <- write.graph(tg, file=tg.file.out, format="graphml")
if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
if(parsed$timing) toc("Write graphml")

## Write pdf file
#if(parsed$timing) tic(type="elapsed")
#lyt <- layout.fruchterman.reingold(tg,niter=500,area=vcount(tg)^2,coolexp=3,repulserad=vcount(tg)^3,maxdelta=vcount(tg))
#tg.file.out <- paste(gsub(". $", "", parsed$out), ".pdf", sep="")
#pdf(file=tg.file.out)
#plot(tg, layout=lyt)
#dev.off()
#if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
#if(parsed$timing) toc("Write pdf")

# Write adjacency matrix file
if(parsed$timing) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", parsed$out), ".adjmat", sep="")
sink(tg.file.out)
print(get.adjacency(tg,names=T))
sink()
if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
if(parsed$timing) toc("Write adjacency matrix")

# Write edgelist file
if(parsed$timing) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", parsed$out), ".edgelist", sep="")
sink(tg.file.out)
print(get.edgelist(tg, names=T))
sink()
if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
if(parsed$timing) toc("Write edgelist")

# Write node attributes
if(parsed$timing) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", parsed$out), ".nodeattr", sep="")
write.table(get.data.frame(tg, what="vertices"), sep=",", file=tg.file.out)
if(parsed$verbose) print(paste("Wrote file:", tg.file.out))
if(parsed$timing) toc("Write node attributes")

# Show problems
if(parsed$analyze)
{
    if(parsed$verbose) print("Analyzing graph for problems ...")
    if(parsed$timing) tic(type="elapsed")
    # add.alpha function from http://www.magesblog.com/2013/04/how-to-change-alpha-value-of-colours-in.html
    add.alpha <- function(col, alpha=1){
      apply(sapply(col, col2rgb)/255, 2, function(x) rgb(x[1], x[2], x[3], alpha=alpha))
    }

    # Base task graph with transparent elements
    base_tg <- tg
    base_tg_vertex_color <- add.alpha(get.vertex.attribute(base_tg, name='color'), alpha=0.2)
    base_tg <- set.vertex.attribute(base_tg, name='color', value=base_tg_vertex_color)
    base_tg <- set.vertex.attribute(base_tg, name='problematic', value=0)
    if(!parsed$tree)
    {
        base_tg_edge_color <- add.alpha(get.edge.attribute(base_tg, name='color'), alpha=0.2)
        base_tg <- set.edge.attribute(base_tg, name='color', value=base_tg_edge_color)
    }

    # Analysis text output
    tg.ana.out <- paste(gsub(". $", "", parsed$out), "-analysis.info", sep="")
    sink(tg.ana.out)
    print("Task graph structure:")
    print(paste("Number of nodes =", length(V(base_tg))))
    print(paste("Number of edges =", length(E(base_tg))))
    print(paste("Number of tasks =", length(tg.data$task)))
    if(!parsed$cplengthonly)
        print(paste("Number of critical tasks =", length(tgdf$task[tgdf$on_crit_path == 1])))
    print(paste("Number of forks =", length(fork_nodes_unique)))
    print("Analysis:")
    sink()

    # Memory hierarchy utilization problem
    if("mem_hier_util" %in% colnames(tg.data))
    {
        prob_tg <- base_tg
        mem_hier_util.thresh <- 0.5
        prob_task <- subset(tg.data, mem_hier_util > mem_hier_util.thresh, select=task)
        sink(tg.ana.out, append=T)
        print(paste(length(prob_task$task), "tasks have mem_hier_util >", mem_hier_util.thresh))
        sink()
        if(!parsed$cplengthonly)
        {
            prob_task_critical <- subset(tgdf, mem_hier_util > mem_hier_util.thresh & on_crit_path == 1, select=task)
            sink(tg.ana.out, append=T)
            print(paste(length(prob_task_critical$task), "critical tasks have mem_hier_util >", mem_hier_util.thresh))
            sink()
        }
        prob_task_index <- match(as.character(prob_task$task), V(prob_tg)$name)
        prob_task_color <- get.vertex.attribute(prob_tg, name='mem_hier_util_to_color', index=prob_task_index)
        prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
        prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-memory-hierarchy-utilization.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Memory footprint problem
    if("mem_fp" %in% colnames(tg.data))
    {
        prob_tg <- base_tg
        mem_fp.thresh <- 512000
        prob_task <- subset(tg.data, mem_fp > mem_fp.thresh, select=task)
        sink(tg.ana.out, append=T)
        print(paste(length(prob_task$task), "tasks have mem_fp >", mem_fp.thresh))
        sink()
        if(!parsed$cplengthonly)
        {
            prob_task_critical <- subset(tgdf, mem_fp > mem_fp.thresh & on_crit_path == 1, select=task)
            sink(tg.ana.out, append=T)
            print(paste(length(prob_task_critical$task), "critical tasks have mem_fp >", mem_fp.thresh))
            sink()
        }
        prob_task_index <- match(as.character(prob_task$task), V(prob_tg)$name)
        prob_task_color <- get.vertex.attribute(prob_tg, name='mem_fp_to_color', index=prob_task_index)
        prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
        prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-memory-footprint.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Compute intensity problem
    if("compute_int" %in% colnames(tg.data))
    {
        prob_tg <- base_tg
        compute_int.thresh <- 2
        prob_task <- subset(tg.data, compute_int < compute_int.thresh, select=task)
        sink(tg.ana.out, append=T)
        print(paste(length(prob_task$task), "tasks have compute_int <", compute_int.thresh))
        sink()
        if(!parsed$cplengthonly)
        {
            prob_task_critical <- subset(tgdf, compute_int < compute_int.thresh & on_crit_path == 1, select=task)
            sink(tg.ana.out, append=T)
            print(paste(length(prob_task_critical$task), "critical tasks have compute_int <", compute_int.thresh))
            sink()
        }
        prob_task_index <- match(as.character(prob_task$task), V(prob_tg)$name)
        prob_task_color <- get.vertex.attribute(prob_tg, name='compute_int_to_color', index=prob_task_index)
        prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
        prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-compute-intensity.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Work deviation problem
    if("work_deviation" %in% colnames(tg.data))
    {
        prob_tg <- base_tg
        work_deviation.thresh <- 2
        prob_task <- subset(tg.data, work_deviation > work_deviation.thresh, select=task)
        sink(tg.ana.out, append=T)
        print(paste(length(prob_task$task), "tasks have work_deviation >", work_deviation.thresh))
        sink()
        if(!parsed$cplengthonly)
        {
            prob_task_critical <- subset(tgdf, work_deviation > work_deviation.thresh & on_crit_path == 1, select=task)
            sink(tg.ana.out, append=T)
            print(paste(length(prob_task_critical$task), "critical tasks have work_deviation >", work_deviation.thresh))
            sink()
        }
        prob_task_index <- match(as.character(prob_task$task), V(prob_tg)$name)
        prob_task_color <- get.vertex.attribute(prob_tg, name='work_deviation_to_color', index=prob_task_index)
        prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
        prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-work-deviation.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Parallel benefit problem
    if("parallel_benefit" %in% colnames(tg.data))
    {
        prob_tg <- base_tg
        parallel_benefit.thresh <- 1
        prob_task <- subset(tg.data, parallel_benefit < parallel_benefit.thresh, select=task)
        sink(tg.ana.out, append=T)
        print(paste(length(prob_task$task), "tasks have parallel_benefit <", parallel_benefit.thresh))
        sink()
        if(!parsed$cplengthonly)
        {
            prob_task_critical <- subset(tgdf, parallel_benefit < parallel_benefit.thresh & on_crit_path == 1, select=task)
            sink(tg.ana.out, append=T)
            print(paste(length(prob_task_critical$task), "critical tasks have parallel_benefit <", parallel_benefit.thresh))
            sink()
        }
        prob_task_index <- match(as.character(prob_task$task), V(prob_tg)$name)
        prob_task_color <- get.vertex.attribute(prob_tg, name='parallel_benefit_to_color', index=prob_task_index)
        prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
        prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-parallel-benefit.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Parallelism problem
    if(!parsed$cplengthonly && !parsed$tree)
    {
        prob_tg <- base_tg
        parallelism.thresh <- length(unique(tg.data$cpu_id))
        ranges <- which(tg_shape$counts < parallelism.thresh)
        sink(tg.ana.out, append=T)
        print(paste(length(ranges), "shape bins out of", length(tg_shape$counts), "have parallelism <", parallelism.thresh))
        sink()
        for (r in ranges)
        {
            prob_task <- subset(tgdf, rdist < tg_shape$breaks[r+1] & rdist > tg_shape$breaks[r], select=label)
            prob_task_index <- match(as.character(prob_task$label), V(prob_tg)$name)
            #prob_task_color <- get.vertex.attribute(prob_tg, name='cpu_id_to_color', index=prob_task_index)
            prob_task_color <- "#FF0000"
            prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
            prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        }
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-parallelism.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Parallelism problem (based on depth)
    if(!parsed$cplengthonly && !parsed$tree)
    {
        prob_tg <- base_tg
        parallelism.thresh <- length(unique(tg.data$cpu_id))
        prob_depths <- tg_shape_depth$depth[which(tg_shape_depth$count < parallelism.thresh)]
        prob_depth_counts <- sort(unique(tg_shape_depth$count[which(tg_shape_depth$count < parallelism.thresh)]))
        prob_depth_colors <- heat.colors(length(prob_depth_counts))
        sink(tg.ana.out, append=T)
        print(paste(length(prob_depths), "shape (depth) bins out of", length(tg_shape_depth$count), "have parallelism <", parallelism.thresh))
        sink()
        for (d in prob_depths)
        {
            prob_task <- subset(tgdf, depth==d, select=label)
            prob_task_index <- match(as.character(prob_task$label), V(prob_tg)$name)
            # Get color
            #prob_task_color <- get.vertex.attribute(prob_tg, name='cpu_id_to_color', index=prob_task_index)
            #prob_task_color <- "#000000"
            prob_task_color <- prob_depth_colors[which(prob_depth_counts == tg_shape_depth$count[which(tg_shape_depth$depth == d)])]
            prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
            prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        }
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-parallelism-depth.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-parallelism-depth.parallelism_to_color", sep="")
        write.csv(data.frame(value=prob_depth_counts, color=prob_depth_colors), tg_file_out, row.names=F)
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Scatter problem
    if("cpu_id" %in% colnames(tg.data) && !parsed$tree)
    {
        prob_tg <- base_tg
        scatter.thresh <- (length(unique(tg.data$cpu_id))/4)
        prob_fork <- V(prob_tg)[fork_nodes_index]$name[which(fork_scatter > scatter.thresh)]
        sink(tg.ana.out, append=T)
        print(paste(length(prob_fork), "forks have scatter >", scatter.thresh))
        sink()
        prob_fork_critical <- 0
        for(f in prob_fork)
        {
            f_i <- match(as.character(f), V(prob_tg)$name)
            prob_task_index <- neighbors(prob_tg, f_i, mode="out")
            if(!parsed$cplengthonly)
            {
                if(any(get.vertex.attribute(prob_tg, name='on_crit_path', index=prob_task_index) == 1))
                   prob_fork_critical <- prob_fork_critical + 1
            }
            prob_task_color <- get.vertex.attribute(prob_tg, name='cpu_id_to_color', index=prob_task_index)
            prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
            prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
            f_s <- unlist(strsplit(f, "\\."))
            f_p <- as.numeric(f_s[2])
            f_p_i <- match(as.character(f_p), V(prob_tg)$name)
            f_p_c <- get.vertex.attribute(prob_tg, name='cpu_id_to_color', index=f_p_i)
            prob_tg <- set.vertex.attribute(prob_tg, name='color', index=f_i, value=f_p_c)
            prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=f_i, value=1)
        }
        if(!parsed$cplengthonly)
        {
            sink(tg.ana.out, append=T)
            print(paste(prob_fork_critical, "critical forks have scatter >", scatter.thresh))
            sink()
        }
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-scatter.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Balance problem (work cycles)
    if("work_cycles" %in% colnames(tg.data) && !parsed$tree)
    {
        prob_tg <- base_tg
        fork_bal.thresh <- 2
        prob_fork <- V(prob_tg)[fork_nodes_index]$name[which(fork_bal_wc > fork_bal.thresh)]
        sink(tg.ana.out, append=T)
        print(paste(length(prob_fork), "forks have load balance (work cycles) >", fork_bal.thresh))
        sink()
        prob_fork_critical <- 0
        for(f in prob_fork)
        {
            f_i <- match(as.character(f), V(prob_tg)$name)
            prob_task_index <- neighbors(prob_tg, f_i, mode="out")
            if(!parsed$cplengthonly)
            {
                if(any(get.vertex.attribute(prob_tg, name='on_crit_path', index=prob_task_index) == 1))
                   prob_fork_critical <- prob_fork_critical + 1
            }
            #prob_task_color <- get.vertex.attribute(prob_tg, name='cpu_id_to_color', index=prob_task_index)
            prob_task_color <- "#FF0000"
            prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
            prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        }
        if(!parsed$cplengthonly)
        {
            sink(tg.ana.out, append=T)
            print(paste(prob_fork_critical, "critical forks have load balance (work cycles) >", fork_bal.thresh))
            sink()
        }
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-balance-work-cycles.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    # Balance problem (execution cycles)
    if("exec_cycles" %in% colnames(tg.data) && !parsed$tree)
    {
        prob_tg <- base_tg
        fork_bal.thresh <- 2
        prob_fork <- V(prob_tg)[fork_nodes_index]$name[which(fork_bal_ec > fork_bal.thresh)]
        sink(tg.ana.out, append=T)
        print(paste(length(prob_fork), "forks have load balance (execution cycles) >", fork_bal.thresh))
        sink()
        prob_fork_critical <- 0
        for(f in prob_fork)
        {
            f_i <- match(as.character(f), V(prob_tg)$name)
            prob_task_index <- neighbors(prob_tg, f_i, mode="out")
            if(!parsed$cplengthonly)
            {
                if(any(get.vertex.attribute(prob_tg, name='on_crit_path', index=prob_task_index) == 1))
                   prob_fork_critical <- prob_fork_critical + 1
            }
            #prob_task_color <- get.vertex.attribute(prob_tg, name='cpu_id_to_color', index=prob_task_index)
            prob_task_color <- "#FF0000"
            prob_tg <- set.vertex.attribute(prob_tg, name='color', index=prob_task_index, value=prob_task_color)
            prob_tg <- set.vertex.attribute(prob_tg, name='problematic', index=prob_task_index, value=1)
        }
        if(!parsed$cplengthonly)
        {
            sink(tg.ana.out, append=T)
            print(paste(prob_fork_critical, "critical forks have load balance (execution cycles) >", fork_bal.thresh))
            sink()
        }
        tg_file_out <- paste(gsub(". $", "", parsed$out), "-problem-balance-exec-cycles.graphml", sep="")
        res <- write.graph(prob_tg, file=tg_file_out, format="graphml")
        if(parsed$verbose) print(paste("Wrote file:", tg_file_out))
    }

    if(parsed$verbose) print(paste("Wrote file:", tg.ana.out))
    if(parsed$timing) toc("Analyzing graph for problems")
}

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)
