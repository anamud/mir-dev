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

# Abort on error
abort_on_error <- T

# Read args
args <- commandArgs(TRUE)
if(length(args) != 3) quit("no", 1)
tg.file <- args[1]
cg.file <- args[2]
atg.file.suffix <- args[3]

tic(type="elapsed")
# Read data
tg.data <- read.csv(tg.file, header=TRUE)
cg.data <- read.csv(cg.file, header=TRUE)
toc("Read data time")

tic(type="elapsed")
# Create annotated task graph
annot.tg <- subset(tg.data, select=c(task, parent, joins_at, child_number, num_children, core_id, exec_cycles))
annot.tg <- merge(annot.tg, cg.data, by.x="task", by.y="task", all=T)
row.has.na <- apply(annot.tg, 1, function(x){any(is.na(x))})
sum.row.has.na <- sum(row.has.na)
if(sum.row.has.na > 0)
{
  if(abort_on_error == T)
  {
      print("Aborting on error!")
      quit("no", 1)
  }
  else
  {
      print(sprintf("Note: %d rows contained NAs in the annotated graph.", sum.row.has.na ))
      print("Warning! Converting NAs in the annotated task graph to zero. This will probably skew the plot. For example, vertex size based on mean values.")
      annot.tg[is.na(annot.tg)] <- 0
  }
}
toc("Create annotated task graph time")

tic(type="elapsed")
# Write out csv
annot.tg.file <- paste(atg.file.suffix, "-annotated_task_graph", sep="")
print(paste("Writing file", annot.tg.file))
write.csv(annot.tg, annot.tg.file, row.names=FALSE)
toc("Write file time")

# Warn
warnings()

# Quit
quit("no", 0)
