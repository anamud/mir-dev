# Clear workspace
rm(list=ls())

# Strings as factors
options(stringsAsFactors = F)

# Import
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Default comparison
comp_type_default_file <- "scripts/profiling/task/for-loop/loop-task-stats-comparison-default.cfg"

# Read arguments
option_list <- list(
                    make_option(c("-l","--left"), help = "Task stats to be compared.", metavar="FILE"),
                    make_option(c("-r","--right"), help = "Other task stats to be compared.", metavar="FILE"),
                    make_option(c("-k","--key"), default="lineage", help = "Column used for comparison [default \"%default\"]", metavar="STRING"),
                    make_option(c("--config"), default=comp_type_default_file, help = "Comparison configuration [default \"%default\"]", metavar="FILE"),
                    make_option(c("-o","--out"), default="loop-task-stats", help = "Output file suffix [default \"%default\"].", metavar="STRING"),
                    make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                    make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                    make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))

parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

if(!exists("left", where=parsed) | !exists("right", where=parsed)) {
    my_print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}

# Read data
if(parsed$verbo) my_print("Reading task stats ...")

ts_data_l <- read.csv(parsed$left, header=TRUE)
ts_data_r <- read.csv(parsed$right, header=TRUE)

if(!(parsed$key %in% colnames(ts_data_l)) | !(parsed$key %in% colnames(ts_data_r))) {
    my_print("Error: Key not found in task stats. Aborting!")
    quit("no", 1)
}

# Read config
if(parsed$verbo) my_print("Reading comparison configuration ...")

if(parsed$config == comp_type_default_file) {
    comp_type <- read.csv(paste(mir_root, comp_type_default_file, sep="/"), header=T)
} else {
    comp_type <- read.csv(parsed$config, header=T)
}

# Remove rows where key is NA.
ts_data_l <- ts_data_l[!is.na(subset(ts_data_l, select=parsed$key)),]
ts_data_r <- ts_data_r[!is.na(subset(ts_data_r, select=parsed$key)),]

# Set output data
ts_data_out <- subset(ts_data_l, select=parsed$key)

# Compare
if(parsed$timing) tic(type="elapsed")
if(parsed$verbose) my_print("Comparing task stats ...")

for(r in seq(1,nrow(comp_type))) {
    # Paramters
    attrib <- as.character(comp_type[r,]$attribute)
    op <- as.character(comp_type[r,]$operation)
    name <- as.character(comp_type[r,]$name)

    if(attrib %in% colnames(ts_data_l) & attrib %in% colnames(ts_data_l)) {
        if(parsed$verbose) my_print(paste("Processing comparison type:" , attrib, op, name))

        # Subset and merge left and right task stats
        ts_data_l_sub <- ts_data_l[,c(parsed$key,attrib)]
        ts_data_r_sub <- ts_data_r[,c(parsed$key,attrib)]
        ts_data_comp <- merge(ts_data_l_sub, ts_data_r_sub, by=parsed$key, suffixes=c("_left","_right"))

        # Apply operation
        if(op == "div") {
            ts_data_comp[name] <- subset(ts_data_comp, select=paste(attrib,'_left',sep=""))/subset(ts_data_comp, select=paste(attrib,'_right',sep=""))
        } else if(op == "sub") {
            ts_data_comp[name] <- subset(ts_data_comp, select=paste(attrib,'_left',sep="")) - subset(ts_data_comp, select=paste(attrib,'_right',sep=""))
        } else {
            my_print(paste("Error: Invalid comparsion operation [", op, "]. Check comparison types.", sep=""))
            quit("no", 1)
        }

        # Merge with output
        ts_data_out <- merge(ts_data_out, ts_data_comp, by=parsed$key, suffixes=c("",""))
    } else {
        if(parsed$verbose) my_print(paste("Warning: Could not find comparsion attribute [", attrib, "] in task stats. Check comparison types.", sep=""))
    }
}

if(parsed$timing) toc("Comparing")

# Write out processed data
out_file <- paste(gsub(". $", "", parsed$out), ".compared", sep="")
sink(out_file)
write.csv(ts_data_out, out_file, row.names=F)
sink()
if(parsed$verbose) my_print(paste("Wrote file:", out_file))

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    my_print(wa)

