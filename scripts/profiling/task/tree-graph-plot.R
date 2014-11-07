# Clean slate
rm(list=ls())

require(igraph, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)
require(gdata, quietly=TRUE, warn.conflicts=FALSE)
options("scipen"=999) # big number of digits
#getOption("scipen")

# Sizes
fork_size <- 10
start_size <- 15
end_size <- start_size
task_size <- 30

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

### Read data
tic(type="elapsed")
args <- commandArgs(TRUE)
if(length(args) != 2) 
{
   print("Arguments missing! Please provide task graph(arg1) and color(arg2, one of {color,gray})")
   quit("no", 1)
}
tg.file <- args[1]
tg.data <- read.csv(tg.file, header=TRUE)
tg.color <- args[2]

toc("Read data")

# Set colors
tic(type="elapsed")
if(tg.color == "color") {
    fork_color <- "#2E8B57"  # seagreen
    other_color <- "#DEB887" # burlywood
} else if(tg.color == "gray") {
    fork_color <- "#D3D3D3"  # light gray
    other_color <- "#D3D3D3" # light gray
} else {
    print("Unsupported color format. Supported formats: color, gray. Defaulting to color.")
    fork_color <- "#2E8B57"  # seagreen
    other_color <- "#DEB887" # burlywood
}

create_edge_color <- "#D3D3D3"  # light gray
scope_edge_color <- "#D3D3D3"  # light gray
cont_edge_color <- "#D3D3D3"  # light gray

toc("Setting colors")

# Create node lists
tic(type="elapsed")
# Create parent nodes list
parent_nodes_unique <- unique(tg.data$parent)

# Create fork nodes list
fork_nodes <- mapply(function(x, y, z) {paste('f', x, y, sep='.')}, x=tg.data$parent, y=tg.data$joins_at)
fork_nodes_unique <- unique(unlist(fork_nodes, use.names=FALSE))
toc("Node list creation")

# Create graph
tic(type="elapsed")
tg <- graph.empty(directed=TRUE) + vertices('E', 
                                            unique(c(fork_nodes_unique, 
                                                     parent_nodes_unique, 
                                                     tg.data$task)))
toc("Graph creation")

# Connect parent fork to task
tic(type="elapsed")
tg[from=fork_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=fork_nodes, to=tg.data$task, attr='color'] <- create_edge_color
toc("Connect parent fork to task")

# Connect parent task to first fork
tic(type="elapsed")
first_forks_index <- which(grepl("f.[0-9]+.0$", fork_nodes_unique))
parent_first_forks <- as.vector(sapply(fork_nodes_unique[first_forks_index], function(x) {gsub('f.(.*)\\.+.*','\\1', x)}))
first_forks <- fork_nodes_unique[first_forks_index]
tg[to=first_forks, from=parent_first_forks, attr='kind'] <- 'scope'
tg[to=first_forks, from=parent_first_forks, attr='color'] <- scope_edge_color   
toc("Connect parent task to first fork")

# Connect fork to next fork
tic(type="elapsed")
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
  }
  else
  {
    # Connect to myself (self-loop)
    next_fork <- node
  }
  next_fork
}
next_forks <- as.vector(sapply(fork_nodes_unique, find_next_fork))
tg[from=fork_nodes_unique, to=next_forks, attr='kind'] <- 'continue'
tg[from=fork_nodes_unique, to=next_forks, attr='color'] <- cont_edge_color
#Rprof(NULL)
toc("Connect fork to next fork")

# Connext E to last fork of task 0
tic(type="elapsed")
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
toc("Connect last fork of 0 to node E")

tic(type="elapsed")
tg <- simplify(tg, remove.multiple=T, remove.loops=T)
toc("Simplify")
 
# Set attributes
tic(type="elapsed")
# Common vertex attributes
V(tg)$label <- V(tg)$name

# Set task vertex attributes
task_index <- match(as.character(tg.data$task), V(tg)$name)

