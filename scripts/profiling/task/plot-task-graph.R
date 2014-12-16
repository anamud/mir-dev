# Clean slate
rm(list=ls())

require(igraph, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)
suppressMessages(require(gdata, quietly=TRUE, warn.conflicts=FALSE))
require(optparse, quietly=TRUE)
options("scipen"=999) # big number of digits
#getOption("scipen")

# Graph element sizes
join_size <- 10
fork_size <- join_size
start_size <- 15
end_size <- start_size
task_size <- 30
task_size_mult <- 8
task_size_bins <- 10

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

# Parse args
option_list <- list(
make_option(c("--noCPE"), action="store_true", default=FALSE, help="Skip critical path enumeration. Calculate critical path length only."),
make_option(c("-v", "--verbose"), action="store_true", default=TRUE, help="Print output [default]"),
make_option(c("-q", "--quiet"), action="store_false", dest="verbose", help="Print little output"),
make_option(c("-t", "--tree"), action="store_true", default=FALSE, help="Plot task graph as tree"),
make_option(c("-d","--data"), help = "Task performance data file", metavar="FILE"),
make_option(c("-p","--pal"), default="color", help = "Color palette for graph elements [default \"%default\"]"),
make_option(c("-o","--out"), default="task-graph", help = "Output file suffix [default \"%default\"]", metavar="STRING"))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("data", where=parsed))
{
    print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}
tg.file <- parsed$data
tg.color <- parsed$pal
tg.ofilen <- parsed$out
verbo <- parsed$verbose
plot_tree <- parsed$tree
cp_len_only <- parsed$noCPE

# Read data
if(verbo) tic(type="elapsed")
tg.data <- read.csv(tg.file, header=TRUE)
if(verbo) toc("Read data")

# Remove non-sense data
if(verbo) tic(type="elapsed")
# Remove background task 
tg.data <- tg.data[!is.na(tg.data$parent),]
if(verbo) toc("Removing non-sense data")

# Set colors
if(verbo) tic(type="elapsed")
if(tg.color == "color") {
    join_color <- "#FF7F50"  # coral
    fork_color <- "#2E8B57"  # seagreen
    task_color <- "#4682B4" #steelblue
    other_color <- "#DEB887" # burlywood
    create_edge_color <- fork_color
    sync_edge_color <- join_color
    scope_edge_color <-"black" 
    cont_edge_color <- "black"
    colorf <- colorRampPalette(c("lightskyblue", "steelblue"))
} else if(tg.color == "gray") {
    join_color <- "#D3D3D3"  # light gray
    fork_color <- "#D3D3D3"  # light gray
    task_color <- "#6B6B6B"  # gray42
    other_color <- "#D3D3D3" # light gray
    create_edge_color <- "black"
    sync_edge_color <- "black"
    scope_edge_color <-"black" 
    cont_edge_color <- "black"
    colorf <- colorRampPalette(c("gray80", "gray80"))
} else {
    print("Unsupported color format. Supported formats: color, gray. Defaulting to color.")
    join_color <- "#FF7F50"  # coral
    fork_color <- "#2E8B57"  # seagreen
    task_color <- "#4682B4" #steelblue
    other_color <- "#DEB887" # burlywood
    create_edge_color <- fork_color
    sync_edge_color <- join_color
    scope_edge_color <-"black" 
    cont_edge_color <- "black"
    colorf <- colorRampPalette(c("lightskyblue", "steelblue"))
}

# Task color binning
task_color_bins <- 10
task_color_pal <- colorf(task_color_bins)
if(verbo) toc("Setting colors")

# Create node lists
if(verbo) tic(type="elapsed")
if(!plot_tree)
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
if(verbo) toc("Node list creation")

# Create graph
if(verbo) tic(type="elapsed")
if(!plot_tree)
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
if(verbo) toc("Graph creation")

# Connect parent fork to task
if(verbo) tic(type="elapsed")
tg[from=fork_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=fork_nodes, to=tg.data$task, attr='color'] <- create_edge_color
if(verbo) toc("Connect parent fork to task")

# Connect parent task to first fork
if(verbo) tic(type="elapsed")
first_forks_index <- which(grepl("f.[0-9]+.0$", fork_nodes_unique))
parent_first_forks <- as.vector(sapply(fork_nodes_unique[first_forks_index], function(x) {gsub('f.(.*)\\.+.*','\\1', x)}))
first_forks <- fork_nodes_unique[first_forks_index]
tg[to=first_forks, from=parent_first_forks, attr='kind'] <- 'scope'
tg[to=first_forks, from=parent_first_forks, attr='color'] <- scope_edge_color   
if("ins_count" %in% colnames(tg.data))
{
    tg[to=first_forks, from=parent_first_forks, attr='ins_count'] <- as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$ins_count)
    tg[to=first_forks, from=parent_first_forks, attr='weight'] <- -as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$ins_count)
}
if(verbo) toc("Connect parent to first fork")

