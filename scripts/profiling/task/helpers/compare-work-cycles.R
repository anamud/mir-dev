# Clean slate
rm(list=ls())

options("scipen"=999) # big number of digits

# Read data
args <- commandArgs(TRUE)
if(length(args) != 1) 
{
    print("Arguments missing! Please provide table(arg1) for comparing work cycles")
    quit("no", 1)
}
dat <- read.csv(args[1], comment.char='#', na.strings="NA")
if(!("work_cycles.left" %in% colnames(dat)) | !("work_cycles.right" %in% colnames(dat)))
{
    print("Error: Columns work_cycles.{left|right} not found in table. Aborting!")
    quit("no", 1)
}
print(paste("Summarizing work_cycles.left/work_cycles.right in table:", args[1]))
print(summary(dat$work_cycles.left/dat$work_cycles.right))

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)
