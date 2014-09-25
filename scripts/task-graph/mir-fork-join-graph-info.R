#### Libary
require(plyr)

### Read data
args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
tg.file <- args[1]
tg.data <- read.csv(tg.file, header=TRUE)

### Summarize
out.file <- paste(gsub(". $", "", tg.file), ".info", sep="")
print(paste("Writing file", out.file))
sink(out.file)
cat("num_tasks: ")
cat(max(tg.data$task))
cat("\n")
cat("joins_at_summary: ")
cat(summary(count(tg.data, c('parent','joins_at'))$freq))
cat("\n")
sink()

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