if(!plot_tree)
{
    # Connect leaf task to join 
    if(verbo) tic(type="elapsed")
    leaf_tasks <- setdiff(tg.data$task, parent_nodes_unique)
    leaf_join_nodes <- join_nodes[match(leaf_tasks, tg.data$task)]
    tg[from=leaf_tasks, to=leaf_join_nodes, attr='kind'] <- 'sync'
    tg[from=leaf_tasks, to=leaf_join_nodes, attr='color'] <- sync_edge_color
    if("ins_count" %in% colnames(tg.data))
    {
        tg[from=leaf_tasks, to=leaf_join_nodes, attr='ins_count'] <- as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$ins_count)
        tg[from=leaf_tasks, to=leaf_join_nodes, attr='weight'] <- -as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$ins_count)
    }
    if(verbo) toc("Connect leaf task to join")

    # Connect join to next fork
    if(verbo) tic(type="elapsed")
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
    if(verbo) toc("Connect join to next fork")
} else {
    # Connect fork to next fork
    if(verbo) tic(type="elapsed")
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
    if(verbo) toc("Connect fork to next fork")

    # Connext E to last fork of task 0
    if(verbo) tic(type="elapsed")
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
    if(verbo) toc("Connect last fork of 0 to node E")
}

# Set attributes
if(verbo) tic(type="elapsed")
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
# Set size 
# Constants
tg <- set.vertex.attribute(tg, name='size', index=task_index, value=task_size)
tg <- set.vertex.attribute(tg, name='width', index=task_index, value=task_size)
tg <- set.vertex.attribute(tg, name='height', index=task_index, value=task_size)
if("ins_count" %in% colnames(tg.data))
{
    # Set size in proportion to average ins count
    #mean_val <- mean(tg.data$ins_count)
    #p_task_size <- task_size_mult * (tg.data$ins_count/mean_val)
    p_task_size <- task_size_mult * as.numeric(cut(tg.data$ins_count, task_size_bins))
    tg <- set.vertex.attribute(tg, name='ins_count_to_size', index=task_index, value=p_task_size)
}
if("work_cycles" %in% colnames(tg.data))
{
    # Set size in proportion to average ins count
    #mean_val <- mean(tg.data$work_cycles)
    #p_task_size <- task_size_mult * (tg.data$work_cycles/mean_val)
    p_task_size <- task_size_mult * as.numeric(cut(tg.data$work_cycles, task_size_bins))
    tg <- set.vertex.attribute(tg, name='work_cycles_to_size', index=task_index, value=p_task_size)
}
if("overhead_cycles" %in% colnames(tg.data))
{
    # Set size in proportion to average ins count
    #mean_val <- mean(tg.data$overhead_cycles)
    #p_task_size <- task_size_mult * (tg.data$overhead_cycles/mean_val)
    p_task_size <- task_size_mult * as.numeric(cut(tg.data$overhead_cycles, task_size_bins))
    tg <- set.vertex.attribute(tg, name='overhead_cycles_to_size', index=task_index, value=p_task_size)
}
if("exec_cycles" %in% colnames(tg.data))
{
    # Set height in proportion to exec_cycles
    exec_cycles <- tg.data$exec_cycles
    exec_cycles_norm <- 1 + ((exec_cycles - min(exec_cycles)) / (max(exec_cycles) - min(exec_cycles)))
    tg <- set.vertex.attribute(tg, name='exec_cycles_to_height', index=task_index, value=exec_cycles_norm*task_size)
}
if("work_cycles" %in% colnames(tg.data))
{
    # Set height in proportion to work_cycles
    work_cycles <- tg.data$work_cycles
    work_cycles_norm <- 1 + ((work_cycles - min(work_cycles)) / (max(work_cycles) - min(work_cycles)))
    tg <- set.vertex.attribute(tg, name='work_cycles_to_height', index=task_index, value=work_cycles_norm*task_size)
}
if("overhead_cycles" %in% colnames(tg.data))
{
    # Set height in proportion to overhead_cycles
    overhead_cycles <- tg.data$overhead_cycles
    overhead_cycles_norm <- 1 + ((overhead_cycles - min(overhead_cycles)) / (max(overhead_cycles) - min(overhead_cycles)))
    tg <- set.vertex.attribute(tg, name='overhead_cycles_to_height', index=task_index, value=overhead_cycles_norm*task_size)
}
# Set color   
# Constants
tg <- set.vertex.attribute(tg, name='color', index=task_index, value=task_color)
if("mem_fp" %in% colnames(tg.data))
{
    # Set color in proportion to average mem fp
    p_task_color <- task_color_pal[as.numeric(cut(tg.data$mem_fp, task_color_bins))]
    tg <- set.vertex.attribute(tg, name='mem_fp_to_color', index=task_index, value=p_task_color)
}
if("cpu_id" %in% colnames(tg.data))
{
    # Set color to indicate cpu_id
    cpu_ids <- tg.data$cpu_id
    cpu_colors <- rainbow(max(cpu_ids)+1)
    tg <- set.vertex.attribute(tg, name='cpu_id_to_color', index=task_index, value=cpu_colors[cpu_ids+1])
    # Write cpu_id colors for reference
    tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".cpu_id_to_color", sep="")
    if(verbo) print(paste("Writing file", tg.file.out))
    unique_cpu_ids <- unique(cpu_ids)
    sink(tg.file.out)
    print(data.frame(cpu=unique_cpu_ids, color=cpu_colors[unique_cpu_ids+1]),row.names=F)
    sink()
}
if("outl_func" %in% colnames(tg.data))
{
    # Set color to indicate outline function name
    ol_funcs <- as.character(tg.data$name)
    unique_ol_funcs <- unique(ol_funcs)
    ol_funcs_colors <- rainbow(length(unique_ol_funcs))
    tg <- set.vertex.attribute(tg, name='of_to_color', index=task_index, value=ol_funcs_colors[match(ol_funcs, unique_ol_funcs)])
    # Write outline function colors for reference
    tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".of_to_color", sep="")
    if(verbo) print(paste("Writing file", tg.file.out))
    sink(tg.file.out)
    print(data.frame(ol_funcs=unique_ol_funcs, color=ol_funcs_colors), row.names=F)
    sink()
}

