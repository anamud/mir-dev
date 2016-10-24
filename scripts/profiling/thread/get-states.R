# Strings as factors
options(stringsAsFactors = F)

# Set number of precision digits
options("scipen"=999)

### Read data
args <- commandArgs(TRUE)
if(length(args) != 2) quit("no", 1)
states <- read.csv(args[1], header=T)
tasks <- read.csv(args[2], header=T)

### Custom summary function
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

### Summarize
states_tasks <- merge(states,tasks,by.x=c("THREAD"),by.y=c("worker"))
states_tasks_processed <- cbind(states_tasks,
                                data.frame(
                                "owned_AND_stolen" = as.numeric(states_tasks$owned)+as.numeric(states_tasks$stolen),
                                "TIDLE_owned_AND_stolen" = as.numeric(states_tasks$TIDLE)/(as.numeric(states_tasks$owned)+as.numeric(states_tasks$stolen)),
                                "TCREATE_created" = as.numeric(states_tasks$TCREATE)/as.numeric(states_tasks$created),
                                "TSYNC_created" = as.numeric(states_tasks$TSYNC)/as.numeric(states_tasks$created),
                                "TEXEC_owned_AND_stolen" = as.numeric(states_tasks$TEXEC)/(as.numeric(states_tasks$owned)+as.numeric(states_tasks$stolen))))

#states_tasks_summarized <- cbind(cust_summary(states$TIDLE, "TIDLE"),
                                 #cust_summary(states$TCREATE, "TCREATE"),
                                 #cust_summary(states$TEXEC, "TEXEC"),
                                 #cust_summary(states$TSYNC, "TSYNC"),
                                 #cust_summary(tasks$created, "created"),
                                 #cust_summary(tasks$owned, "owned"),
                                 #cust_summary(tasks$stolen, "stolen"),
                                 #cust_summary(tasks$inlined, "inlined"),
                                 #cust_summary(states_tasks_processed$owned_AND_stolen, "owned_AND_stolen"),
                                 #cust_summary(states_tasks_processed$TCREATE_created, "TCREATE_created"),
                                 #cust_summary(states_tasks_processed$TSYNC_created, "TSYNC_created"),
                                 #cust_summary(states_tasks_processed$TEXEC_owned_AND_stolen, "TEXEC_owned_AND_stolen"))

states_tasks_summarized <- as.data.frame(sapply(states_tasks_processed, cust_summary))
states_tasks_summarized <- cbind(summary=cust_summary(0,T), states_tasks_summarized)

### Print summary to file
#out_file <- paste(gsub(". $", "", args[1]), ".info", sep="")
out_file <- "states.csv"
cat("Writing states to file:", out_file, "\n")
write.csv(states_tasks_processed, out_file, row.names=F)
out_file <- "states_summary.csv"
cat("Writing summary of states to file:", out_file, "\n")
write.csv(states_tasks_summarized, out_file, row.names=F)

# Warn
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)

