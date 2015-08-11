# Clear workspace
rm(list=ls())

# Strings as factors
options(stringsAsFactors = F)

# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Parse args
library(optparse, quietly=TRUE)

option_list <- list(
                    make_option(c("-v", "--verbose"), action="store_true", default=TRUE, help="Print output [default]"),
                    make_option(c("-q", "--quiet"), action="store_false", dest="verbose", help="Print little output"),
                    make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing"),
                    make_option(c("-l","--left"), help = "Task stats to be merged", metavar="FILE"),
                    make_option(c("-r","--right"), help = "Other task stats to be merged", metavar="FILE"),
                    make_option(c("-o","--out"), default="loop-task-stats.merged", help = "Output file name [default \"%default\"]", metavar="STRING"),
                    make_option(c("-k","--key"), help = "Column used for merging"),
                    make_option(c("-c","--common"), default="prompt", help = "How to treat common columns? Choose from: left, right, both, avoid, prompt [default \"%default\"]", metavar="STRING"))

parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

if(!exists("left", where=parsed) | !exists("right", where=parsed) | !exists("key", where=parsed)) {
    my_print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}

# Read data
if(parsed$verbose) my_print("Reading data ...")

dleft <- read.csv(parsed$left, header=TRUE)
dright <- read.csv(parsed$right, header=TRUE)

# Sanity check for key
if(parsed$verbose) my_print("Running sanity checks ...")

if(!(parsed$key %in% colnames(dleft)) | !(parsed$key %in% colnames(dright))) {
    my_print("Error: Key not found in tables. Aborting!")
    quit("no", 1)
}

# Merge while checking for common columns
if(parsed$verbose) my_print("Merging ...")
if(parsed$timing) tic(type="elapsed")

common <- intersect(colnames(dleft)[colnames(dleft) != parsed$key], colnames(dright)[colnames(dright) != parsed$key])
if(length(common) > 0) {
    my_print(paste("Tables contain common columns: ", common))
    if(parsed$common == "prompt") {
        my_print("Merging common columns:  prompt ")
        my_print("Enter choice for merging common columns: both [b], all from left [l], all from right [r], avoid [a] or make selection [s].")
        mc <- scan(file = "stdin", what=character(), n=1, quiet=T)
        if(mc == 'b') {
        } else if(mc == 'l') {
            dright <- subset(dright, select=(setdiff(colnames(dright), common)))
        } else if(mc == 'r') {
            dleft <- subset(dleft, select=(setdiff(colnames(dleft), common)))
        } else if(mc == 'a') {
            dleft <- subset(dleft, select=(setdiff(colnames(dleft), common)))
            dright <- subset(dright, select=(setdiff(colnames(dright), common)))
        } else if(mc == 's') {
            for(c in common) {
                my_print(paste("Enter choice for merging column <", c, ">: both [b], left [l], right [r] or avoid [a]."))
                mcs <- scan(file = "stdin", what=character(), n=1, quiet=T)
                if(mcs == 'b') {
                } else if(mcs == 'l') {
                    dright <- subset(dright, select=(setdiff(colnames(dright), c)))
                } else if(mcs == 'r') {
                    dleft <- subset(dleft, select=(setdiff(colnames(dleft), c)))
                } else if(mcs == 'a') {
                    dright <- subset(dright, select=(setdiff(colnames(dright), c)))
                    dleft <- subset(dleft, select=(setdiff(colnames(dleft), c)))
                } else {
                    my_print("Error: Invalid choice. Aborting!")
                    quit("no", 1)
                }
            }
        } else {
            my_print("Error: Invalid choice. Aborting!")
            quit("no", 1)
        }
    } else if (parsed$common == "both") {
        my_print("Merging common columns: both")
    } else if (parsed$common == "left") {
        my_print("Merging common columns:  all from left")
        dright <- subset(dright, select=(setdiff(colnames(dright), common)))
    } else if (parsed$common == "right") {
        my_print("Merging common columns: all from right")
        dleft <- subset(dleft, select=(setdiff(colnames(dleft), common)))
    } else if (parsed$common == "avoid") {
        my_print("Merging common columns: avoid")
        dleft <- subset(dleft, select=(setdiff(colnames(dleft), common)))
        dright <- subset(dright, select=(setdiff(colnames(dright), common)))
    } else {
        my_print("Error: Invalid --common/-c input. Check help (-h)")
        quit("no", 1)
    }

    dmerge <- merge(dleft, dright, by=parsed$key, all=T, suffixes=c("_left", "_right"))
} else {
    dmerge <- merge(dleft, dright, by=parsed$key, all=T, suffixes=c("_left", "_right"))
}

# Remove background task
dmerge.mod <- dmerge[!is.na(dmerge$parent),]

# Handle NAs
if(parsed$verbose) my_print("Checking for NAs ...")
row.has.na <- apply(dmerge.mod, 1, function(x){any(is.na(x))})
sum.row.has.na <- sum(row.has.na)
if(sum.row.has.na > 0) {
    my_print(sprintf("Warning: %d rows contained NAs in the merged table", sum.row.has.na ))
}

if(parsed$timing) toc("Merge")

# Write out csv
write.csv(dmerge, parsed$out, row.names=FALSE)
my_print(paste("Wrote file:", parsed$out))

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

