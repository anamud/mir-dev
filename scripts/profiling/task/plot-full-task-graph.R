# Clean slate
rm(list=ls())

## Treat warnings as errors
#options(warn=2)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Library
suppressMessages(library(data.table, quietly=TRUE, warn.conflicts=FALSE))
library(igraph, quietly=TRUE)

# Read data
tic(type="elapsed")
tg_file_in <- "/home/ananya/mir-dev/programs/omp/generic/mir-task-stats"
print(paste("Reading file:", tg_file_in, sep=" "))
tg_data <- fread(tg_file_in, header=TRUE)
toc("Read data")

# Join frequeny
tic(type="elapsed")
join_freq <- tg_data %>% group_by(parent,joins_at) %>% summarise(count = n())
toc("Compute join frequency")

# Funciton returns fragment chain for input task
fragmentize <- function (task, num_children, parent, child_number, joins_at)
{
    num_fragments <- 1
    # Connect to parent fork
    new_fragment <- paste(task, num_fragments, sep='.')
    parent_fork <- paste('f', parent, child_number, sep='.')
    edg <- c(parent_fork, new_fragment)
    last_fragment <- new_fragment
    # Connect fragments
    if(num_children > 0)
    {
        joins <- join_freq[parent == task]$count
        joins_ind <- 1
        num_forks <- 0
        num_joins <- 0
        for(i in seq(1,num_children))
        {
            num_fragments <- num_fragments + 1
            new_fragment <- paste(task, num_fragments, sep='.')
            fork <- paste('f', task, i, sep=".")
            edg <- c(edg, last_fragment, fork, fork, new_fragment)
            num_forks <- num_forks + 1
            last_fragment <- new_fragment
            if(num_forks == joins[joins_ind])
            {
                num_fragments <- num_fragments + 1
                new_fragment <- paste(task, num_fragments, sep='.')
                join <- paste('j', task, num_joins, sep=".")
                edg <- c(edg, last_fragment, join, join, new_fragment)
                num_joins <- num_joins + 1
                joins_ind <- joins_ind + 1
                last_fragment <- new_fragment
            }
        }
    }
    # Connect to parent join
    parent_join <- paste('j', parent, joins_at, sep='.')
    edg <- c(edg, last_fragment, parent_join)
    return(edg) 
}

# Apply fragmentize on all tasks
tic(type="elapsed")
tg_edges <- unlist(mapply(fragmentize, 
                          task=tg_data$task, num_children=tg_data$num_children, 
                          parent=tg_data$parent, child_number=tg_data$child_number, 
                          joins_at=tg_data$joins_at))
tg_edges <- matrix(tg_edges, nc=2, byrow=TRUE)
toc("Fragmentize")

# Construct graph
tic(type="elapsed")
tg <- graph.edgelist(tg_edges, directed = TRUE)
toc("Construct graph")

# Visual attributes
node_min_size <- 10
node_max_size <- 50
  
# Assign weights to nodes
## Read graph as table
tic(type="elapsed")
tg_vertices <- data.table(node=get.data.frame(tg, what="vertices")$name, exec_cycles=0, kind="fragment")
pesky_factors <- sapply(tg_vertices, is.factor)
tg_vertices[pesky_factors] <- lapply(tg_vertices[pesky_factors], as.character)
## Get helpful indexes
fork_ind <- with(tg_vertices, grepl("^f.[0-9]+.[0-9]+$", node))
join_ind <- with(tg_vertices, grepl("^j.[0-9]+.[0-9]+$", node))
fragment_ind <- with(tg_vertices, grepl("^[0-9]+.[0-9]+$", node))
start_ind <- with(tg_vertices, grepl("^f.0.1$", node))
end_ind <- with(tg_vertices, grepl("^j.0.0$", node))
## Assign kind
tg_vertices[fork_ind]$kind <- "fork"
tg_vertices[join_ind]$kind <- "join"
tg <- set.vertex.attribute(tg, name="kind", index=V(tg), value=tg_vertices$kind)
toc("Assign other weights")
## Assign exec cycles
compute_fragment_duration <- function(task, wait, exec_cycles, choice)
{
    wait_instants <- as.numeric(unlist(strsplit(substring(wait, 2, nchar(wait)-1), ";", fixed = TRUE)))
    create_instants <- as.numeric(tg_data$create[tg_data$parent == task])
    instants <- c(wait_instants, create_instants, 1, 1 + exec_cycles)
    instants <- sort(instants)
    instants <- instants[instants != 0]
    durations <- diff(instants)
    stopifnot(length(durations) > 0)
    fragments <- paste(as.character(task),paste(".",as.character(seq(1:length(durations))),sep=""),sep="")
    if(choice == 1)
      return(fragments)
    else 
      return(durations)
}
tic(type="elapsed")
fd <- data.table(fragment=unlist(mapply(compute_fragment_duration, task=tg_data$task, wait=tg_data$"[wait]", exec_cycles=tg_data$exec_cycles, choice=1)),
                   duration=unlist(mapply(compute_fragment_duration, task=tg_data$task, wait=tg_data$"[wait]", exec_cycles=tg_data$exec_cycles, choice=2))
                   )
toc("Assign execution cycles [step 1]")
tic(type="elapsed")
tg_vertices[match(fd$fragment, tg_vertices$node)]$exec_cycles <- fd$duration
toc("Assign execution cycles [step 2]")
tic(type="elapsed")
tg <- set.vertex.attribute(tg, name="exec_cycles", index=V(tg), value=tg_vertices$exec_cycles)
scale_fn <- function(x) { node_min_size + (x * 100/max(x, na.rm = TRUE)) }
tg <- set.vertex.attribute(tg, name="scaled_exec_cycles", index=V(tg), value=scale_fn(tg_vertices$exec_cycles))
toc("Assign execution cycles [step 3]")

# Write graph as gml file
tg_file_out <- paste(gsub(". $", "", tg_file_in), ".graphml", sep="")
res <- write.graph(tg, file=tg_file_out, format="graphml")
print(paste("Wrote file:", tg_file_out, sep=" "))
