# Clean slate
rm(list=ls())

require(igraph, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)
require(gdata, quietly=TRUE, warn.conflicts=FALSE)

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

tic(type="elapsed")
### Read data
args <- commandArgs(TRUE)
if(length(args) != 2) quit("no", 1)
tg.file <- args[1]
tg.data <- read.csv(tg.file, header=TRUE)
tg.color <- args[2]
toc("Read data")

tic(type="elapsed")
# Set colors
if(tg.color == "color") {
    fork_color <- "#2E8B57"  # seagreen
    other_color <- "#DEB887" # burlywood
    create_edge_color <- fork_color
} else if(tg.color == "gray") {
    fork_color <- "#D3D3D3"  # light gray
    other_color <- "#D3D3D3" # light gray
    create_edge_color <- "black"
} else {
    print("Unsupported color format. Supported formats: color, gray. Defaulting to color.")
    fork_color <- "#2E8B57"  # seagreen
    other_color <- "#DEB887" # burlywood
    create_edge_color <- fork_color
}

scope_edge_color <-"black" 
cont_edge_color <- "black"

toc("Setting colors")

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

tic(type="elapsed")
# Connect parent fork to task node
tg[from=fork_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=fork_nodes, to=tg.data$task, attr='color'] <- create_edge_color
toc("Connect parent fork to task node")

tic(type="elapsed")
first_forks_index <- which(grepl("f.[0-9]+.0$", fork_nodes_unique))
parent_first_forks <- as.vector(sapply(fork_nodes_unique[first_forks_index], function(x) {gsub('f.(.*)\\.+.*','\\1', x)}))
first_forks <- fork_nodes_unique[first_forks_index]
tg[to=first_forks, from=parent_first_forks, attr='kind'] <- 'scope'
tg[to=first_forks, from=parent_first_forks, attr='color'] <- scope_edge_color   
toc("Connect parent to first fork")

tic(type="elapsed")
# Connect fork to next fork
#Rprof("profile-forktonext.out")
find_next_fork <- function(node)
{
  #print(paste('Processing node',node, sep=" "))
  
  # Get node info
  node_split <- unlist(strsplit(node, "\\."))
  parent <- as.numeric(node_split[2])
  join_count <- as.numeric(node_split[3])
  
  # Find next work
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
 
tic(type="elapsed")
# Common vertex attributes
V(tg)$label <- V(tg)$name

# Set task vertex attributes
task_index <- match(as.character(tg.data$task), V(tg)$name)
# Set annotations
for (annot in c('exec_cycles','child_number','num_children','core_id'))
{
    values <- tg.data[which(tg.data$task %in% V(tg)[task_index]$name),annot]
    tg <- set.vertex.attribute(tg, name=annot, index=task_index, value=values)
}
# Set width to constant
tg <- set.vertex.attribute(tg, name='size', index=task_index, value=task_size)
# Set color to indicate core_id
core_ids <- tg.data[which(tg.data$task %in% V(tg)[task_index]$name),]$core_id
core_colors <- rainbow(max(core_ids)+1)
tg <- set.vertex.attribute(tg, name='color', index=task_index, value=core_colors[core_ids+1])
# Write core_id colors for reference
tg.file.out <- paste(gsub(". $", "", tg.file), ".colormap", sep="")
print(paste("Writing file", tg.file.out))
unique_core_ids <- unique(core_ids)
sink(tg.file.out)
print(data.frame(core=unique_core_ids, color=core_colors[unique_core_ids+1]),row.names=F)
sink()

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

# Make fork nodes small
fork_nodes_index <- startsWith(V(tg)$name, 'f')
tg <- set.vertex.attribute(tg, name='size', index=fork_nodes_index, value=fork_size)
tg <- set.vertex.attribute(tg, name='color', index=fork_nodes_index, value=fork_color)
tg <- set.vertex.attribute(tg, name='label', index=fork_nodes_index, value='^')
toc("Attribute setting")

tic(type="elapsed")
# Write dot file
tg.file.out <- paste(gsub(". $", "", tg.file), ".dot", sep="")
print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="dot")
toc("Write dot")

tic(type="elapsed")
# Write gml file
tg.file.out <- paste(gsub(". $", "", tg.file), ".graphml", sep="")
print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="graphml")
toc("Write graphml")

# tic(type="elapsed")
# # Write pdf file
# lyt <- layout.fruchterman.reingold(tg,niter=500,area=vcount(tg)^2,coolexp=3,repulserad=vcount(tg)^3,maxdelta=vcount(tg))
# tg.file.out <- paste(gsub(". $", "", tg.file), ".pdf", sep="")
# print(paste("Writing file", tg.file.out))
# pdf(file=tg.file.out)
# plot(tg, layout=lyt)
# dev.off()
# toc("Write pdf")

tic(type="elapsed")
# Write adjacency matrix file
tg.file.out <- paste(gsub(". $", "", tg.file), ".adjm", sep="")
print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(get.adjacency(tg,names=T))
sink()
toc("Write adjacency matrix")

tic(type="elapsed")
# Write edgelist file
tg.file.out <- paste(gsub(". $", "", tg.file), ".edgelist", sep="")
print(paste("Writing file", tg.file.out))
sink(tg.file.out)
print(get.edgelist(tg, names=T))
sink()
toc("Write edgelist")

# Warn
warnings()
