require(igraph, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)
require(gdata, quietly=TRUE, warn.conflicts=FALSE)

# Colors
tg.colors <- brewer.pal(6, 'Dark2')
task_color <- tg.colors[1]
join_color <- tg.colors[2]
create_edge_color <- tg.colors[3]
sync_edge_color <- tg.colors[4]
scope_edge_color <- tg.colors[5]
cont_edge_color <- tg.colors[6]

### Read data
args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
tg.file <- args[1]
tg.data <- read.csv(tg.file, header=TRUE)

### Nodes
parent_nodes <- mapply(function(x, y) {ifelse(y>0, paste(x, y, sep='.'), paste(x))}, x=tg.data$parent, y=tg.data$join_node_pass_count)
join_nodes <- mapply(function(x, y, z) {paste('j', x, y, z, sep='.')}, x=tg.data$join_node, y=tg.data$join_node_parent, z=tg.data$join_node_pass_count)
all_nodes <- c(tg.data$task, parent_nodes, join_nodes)
all_nodes_unique <- unique(unlist(all_nodes, use.names=FALSE))

### Create graph
tg <- graph.empty(directed=TRUE) + vertices(all_nodes_unique)

### Node attributes
join_nodes_index <- startsWith(V(tg)$name, 'j')
tg <- set.vertex.attribute(tg, name='color', index=join_nodes_index, value=join_color)
tg <- set.vertex.attribute(tg, name='color', index=!join_nodes_index, value=task_color)
V(tg)$label <- V(tg)$name

### Edges and attributes
unique_parent_nodes <- unique(parent_nodes)
unique_join_nodes <- unique(join_nodes)
tg[from=parent_nodes, to=tg.data$task, attr='kind'] <- 'create'
tg[from=parent_nodes, to=tg.data$task, attr='color'] <- create_edge_color
tg[from=tg.data$task, to=join_nodes, attr='kind'] <- 'sync'
tg[from=tg.data$task, to=join_nodes, attr='color'] <- sync_edge_color

#print('Parent child done!')

for(node in unique_join_nodes)
{
  #print(paste('Processing node',node, sep=" "))
  s <- unlist(strsplit(node, "\\."));
  
  # Connect potential continue to join
  pot_cont <- paste(s[3], as.character(as.numeric(s[4]) + 1), sep=".")
  if(any(unique_parent_nodes == pot_cont)) 
    cont <- pot_cont
  else 
    cont <- s[3]
  #print(paste('Connecting',node,'to continue',cont,sep=" "));
  tg[from=node, to=cont, attr='kind'] <- 'continue';
  tg[from=node, to=cont, attr='color'] <- cont_edge_color
  
  # Connect parent
  join_count <- as.numeric(s[4])
  if( join_count != 0)
    par <- paste(s[3], as.character(join_count), sep=".")
  else
    par <- s[3]
  #print(paste('Connecting parent',par,'to',node,sep=" "));
  tg[from=par, to=node, attr='kind'] <- 'scope';
  tg[from=par, to=node, attr='color'] <- scope_edge_color
}

#print('Join continue and parent done!')

### Simplify
tg <- simplify(tg, edge.attr.comb=toString)

### Remove vertex join labels
V(tg)[join_nodes_index]$label <- "J"

### Information (Not needed anymore)
### This is now written by mir-task-plot-info.R
#sink(paste(gsub(". $", "", tg.file), ".info", sep=""))
#cat("num_tasks: ")
#cat(length(V(tg)[!grepl("\\.", V(tg)$name)]))
#cat("\n")
#cat("join_degree_summary: ")
#cat(summary(degree(tg, unique_join_nodes, mode="in")))
#cat("\n")
#cat("fork_degree_summary: ")
#cat(summary(degree(tg, unique_parent_nodes, mode="out")))
#cat("\n")
#sink()

### Write dot file
#tg.file.out <- paste(gsub(". $", "", tg.file), ".dot", sep="")
#print(paste("Writing file", tg.file.out))
#res <- write.graph(tg, file=tg.file.out, format="dot")

### Write pdf file
#lyt <- layout.fruchterman.reingold(tg,niter=500,area=vcount(tg)^2,coolexp=3,repulserad=vcount(tg)^3,maxdelta=vcount(tg))
#tg.file.pdf.out <- paste(gsub(". $", "", tg.file), ".pdf", sep="")
#pdf(file=tg.file.pdf.out)
#plot(tg, layout=lyt)
#dev.off()

### Quit
quit("no", 0)
