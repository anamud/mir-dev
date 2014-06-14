# Clean slate
rm(list=ls())

require(igraph, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)
require(gdata, quietly=TRUE, warn.conflicts=FALSE)

# Sizes
join_size <- 10
fork_size <- join_size
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
    join_color <- "#FF7F50"  # coral
    fork_color <- "#2E8B57"  # seagreen
    start_color <- "#DEB887" # burlywood
    create_edge_color <- fork_color
    sync_edge_color <- join_color
    colorf <- colorRampPalette(c("lightskyblue", "steelblue"))
} else if(tg.color == "gray") {
    join_color <- "#FFFAFA"  # snow
    fork_color <- "#FFFAFA"  # snow
    start_color <- "#FFFAFA" # snow
    create_edge_color <- "black"
    sync_edge_color <- "black"
    colorf <- colorRampPalette(c("gray80", "gray80"))
} else {
    print("Unsupported color format. Supported formats: color, gray. Defaulting to color.")
    join_color <- "#FF7F50"  # coral
    fork_color <- "#2E8B57"  # seagreen
    start_color <- "#DEB887" # burlywood
    create_edge_color <- fork_color
    sync_edge_color <- join_color
    colorf <- colorRampPalette(c("lightskyblue", "steelblue"))
}

scope_edge_color <-"black" 
cont_edge_color <- "black"

# Task colors
task_color_bins <- 10
task_color_pal <- colorf(task_color_bins)

toc("Setting colors")

tic(type="elapsed")
# Increment join node pass counts 
# The pass count starts at 0
tg.data$joins_at_plus_one <- tg.data$joins_at + 1

# Create join node list
join_nodes <- mapply(function(x, y, z) {paste('j', x, y, sep='.')}, x=tg.data$parent, y=tg.data$joins_at_plus_one)
join_nodes_unique <- unique(unlist(join_nodes, use.names=FALSE))

# Create parent nodes list
parent_nodes_unique <- unique(tg.data$parent)

# Create fork nodes list
fork_nodes <- mapply(function(x, y, z) {paste('f', x, y, sep='.')}, x=tg.data$parent, y=tg.data$joins_at)
fork_nodes_unique <- unique(unlist(fork_nodes, use.names=FALSE))
toc("Node list creation")

# Create graph
tic(type="elapsed")
tg <- graph.empty(directed=TRUE) + vertices('E', 
                                            unique(c(join_nodes_unique, 
                                                     fork_nodes_unique, 
                                                     parent_nodes_unique, 
                                                     tg.data$task)))
toc("Graph creation")

tic(type="elapsed")
# Connect parent to task node
tg[from=fork_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=fork_nodes, to=tg.data$task, attr='color'] <- create_edge_color
toc("Connect parent to task node")

tic(type="elapsed")
first_forks_index <- which(grepl("f.[0-9]+.0$", fork_nodes_unique))
parent_first_forks <- as.vector(sapply(fork_nodes_unique[first_forks_index], function(x) {gsub('f.(.*)\\.+.*','\\1', x)}))
first_forks <- fork_nodes_unique[first_forks_index]
tg[to=first_forks, from=parent_first_forks, attr='kind'] <- 'scope'
tg[to=first_forks, from=parent_first_forks, attr='color'] <- scope_edge_color   
tg[to=first_forks, from=parent_first_forks, attr='weight'] <- -as.numeric(tg.data[match(parent_first_forks, tg.data$task),]$execution_time)
toc("Connect parent to first fork")

tic(type="elapsed")
# Connect leaf task to join node
leaf_tasks <- setdiff(tg.data$task, parent_nodes_unique)
leaf_join_nodes <- join_nodes[match(leaf_tasks, tg.data$task)]
tg[from=leaf_tasks, to=leaf_join_nodes, attr='kind'] <- 'sync'
tg[from=leaf_tasks, to=leaf_join_nodes, attr='color'] <- sync_edge_color
tg[from=leaf_tasks, to=leaf_join_nodes, attr='weight'] <- -as.numeric(tg.data[match(leaf_tasks, tg.data$task),]$execution_time)
toc("Connect leaf task to join node")

tic(type="elapsed")
# Connect join to another fork
#Rprof("profile-jointonext.out")
#for(node in join_nodes_unique)
find_next_fork <- function(node)
{
  #print(paste('Processing node',node, sep=" "))
  
  # Get node info
  node_split <- unlist(strsplit(node, "\\."))
  parent <- as.numeric(node_split[2])
  join_count <- as.numeric(node_split[3])
  
  # Find next work
  next_fork <- paste('f', as.character(parent), as.character(join_count), sep=".")
  if(is.na(match(next_fork, fork_nodes_unique)) == F)
  {
    # Connect to next fork
    next_fork <- next_fork
  }
  else 
  {
    # Next fork is part of grandfather
    parent_index <- match(parent, tg.data$task)
    gfather <- tg.data[parent_index,]$parent
    gfather_join <- paste('j', as.character(gfather), as.character(tg.data[parent_index,]$joins_at_plus_one), sep=".")

    if(is.na(match(gfather_join, join_nodes_unique)) == F)
    {
      # Connect to grandfather's join     
      next_fork <- gfather_join
    }
    else
    { 
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
toc("Connect join to next fork")

tic(type="elapsed")
# Common vertex attributes
V(tg)$label <- V(tg)$name

# Set task vertex attributes
task_index <- match(as.character(tg.data$task), V(tg)$name)
# Set width to constant
tg <- set.vertex.attribute(tg, name='size', index=task_index, value=task_size)
# Set color to indicate core_id
tg <- set.vertex.attribute(tg, name='color', index=task_index, value='red')

# Set label and color of 'task 0'
start_index <- V(tg)$name == '0'
tg <- set.vertex.attribute(tg, name='color', index=start_index, value=start_color)
tg <- set.vertex.attribute(tg, name='label', index=start_index, value='S')
tg <- set.vertex.attribute(tg, name='size', index=start_index, value=start_size)

# Set label and color of 'task E'
end_index <- V(tg)$name == "E"
tg <- set.vertex.attribute(tg, name='color', index=end_index, value=start_color)
tg <- set.vertex.attribute(tg, name='label', index=end_index, value='E')
tg <- set.vertex.attribute(tg, name='size', index=end_index, value=end_size)

# Make join and fork nodes small
join_nodes_index <- startsWith(V(tg)$name, 'j')
fork_nodes_index <- startsWith(V(tg)$name, 'f')
tg <- set.vertex.attribute(tg, name='size', index=join_nodes_index, value=join_size)
tg <- set.vertex.attribute(tg, name='color', index=join_nodes_index, value=join_color)
tg <- set.vertex.attribute(tg, name='label', index=join_nodes_index, value='*')
tg <- set.vertex.attribute(tg, name='size', index=fork_nodes_index, value=fork_size)
tg <- set.vertex.attribute(tg, name='color', index=fork_nodes_index, value=fork_color)
tg <- set.vertex.attribute(tg, name='label', index=fork_nodes_index, value='^')

# Set edge attributes
# Zero weight
tg <- set.edge.attribute(tg, name="weight", index=which(is.na(E(tg)$weight)), value=0)
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
