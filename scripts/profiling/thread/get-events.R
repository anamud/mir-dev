rm(list=ls())

# Strings as factors
options(stringsAsFactors = F)

# Set number of precision digits
options("scipen"=999)

# Libary
library(reshape2)

# Read data
args <- commandArgs(TRUE)
if(length(args) != 1) quit("no", 1)
cat("Reading events file:", args[1], "\n")
events <- read.csv(args[1],sep=':',header=F)
colnames(events) <- c("indicator","cpu","appl","task","thread","time","event","value")
events[,"value"] <- as.numeric(events[,"value"])

# Set first two and last event count of every thread to 0
etypes <- unique(events$event)
threads <- unique(events$cpu)
for(etype in etypes)
{
  for(thread in threads)
  {
    events[events$cpu==thread & events$event==etype,]$value[1] <- 0
    events[events$cpu==thread & events$event==etype,]$value[2] <- 0
    len <- length(events[events$cpu==thread & events$event==etype,]$value)
    events[events$cpu==thread & events$event==etype,]$value[len] <- 0
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
events_sum <- dcast(events, cpu~event, sum, value.var="value")
events_sum <- append_to_colnames(events_sum, ".sum")

## This throws warning "no non-missing arguments to min; returning Inf"
## Warning can be ignored
## See: http://stackoverflow.com/questions/24282550/no-non-missing-arguments-warning-when-using-min-or-max-in-reshape2
events_min <- dcast(events, cpu~event, min, value.var="value")
events_min <- append_to_colnames(events_min, ".min")

## This throws warning "no non-missing arguments to max; returning -Inf"
## Warning can be ignored
## See: http://stackoverflow.com/questions/24282550/no-non-missing-arguments-warning-when-using-min-or-max-in-reshape2
events_max <- dcast(events, cpu~event, max, value.var="value")
events_max <- append_to_colnames(events_max, ".max")

events_median <- dcast(events, cpu~event, median, value.var="value")
events_median <- append_to_colnames(events_median, ".median")

events_mad <- dcast(events, cpu~event, mad, value.var="value")
events_mad <- append_to_colnames(events_mad, ".mad")

events_mean <- dcast(events, cpu~event, mean, value.var="value")
events_mean <- append_to_colnames(events_mean, ".mean")

events_sd <- dcast(events, cpu~event, sd, value.var="value")
events_sd <- append_to_colnames(events_sd, ".sd")

# Combine
events_comb <- merge(events_sum, events_min, by="cpu", all=T)
events_comb <- merge(events_comb, events_max, by="cpu", all=T)
events_comb <- merge(events_comb, events_median, by="cpu", all=T)
events_comb <- merge(events_comb, events_mad, by="cpu", all=T)
events_comb <- merge(events_comb, events_mean, by="cpu", all=T)
events_comb <- merge(events_comb, events_sd, by="cpu", all=T)

# Write out
out_file <- "events-all.csv"
cat("Writing all events to file:", out_file, "\n")
write.csv(events_comb, out_file, row.names=F)

# Write out individual event summary
cust_summary <- function(data, name=F)
{
  if(name==T)
    return(c("min", "lhinge", "median", "uhinge", "max", "mad", "mean", "sd"))

  fiveps <- fivenum(as.numeric(data))
  avg <- mean(as.numeric(data))
  stddev <- sd(as.numeric(data))
  minabsdev <- mad(as.numeric(data))
  return(c(fiveps, minabsdev, avg, stddev))
}
etypes <- unique(events$event)
for(etype in etypes)
{
  events <- events_comb[,grepl(etype, names(events_comb))]
  events_summarized <- as.data.frame(sapply(events, cust_summary))
  events_summarized <- cbind(summary=cust_summary(0,T), events_summarized)

  out_file <- paste("events-",etype,".csv",sep="")
  cat("Writing events by type to file:", out_file, "\n")
  write.csv(events, out_file, row.names=F)
  out_file <- paste("events-",etype,"-summary.csv",sep="")
  cat("Writing summary of events by type to file:", out_file, "\n")
  write.csv(events_summarized, out_file, row.names=F)
}

# Warn
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)

