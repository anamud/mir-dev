# Clean slate
rm(list=ls())

options("scipen"=999) # big number of digits

# Read data
args <- commandArgs(TRUE)
if(length(args) != 1) 
{
    print("Arguments missing! Please provide table(arg1) for summarizing")
    quit("no", 1)
}
dat <- read.csv(args[1], comment.char='#', na.strings="NA")
print("Table structure")
str(dat)
print("Standard R Summary")
summary(dat)
print("Standard Devation")
apply(dat, 2, sd)
print("Median Absolute Deviation")
apply(dat, 2, mad)

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)
