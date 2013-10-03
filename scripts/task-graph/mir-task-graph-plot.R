require(igraph, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)
require(gdata, quietly=TRUE, warn.conflicts=FALSE)

args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
tg.file <- args[1]
tg.data <- read.csv(tg.file, header=TRUE)

### Nodes
parent_nodes <- mapply(function(x, y) {ifelse(y>0, paste(x, y, sep='.'), paste(x))}, x=tg.data$parent, y=tg.data$join_node_pass_count)
join_nodes <- mapply(function(x, y, z) {paste('j', x, y, z, sep='.')}, x=tg.data$join_node, y=tg.data$join_node_parent, z=tg.data$join_node_pass_count)
all_nodes <- c(tg.data$task, parent_nodes, join_nodes)
all_nodes_unique <- unique(unlist(all_nodes, use.names=FALSE))

tg <- graph.empty(directed=TRUE) + vertices(all_nodes_unique)

### Node attributes
tg.colors <- brewer.pal(3, 'Dark2')
join_nodes_index <- startsWith(V(tg)$name, 'j')
tg <- set.vertex.attribute(tg, name='color', index=join_nodes_index, value=tg.colors[2])
tg <- set.vertex.attribute(tg, name='color', index=!join_nodes_index, value=tg.colors[1])
V(tg)$label <- V(tg)$name

### Edges and attributes
tg[from=parent_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=parent_nodes, to=join_nodes, attr='kind'] <- 'continue'
tg[from=tg.data$task, to=join_nodes, attr='kind'] <- 'sync'
unique_parent_nodes <- unique(parent_nodes)
unique_join_nodes <- unique(join_nodes)
for(node in unique_join_nodes)
{
  s <- unlist(strsplit(node, "\\."));
  cont <- paste(s[3], as.character(as.numeric(s[4]) + 1), sep=".")
  if(any(unique_parent_nodes == cont))
  {
    #print(paste('Connecting',node,'to',cont,sep=" "));     
    tg[from=node, to=cont, attr='kind'] <- 'continue';
  }
}

### Simplify
tg <- simplify(tg)

# Write dot file
tg.file.out <- paste(gsub(". $", "", tg.file), ".dot", sep="")
print(paste("Writing file", tg.file.out))
res <- write.graph(tg, file=tg.file.out, format="dot")

### Information
sink(paste(gsub(". $", "", tg.file), ".info", sep=""))
cat("num_tasks: ")
cat(length(V(tg)[!grepl("\\.", V(tg)$name)]))
cat("\n")
cat("join_degree_summary: ")
cat(summary(degree(tg, V(tg)[join_nodes_index], mode="in")))
cat("\n")
cat("fork_degree_summary: ")
cat(summary(degree(tg, V(tg)[!join_nodes_index], mode="out")))
cat("\n")
sink()

# Quit
quit("no", 0)
