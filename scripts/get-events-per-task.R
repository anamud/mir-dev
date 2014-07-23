# Clean slate
rm(list=ls())

# Libraries
require(reshape2, quietly=T)
require(data.table, quietly=T)

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

tic(type="elapsed")
### Read data
print("Processing accumulated events")
args <- commandArgs(TRUE)
if(length(args) == 0 || length(args) == 2 || length(args) > 3) 
{
  print("Arguments missing! Please provide accumulated events file (arg1). Also provide optional arguments - the task graph (arg2) and the field name (arg3) in the task graph with which event summary should be merged")
  quit("no", 1)
}
dat_file <- args[1]
dat_raw <- read.csv(dat_file, sep=':', na.strings=c(""))

toc("Read data")

tic(type="elapsed")

# Read events and subset
dat_raw <- dat_raw[complete.cases(dat_raw),]
setnames(dat_raw, c("type","number","instant","count","value","meta"))
num_events <- dat_raw[1,]$count 
dat_int <- subset(dat_raw, select=c(value, meta))

# Get events and meta data field names
event_names <- character()
events_1 <- strsplit(as.character(dat_raw[1,]$value), ",")[[1]]
for(event_str in events_1) 
  event_names <- append(event_names, strsplit(event_str, "=")[[1]][1])

# Make proper
# Split columns
dat_proper <- with(dat_int, cbind(colsplit(dat_int$meta, pattern = ",", names=c("id","name")), colsplit(dat_int$value, pattern = "\\,", names=event_names)))
# Remove "EVENT=" and trailing comma from each event
dat_more_proper <- as.data.frame(apply(dat_proper[,event_names], 2, function(x) gsub("[[:alpha:]]+|[[:punct:]]+","",x)))
# Add id and name
dat_most_proper <- data.frame(dat_proper[,c("id")], dat_more_proper)
setnames(dat_most_proper,c("id", event_names))
dat_most_proper <- as.data.frame(apply(dat_most_proper, 2, function(x) as.numeric(as.character(x))))

toc("Tabulating")

tic(type="elapsed")
# Summarize by id
dat_table <- data.table(dat_most_proper)
setkey(dat_table, id)
dat_table_sd <- dat_table[, lapply(.SD, sd, na.rm=TRUE), by=id, .SDcols=event_names] 
setnames(dat_table_sd, c("id", paste(event_names,sep="","_sd")))
dat_table_mean <- dat_table[, lapply(.SD, mean, na.rm=TRUE), by=id, .SDcols=event_names] 
setnames(dat_table_mean, c("id", paste(event_names,sep="","_mean")))
dat_table_sum <- dat_table[, lapply(.SD, sum, na.rm=TRUE), by=id, .SDcols=event_names] 
setnames(dat_table_sum, c("id", paste(event_names,sep="","_sum")))
dat_table_merged <- merge(dat_table_sd, dat_table_mean, by="id")
dat_table_merged <- merge(dat_table_merged, dat_table_sum, by="id")
toc("Summarizing")

tic(type="elapsed")
# Write out
out_file <- paste(sub("^([^.]*).*", "\\1", dat_file), ".table", sep="")
print(paste("Writing tabulated info to file", out_file))
write.table(dat_most_proper, out_file, sep=",", row.names=F)
out_file <- paste(sub("^([^.]*).*", "\\1", dat_file), ".summary", sep="")
print(paste("Writing summary info to file", out_file))
write.table(dat_table_merged, out_file, sep=",", row.names=F)
toc("Writing out")

if(length(args) == 3) 
{
    tic(type="elapsed")
    # Merge with task graph
    tg_file <- args[2]
    tg_field <- args[3]
    tg <- read.csv(tg_file)
    tg_merged <- merge(tg, as.data.frame(dat_table_merged), by.y="id", by.x=tg_field)
    out_file <- paste(sub("^([^.]*).*", "\\1", tg_file), ".with-events", sep="")
    print(paste("Writing event-merged task graph to file", out_file))
    write.table(tg_merged, out_file, sep=",", row.names=F)
    toc("Merging events with task graph")
}

# Warn
w <- warnings()
if(length(w)) w
