#### Libary

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
    s.d <- sd(as.numeric(data))
    m.a.d <- mad(as.numeric(data))
    return(c(fiveps, m.a.d, avg, s.d))
}

### Summarize
states.tasks <- merge(states,tasks,by.x=c("THREAD"),by.y=c("worker"))
states.tasks.processed <- cbind(states.tasks, 
                                data.frame(
                                "owned_And_stolen" = as.numeric(states.tasks$owned)+as.numeric(states.tasks$stolen),
                                "TIDLE_Per_owned_And_stolen" = as.numeric(states.tasks$TIDLE)/(as.numeric(states.tasks$owned)+as.numeric(states.tasks$stolen)),
                                "TCREATE_Per_created" = as.numeric(states.tasks$TCREATE)/as.numeric(states.tasks$created),
                                "TSYNC_Per_created" = as.numeric(states.tasks$TSYNC)/as.numeric(states.tasks$created),
                                "TEXEC_Per_owned_And_stolen" = as.numeric(states.tasks$TEXEC)/(as.numeric(states.tasks$owned)+as.numeric(states.tasks$stolen))))

#sum.df <- cbind(cust_summary(states$TIDLE, "TIDLE"),
            #cust_summary(states$TCREATE, "TCREATE"),
            #cust_summary(states$TEXEC, "TEXEC"),
            #cust_summary(states$TSYNC, "TSYNC"),
            #cust_summary(tasks$created, "created"),
            #cust_summary(tasks$owned, "owned"),
            #cust_summary(tasks$stolen, "stolen"),
            #cust_summary(tasks$inlined, "inlined"),
            #cust_summary(states.tasks.processed$owned_And_stolen, "owned_And_stolen"),
            #cust_summary(states.tasks.processed$TCREATE_Per_created, "TCREATE_Per_created"),
            #cust_summary(states.tasks.processed$TSYNC_Per_created, "TSYNC_Per_created"),
            #cust_summary(states.tasks.processed$TEXEC_Per_owned_And_stolen, "TEXEC_Per_owned_And_stolen"))

sum.df <- as.data.frame(sapply(states.tasks.processed, cust_summary))
rownames(sum.df) <- cust_summary(0,T)

### Print summary to file
#out.file <- paste(gsub(". $", "", args[1]), ".info", sep="")
out.file <- "states.info"
cat("Writing state summary in file:", out.file, "\n")
sink(out.file)
print(states.tasks.processed)
print(sum.df)
sink()