# Set label and color of 'task 0'
start_index <- V(tg)$name == '0'
tg <- set.vertex.attribute(tg, name='color', index=start_index, value=other_color)
tg <- set.vertex.attribute(tg, name='label', index=start_index, value='S')
tg <- set.vertex.attribute(tg, name='size', index=start_index, value=start_size)
# Set label and color of 'task E'
end_index <- V(tg)$name == "E"
tg <- set.vertex.attribute(tg, name='color', index=end_index, value=other_color)
tg <- set.vertex.attribute(tg, name='label', index=end_index, value='E')
tg <- set.vertex.attribute(tg, name='size', index=end_index, value=end_size)

# Set fork vertex attributes 
fork_nodes_index <- startsWith(V(tg)$name, 'f')
tg <- set.vertex.attribute(tg, name='size', index=fork_nodes_index, value=fork_size)
tg <- set.vertex.attribute(tg, name='color', index=fork_nodes_index, value=fork_color)
tg <- set.vertex.attribute(tg, name='label', index=fork_nodes_index, value='^')
if("exec_cycles" %in% colnames(tg.data))
{
    # Set fork balance
    get_fork_bal <- function(fork)
    {
      # Get fork info
      fork_split <- unlist(strsplit(fork, "\\."))
      parent <- as.numeric(fork_split[2])
      join_count <- as.numeric(fork_split[3])
      
      # Get exec_cycles
      exec_cycles <- tg.data[tg.data$parent == parent & tg.data$joins_at == join_count, ]$exec_cycles
      
      # Compute balance
      bal <- max(exec_cycles)/min(exec_cycles)
      
      bal
    }
    fork_bal <- as.vector(sapply(V(tg)[fork_nodes_index]$name, get_fork_bal))
    tg <- set.vertex.attribute(tg, name='work_balance_to_size', index=fork_nodes_index, value=fork_bal*fork_size)
}

# Set join vertex attributes 
if(!plot_tree)
{
join_nodes_index <- startsWith(V(tg)$name, 'j')
tg <- set.vertex.attribute(tg, name='size', index=join_nodes_index, value=join_size)
tg <- set.vertex.attribute(tg, name='color', index=join_nodes_index, value=join_color)
tg <- set.vertex.attribute(tg, name='label', index=join_nodes_index, value='*')
}

# Set edge attributes
if("ins_count" %in% colnames(tg.data))
{
    tg <- set.edge.attribute(tg, name="weight", index=which(is.na(E(tg)$weight)), value=0)
    tg <- set.edge.attribute(tg, name="ins_count", index=which(is.na(E(tg)$ins_count)), value=0)
}
if(verbo) toc("Attribute setting")

