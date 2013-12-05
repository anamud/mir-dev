# Clear workspace
rm(list=ls())

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

# Read args
args <- commandArgs(TRUE)
if(length(args) != 2) quit("no", 1)
tg.file <- args[1]
cg.file <- args[2]

tic(type="elapsed")
# Read data
tg.data <- read.csv(tg.file, header=TRUE)
cg.data <- read.csv(cg.file, header=TRUE)
toc("Read data")

tic(type="elapsed")
# Compute cg.task.id to tg.task.id map
# For each task T in cg.data, the map is the task in tg.data whose creation time 
# ... is closest to creation time of T
#get_closest <- function(x) 
#{ 
  #k <- tg.data$execution_start_time-x; 
  #k2 <- ifelse(k>0, NA, k); 
  #res <- ifelse(all(is.na(k2)), NA, which.max(k2)); 
  #res; 
#}
#tg_idx <- sapply(cg.data$creation_time, get_closest)
#tg_idx <- sapply(cg.data$creation_time, function(x)which.max(abs(x - tg.data$execution_start_time)))
# nearest.vec <- function(x, vec)
# {
#   smallCandidate <- findInterval(x, vec, all.inside=T)
#   largeCandidate <- smallCandidate + 1
#   nudge is TRUE if large candidate is nearer, FALSE otherwise
#   nudge <- 2 * x > vec[smallCandidate] + vec[largeCandidate]
#   return(smallCandidate + nudge)
# }
tg.data <- tg.data[order(tg.data$execution_start_time, decreasing=F),]
tg_idx <- findInterval(cg.data$creation_time, tg.data$execution_start_time, all.inside=F)
tg_id <- tg.data$task[tg_idx]
toc("Get closest in time")

tic(type="elapsed")
# Bind everything into a table
cg.data.bind <- cbind(tg_id, cg.data)
toc("Bind task graph id to call graph")

tic(type="elapsed")
# Remove NAs from cg.data.bind
row.has.na <- apply(cg.data.bind, 1, function(x){any(is.na(x))})
print(sprintf("Note: %d rows contained NAs in the bound call graph.", sum(row.has.na)))
cg.data.bind.filtered <- cg.data.bind[!row.has.na,]
# Create extended task graph
ext.tg <- subset(tg.data, select=c(task, parent, join_node_pass_count))
ext.tg <- merge(ext.tg, cg.data.bind.filtered, by.x="task", by.y="tg_id", all=T)
row.has.na <- apply(ext.tg, 1, function(x){any(is.na(x))})
sum.row.has.na <- sum(row.has.na)
print(sprintf("Note: %d rows contained NAs in the extended graph.", sum.row.has.na ))
if(sum.row.has.na > 0)
{
  print("Warning! Converting NAs in the extended task graph to zero. This will probably skew the plot. For example, vertex size based on mean values.")
  ext.tg[is.na(ext.tg)] <- 0
}
toc("Creating extended task graph")

tic(type="elapsed")
# Write out csv
cg.file.out <- paste(gsub(". $", "", cg.file), ".bind.csv", sep="")
print(paste("Writing file", cg.file.out))
write.csv(cg.data.bind, cg.file.out, row.names=FALSE, na="-1")

ext.tg.file <- paste(gsub(". $", "", tg.file), "-ext", sep="")
print(paste("Writing file", ext.tg.file))
write.csv(ext.tg, ext.tg.file, row.names=FALSE)
toc("Write result")

# Warn
warnings()

# Quit
quit("no", 0)
