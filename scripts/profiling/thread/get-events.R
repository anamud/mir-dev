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

events_min <- dcast(events, cpu~event, min, value.var="value")
events_min <- append_to_colnames(events_min, ".min")

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
cat("Writing event summary:", "event-summary-all.txt", "\n")
sink("event-summary-all.txt")
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
etypes <- unique(events$event)
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