# Check if any fork or join vertices have bad degrees
if(verbo) tic(type="elapsed")
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
if(!plot_tree)
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
if(verbo) toc("Checking for bad structure")

if("ins_count" %in% colnames(tg.data) && !plot_tree)
{
    # Simplify - DO NOT USE. Fucks up the critical path analysis.
    #if(verbo) tic(type="elapsed")
    #tg <- simplify(tg, edge.attr.comb=toString)
    #if(verbo) toc("Simplify")

    # Get critical path
    if(verbo) tic(type="elapsed")
    #Rprof("profile-critpathcalc.out")
    if(verbo) print("Calculating critical path ...")
    if(cp_len_only) 
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
      V(tg)[tsg[1]]$rpath <- tsg[1]
      # Get longest paths from root
      for(node in tsg[-1])
      {
        w <- E(tg)[to(node)]$ins_count
        d <- V(tg)[nei(node,mode="in")]$rdist
        wd <- w+d
        mwd <- max(wd)
        V(tg)[node]$rdist <- mwd
        mwdn <- as.vector(V(tg)[nei(node,mode="in")])[match(mwd,wd)]
        V(tg)[node]$rpath <- list(c(unlist(V(tg)[mwdn]$rpath), node))
        if(verbo) {ctr <- ctr + 1; setTxtProgressBar(pb, ctr);}
      }
      ## Longest path is the largest root distance
      lpl <- max(V(tg)$rdist)
      # Enumerate longest path
      lpm <- unlist(V(tg)[match(lpl,V(tg)$rdist)]$rpath)
      V(tg)$on_crit_path <- 0
      tg <- set.vertex.attribute(tg, name="on_crit_path", index=lpm, value=1)
      if(verbo) {ctr <- ctr + 1; setTxtProgressBar(pb, ctr);}
      close(pb)
    }
    #Rprof(NULL)
    if(verbo) toc("Critical path calculation")
    
    # Calculate and write info
    if(verbo) tic(type="elapsed")
    tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".info", sep="")
    if(verbo) print(paste("Writing file", tg.file.out))
    sink(tg.file.out)
    print("span (critical path)")
    print(lpl)
    print("work")
    work <- sum(as.numeric(tg.data$ins_count))
    print(work)
    print("parallelism")
    print(work/lpl)
    sink()

    if(!cp_len_only) 
    {
        # Clear rpath since dot/table writing complains 
        tg <- remove.vertex.attribute(tg,"rpath")

        # Write out shape
        tg.file.out <- paste(gsub(". $", "", tg.ofilen), "-shape.pdf", sep="")
        if(verbo) print(paste("Writing file", tg.file.out))
        pdf(tg.file.out)
        hist(get.data.frame(tg, what="vertices")$rdist, freq=T, main="Histogram of distance from start vertex", xlab="Distance", ylab="Tasks")
        dev.off()
    }
    if(verbo) toc("Calc and write info")
} else {
    if(verbo) tic(type="elapsed")
    tg <- simplify(tg, remove.multiple=T, remove.loops=T)
    if(verbo) toc("Simplify")
}

# Write dot file
if(verbo) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".dot", sep="")
if(verbo) print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="dot")
if(verbo) toc("Write dot")

# Write gml file
if(verbo) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".graphml", sep="")
if(verbo) print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="graphml")
if(verbo) toc("Write graphml")

# # Write pdf file
# if(verbo) tic(type="elapsed")
# lyt <- layout.fruchterman.reingold(tg,niter=500,area=vcount(tg)^2,coolexp=3,repulserad=vcount(tg)^3,maxdelta=vcount(tg))
# tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".pdf", sep="")
# if(verbo) print(paste("Writing file", tg.file.out))
# pdf(file=tg.file.out)
# plot(tg, layout=lyt)
# dev.off()
# if(verbo) toc("Write pdf")

# Write adjacency matrix file
if(verbo) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".adjmat", sep="")
if(verbo) print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(get.adjacency(tg,names=T))
sink()
if(verbo) toc("Write adjacency matrix")

# Write edgelist file
if(verbo) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".edgelist", sep="")
if(verbo) print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(get.edgelist(tg, names=T))
sink()
if(verbo) toc("Write edgelist")

# Write node attributes 
if(verbo) tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.ofilen), ".nodeattr", sep="")
if(verbo) print(paste("Writing file", tg.file.out))
write.table(get.data.frame(tg, what="vertices"), sep=",", file=tg.file.out)
if(verbo) toc("Write node attributes")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)
