# Read args
args <- commandArgs(TRUE)
if(length(args) != 2) quit("no", 1)
tg.file <- args[1]
cg.file <- args[2]

# Read data
tg.data <- read.csv(tg.file, header=TRUE)
cg.data <- read.csv(cg.file, header=TRUE)

# Compute cg.task.id to tg.task.id map
# For each task T in cg.data, the map is the task in tg.data whose creation time 
# ... is closest to creation time of T
tg_id <- sapply(cg.data$creation_time, function(x) { k <- tg.data$execution_start_time-x; k2 <- ifelse(k>0, NA, k); res <- ifelse(all(is.na(k2)), NA, which.max(k2)); res; })

# Bind everything into a table
cg.data.bind <- cbind(tg_id, cg.data)

# Write out csv
cg.file.out <- paste(gsub(". $", "", cg.file), ".bind.csv", sep="")
print(paste("Writing file", cg.file.out))
result <- write.csv(cg.data.bind, cg.file.out, row.names=FALSE, na="-1")

# Quit
quit("no", 0)
