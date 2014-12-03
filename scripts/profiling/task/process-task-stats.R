# Libary
#require(dplyr, quietly=TRUE)
suppressMessages(library(dplyr))

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

# Constants
verbo <- T

# Read data
args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
ts.file <- args[1]
if(verbo) print(paste("Reading file", ts.file))
ts.data <- read.csv(ts.file, header=TRUE)

# Find task executed last per worker
if(verbo) tic(type="elapsed")
max.exec.end <- ts.data %>% group_by(core_id) %>% filter(exec_end == max(exec_end))
ts.data["last_to_finish"] <- F
for(task in max.exec.end$task)
{
    ts.data$last_to_finish[ts.data$task == task] <- T
}

# Write out processed data
out.file <- paste(gsub(". $", "", ts.file), ".processed", sep="")
if(verbo) print(paste("Writing file", out.file))
sink(out.file)
print(ts.data)
sink()
if(verbo) toc("Processing")

# Calculate lineage
if(verbo) tic(type="elapsed")
ts.data.sub <- subset(ts.data, select=c("task", "parent"))
ts.data.sub <- ts.data.sub[order(ts.data.sub$task),]
ts.data.sub["lineage"] <- "NA"
for(task in ts.data.sub$task)
{
    parent <- ts.data.sub$parent[ts.data.sub$task == task]
    if(parent != 0)
        ts.data.sub$lineage[ts.data.sub$task == task] <- paste(ts.data.sub$lineage[ts.data.sub$task == parent], as.character(task), sep="-")
    else
        ts.data.sub$lineage[ts.data.sub$task == task] <- paste("0", as.character(task), sep="-")
}
out.file <- paste(gsub(". $", "", ts.file), ".lineage", sep="")
if(verbo) print(paste("Writing file", out.file))
write.csv(ts.data.sub, out.file, row.names=F)
if(verbo) toc("Lineage calculation")

# Summarize
if(verbo) tic(type="elapsed")
out.file <- paste(gsub(". $", "", ts.file), ".info", sep="")
if(verbo) print(paste("Writing file", out.file))
sink(out.file)
cat("num_tasks: ")
cat(max(ts.data$task))
cat("\n")
cat("joins_at_summary: ")
join.freq <- ts.data %>% group_by(parent, joins_at) %>% summarise(count = n())
cat(summary(join.freq$count))
cat("\n")
sink()
if(verbo) toc("Summarizing")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

