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
# ts = task stats produced by runtime system (MIR_CONF: -g)
# ip = instruction profile produced by the outline function profiler
ts.file <- args[1]
ip.file <- args[2]
comb.tp.file.prefix <- args[3]

tic(type="elapsed")
# Read data
ts.data <- read.csv(ts.file, header=TRUE)
ip.data <- read.csv(ip.file, header=TRUE)
toc("Read data time")

tic(type="elapsed")
# Create combined task performance table
comb.tp <- subset(ts.data, select=c(task, parent, joins_at, child_number, num_children, core_id, exec_cycles))
comb.tp <- merge(comb.tp, ip.data, by.x="task", by.y="task", all=T)
row.has.na <- apply(comb.tp, 1, function(x){any(is.na(x))})
sum.row.has.na <- sum(row.has.na)
if(sum.row.has.na > 0)
{
  print(sprintf("Note: %d rows contained NAs in the combined task performance table", sum.row.has.na ))
  if(abort_on_error == T)
  {
    print("Aborting on error!")
    quit("no", 1)
  }
  else
  {
      print("Warning! Converting NAs in the combined task performance table to zero. This will probably skew processing. For example, scaling vertex size based on mean values while plotting graphs.")
      comb.tp[is.na(comb.tp)] <- 0
  }
}
toc("Create combined task performance table time")

tic(type="elapsed")
# Write out csv
comb.tp.file <- paste(comb.tp.file.prefix, "", sep="")
print(paste("Writing file", comb.tp.file))
write.csv(comb.tp, comb.tp.file, row.names=FALSE)
toc("Write file time")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

