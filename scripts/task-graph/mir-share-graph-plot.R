require(igraph, quietly=TRUE)
#require(tcltk, quietly=TRUE)
require(RColorBrewer, quietly=TRUE)

# Set defaults
sg_ignore_weight_lt <- 0
sg_ignore_weight_gt <- Inf

# Read args
args <- commandArgs(TRUE)
if(length(args) < 1) quit("no", 1)
sg_file <- args[1]
if(length(args) > 1) sg_ignore_weight_lt <- as.integer(args[2])
if(length(args) > 2) sg_ignore_weight_gt <- as.integer(args[3])

# Read data
sg_data <- read.csv(sg_file, header=FALSE)
sg_data_len <- length(sg_data)

# Create graph
sg <- graph(c(), n=sg_data_len, directed=FALSE)
for(from in c(1:sg_data_len))
{
  for(to in c(1:from))
  {
    if(from != to)
    {
      weight <- sg_data[from, to]
      if(!is.na(weight))
      {
        # Make sure weight is between limits
        if(weight > sg_ignore_weight_lt & weight < sg_ignore_weight_gt) 
        {
          sg <- add.edges(sg, c(from, to), attr=list(weight=weight))
        }
      }
    }
  }
}

# Assign edge colors
edge_weights <- unique(E(sg)$weight)
edge_colors <- brewer.pal(length(edge_weights), 'Pastel1')
ctr <- 1
for(w in edge_weights)
{
  E(sg)[weight == w]$color <- edge_colors[ctr]
  ctr <- ctr + 1
}

# Assign other properties to graph
res <- autocurve.edges(sg)

# Plot graph
#tkplot(sg)
#plot.igraph(sg, layout=layout.auto, edge.label=E(sg)$weight, edge.curved=TRUE, edge.label.family='mono', edge.label.font=1)
#plot.igraph(sg, layout=layout.auto, edge.width=E(sg)$weight/max(E(sg)$weight), edge.curved=TRUE)

# Write dot file
sg_file_out <- paste(gsub(". $", "", sg_file), ".dot", sep="")
print(paste("Writing file", sg_file_out))
res <- write.graph(sg, file=sg_file_out, format="dot")
            
# Quit
quit("no", 0)