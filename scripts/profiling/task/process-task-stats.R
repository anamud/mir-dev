# Libary
suppressMessages(library(dplyr))
require(optparse, quietly=TRUE)

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

# Read arguments
option_list <- list(
make_option(c("--lineage"), action="store_true", default=FALSE, help="Calculate task lineage"),
make_option(c("-d","--data"), help = "Task stats file", metavar="FILE"),
make_option(c("-v", "--verbose"), action="store_true", default=TRUE, help="Print output [default]"),
make_option(c("-q", "--quiet"), action="store_false", dest="verbose", help="Print little output"))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("data", where=parsed))
{
    print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}
ts.file <- parsed$data
verbo <- parsed$verbose
calc_lineage <- parsed$lineage

# Read data
if(verbo) print(paste("Reading file", ts.file))
ts.data <- read.csv(ts.file, header=TRUE)

# Find task executed last per worker
if(verbo) tic(type="elapsed")
max.exec.end <- ts.data %>% group_by(cpu_id) %>% filter(exec_end == max(exec_end))
ts.data["last_to_finish"] <- F
for(task in max.exec.end$task)
{
    ts.data$last_to_finish[ts.data$task == task] <- T
}

# Write out processed data
out.file <- paste(gsub(". $", "", ts.file), ".processed", sep="")
if(verbo) print(paste("Writing file", out.file))
sink(out.file)
write.csv(ts.data, out.file, row.names=F)
sink()
if(verbo) toc("Processing")

# Calculate lineage
if(calc_lineage)
{
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
}

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

# Plot useful summaries
out.file <- "task-execution-load-balance.pdf"
if(verbo) print(paste("Writing file", out.file))
pdf(file=out.file)
barplot(as.table(tapply(ts.data$exec_cycles, ts.data$cpu_id, FUN= function(x) {sum(as.numeric(x))} )), log="y", xlab="cpu", ylab="task execution [log-cycles]")
garbage <- dev.off()
out.file <- "task-count-load-balance.pdf"
if(verbo) print(paste("Writing file", out.file))
pdf(file=out.file)
barplot(as.table(tapply(ts.data$task, ts.data$cpu_id, FUN=length)), xlab="cpu", ylab="tasks")
garbage <- dev.off()
if(verbo) toc("Summarizing")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

