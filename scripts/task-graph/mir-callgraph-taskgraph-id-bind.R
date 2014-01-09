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
# Create extended task graph
ext.tg <- subset(tg.data, select=c(task, parent, join_node_pass_count))
ext.tg <- merge(ext.tg, cg.data, by.x="task", by.y="task", all=T)
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
ext.tg.file <- paste(gsub(". $", "", tg.file), "-ext", sep="")
print(paste("Writing file", ext.tg.file))
write.csv(ext.tg, ext.tg.file, row.names=FALSE)
toc("Write result")

# Warn
warnings()

# Quit
quit("no", 0)
