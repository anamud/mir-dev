rm(list=ls())
library(reshape2)

# FIXME! Use proper names, rather than Vx!
# Yes I know, that we rely on the fixed nature of the paraver file format, but it is wise to avoid sequential numbers in column names. 

# Read data 
args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
cat("Reading events file:", args[1], "\n")
events <- read.csv(args[1],sep=':',header=F)
events[,"V8"] <- as.numeric(events[,"V8"])

# Set first two and last event count of every thread to 0
etypes <- unique(events$V7)
threads <- unique(events$V2)
for(etype in etypes)
{
  for(thread in threads)
  {
    events[events$V2==thread & events$V7==etype,]$V8[1] <- 0
    events[events$V2==thread & events$V7==etype,]$V8[2] <- 0
    len <- length(events[events$V2==thread & events$V7==etype,]$V8)
    events[events$V2==thread & events$V7==etype,]$V8[len] <- 0
  }
}

# Summarize
append_to_colnames <- function(df, name)
{
  n <- colnames(df)
  nn <- paste(n, name, sep="")
  nn[1] <- n[1]
  colnames(df) <- nn
  return(df)
}
events_sum <- dcast(events, V2~V7, sum, value.var="V8")
events_sum <- append_to_colnames(events_sum, ".sum")

events_min <- dcast(events, V2~V7, min, value.var="V8")
events_min <- append_to_colnames(events_min, ".min")

events_max <- dcast(events, V2~V7, max, value.var="V8")
events_max <- append_to_colnames(events_max, ".max")

events_median <- dcast(events, V2~V7, median, value.var="V8")
events_median <- append_to_colnames(events_median, ".median")

events_mad <- dcast(events, V2~V7, mad, value.var="V8")
events_mad <- append_to_colnames(events_mad, ".mad")

events_mean <- dcast(events, V2~V7, mean, value.var="V8")
events_mean <- append_to_colnames(events_mean, ".mean")

events_sd <- dcast(events, V2~V7, sd, value.var="V8")
events_sd <- append_to_colnames(events_sd, ".sd")

# Combine
events_comb <- merge(events_sum, events_min, by="V2", all=T)
events_comb <- merge(events_comb, events_max, by="V2", all=T)
events_comb <- merge(events_comb, events_median, by="V2", all=T)
events_comb <- merge(events_comb, events_mad, by="V2", all=T)
events_comb <- merge(events_comb, events_mean, by="V2", all=T)
events_comb <- merge(events_comb, events_sd, by="V2", all=T)

# Write out
cat("Writing event summary:", "event-summary-all.txt", "\n")
sink("event-summary-al.txt")
print(events_comb)
sink()

# Write out individual event summary
cust_summary <- function(data, name=F)
{
  if(name==T)
    return(c("min", "lhinge", "median", "uhinge", "max", "mad", "mean", "sd"))
  
  fiveps <- fivenum(as.numeric(data))
  avg <- mean(as.numeric(data))
  s.d <- sd(as.numeric(data))
  m.a.d <- mad(as.numeric(data))
  return(c(fiveps, m.a.d, avg, s.d))
}
etypes <- unique(events$V7)
for(etype in etypes)
{
  df <- events_comb[,grepl(etype, names(events_comb))]
  sum.df <- as.data.frame(sapply(df, cust_summary))
  rownames(sum.df) <- cust_summary(0,T)

  fi <- paste("event-summary-",etype,".txt",sep="")
  cat("Summarizing event", etype, "in file:", fi, "\n")
  sink(fi)
  print(df)
  print(sum.df)
  sink()
}