# Set task annotations
for (annot in colnames(tg.data))
{
    values <- as.character(tg.data[,annot])
    tg <- set.vertex.attribute(tg, name=annot, index=task_index, value=values)
}

# Set task width and size to constant
tg <- set.vertex.attribute(tg, name='size', index=task_index, value=task_size)
tg <- set.vertex.attribute(tg, name='width', index=task_index, value=task_size)

# Set task length to exec_cycles
exec_cycles <- tg.data$exec_cycles
exec_cycles_norm <- 1 + ((exec_cycles - min(exec_cycles)) / (max(exec_cycles) - min(exec_cycles)))
tg <- set.vertex.attribute(tg, name='length', index=task_index, value=exec_cycles_norm*task_size)

# Map core_id to task color
core_ids <- tg.data$core_id
core_colors <- rainbow(max(core_ids)+1)
tg <- set.vertex.attribute(tg, name='core_id_as_color', index=task_index, value=core_colors[core_ids+1])
# Write core_id colors for reference
tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.core_id_to_color", sep="")
print(paste("Writing file", tg.file.out))
unique_core_ids <- unique(core_ids)
sink(tg.file.out)
print(data.frame(core=unique_core_ids, color=core_colors[unique_core_ids+1]),row.names=F)
sink()

# Map outline function name to task color
ol_funcs <- as.character(tg.data$name)
unique_ol_funcs <- unique(ol_funcs)
ol_funcs_colors <- rainbow(length(unique_ol_funcs))
tg <- set.vertex.attribute(tg, name='ofunc_as_color', index=task_index, value=ol_funcs_colors[match(ol_funcs, unique_ol_funcs)])
# Write outline function colors for reference
tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.outline_func_to_color", sep="")
print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(data.frame(ol_funcs=unique_ol_funcs, color=ol_funcs_colors), row.names=F)
sink()

# Set label and color of task 0
start_index <- V(tg)$name == '0'
tg <- set.vertex.attribute(tg, name='color', index=start_index, value=other_color)
tg <- set.vertex.attribute(tg, name='label', index=start_index, value='S')
tg <- set.vertex.attribute(tg, name='size', index=start_index, value=start_size)

# Set label and color of task E
end_index <- V(tg)$name == "E"
tg <- set.vertex.attribute(tg, name='color', index=end_index, value=other_color)
tg <- set.vertex.attribute(tg, name='label', index=end_index, value='E')
tg <- set.vertex.attribute(tg, name='size', index=end_index, value=end_size)

# Make forks small
fork_nodes_index <- startsWith(V(tg)$name, 'f')
tg <- set.vertex.attribute(tg, name='size', index=fork_nodes_index, value=fork_size)
tg <- set.vertex.attribute(tg, name='color', index=fork_nodes_index, value=fork_color)
tg <- set.vertex.attribute(tg, name='label', index=fork_nodes_index, value='^')

# Set fork balance
get_fork_bal <- function(fork)
{
  #print(paste('Processing fork', fork, sep=" "))
  
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
tg <- set.vertex.attribute(tg, name='work_balance', index=fork_nodes_index, value=fork_bal*fork_size)
toc("Attribute setting")

# Write dot file
tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.dot", sep="")
print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="dot")
toc("Write dot")

# Write gml file
tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.graphml", sep="")
print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="graphml")
toc("Write graphml")

## Write pdf file
#tic(type="elapsed")
#lyt <- layout.reingold.tilford(tg)
#tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.pdf", sep="")
#print(paste("Writing file", tg.file.out))
#pdf(file=tg.file.out)
#plot(tg, layout=lyt)
#dev.off()
#toc("Write pdf")

# Write adjacency matrix file
tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.adjm", sep="")
print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(get.adjacency(tg,names=T))
sink()
toc("Write adjacency matrix")

# Write edgelist file
tic(type="elapsed")
tg.file.out <- paste(gsub(". $", "", tg.file), "-tree.edgelist", sep="")
print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(get.edgelist(tg, names=T))
sink()
toc("Write edgelist")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)
