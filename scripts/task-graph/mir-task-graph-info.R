#### Libary
require(plyr)

### Read data
args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
tg.file <- args[1]
tg.data <- read.csv(tg.file, header=TRUE)

### Summarize
sink(paste(gsub(". $", "", tg.file), ".info", sep=""))
cat("num_tasks: ")
cat(max(tg.data$task))
cat("\n")
cat("join_degree_summary: ")
cat(summary(count(tg.data, c('join_node','join_node_pass_count'))$freq))
cat("\n")
sink()
