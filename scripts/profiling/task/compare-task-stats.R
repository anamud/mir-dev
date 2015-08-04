# Include
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
suppressMessages(library(dplyr))
library(optparse, quietly=TRUE)

# Default comparison
comp.type.default.file <- "scripts/profiling/task/task-stats-comparison-default.cfg"

# Read arguments
option_list <- list(
                    make_option(c("-l","--left"), help = "Task stats left.", metavar="FILE"),
                    make_option(c("-r","--right"), help = "Task stats right.", metavar="FILE"),
                    make_option(c("-k","--key"), default="lineage", help = "Column used for comparison [default \"%default\"]", metavar="STRING"),
                    make_option(c("--config"), default=comp.type.default.file, help = "Comparison configuration [default \"%default\"]", metavar="FILE"),
                    make_option(c("-o","--out"), default="task-stats", help = "Output file suffix [default \"%default\"].", metavar="STRING"),
                    make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                    make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                    make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))

parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

if(!exists("left", where=parsed) | !exists("right", where=parsed)) {
    print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}

# Read data
if(parsed$verbo) print("Reading task stats ...")

ts.data.l <- read.csv(parsed$left, header=TRUE)
ts.data.r <- read.csv(parsed$right, header=TRUE)

if(!(parsed$key %in% colnames(ts.data.l)) | !(parsed$key %in% colnames(ts.data.r))) {
    print("Error: Key not found in task stats. Aborting!")
    quit("no", 1)
}

if(parsed$verbo) print("Reading comparison configuration ...")
if(parsed$config == comp.type.default.file) {
    comp.type <- read.csv(paste(mir_root, comp.type.default.file, sep="/"), header=T)
} else {
    comp.type <- read.csv(parsed$config, header=T)
}

# Set output data
ts.data.out <- subset(ts.data.l, select=parsed$key)

# Compare
if(parsed$timing) tic(type="elapsed")
if(parsed$verbose) print("Comparing task stats ...")
for(r in seq(1,nrow(comp.type))) {
    # Paramters
    attrib <- as.character(comp.type[r,]$comp.attrib)
    op <- as.character(comp.type[r,]$comp.op)
    name <- as.character(comp.type[r,]$comp.name)

    if(attrib %in% colnames(ts.data.l) & attrib %in% colnames(ts.data.l)) {
        if(parsed$verbose) print(paste("Processing comparison type:" , attrib, op, name))

        # Subset and merge left and right task stats
        ts.data.l.sub <- ts.data.l[,c(parsed$key,attrib)]
        ts.data.r.sub <- ts.data.r[,c(parsed$key,attrib)]
        ts.data.comp <- merge(ts.data.l.sub, ts.data.r.sub, by=parsed$key, suffixes=c(".l",".r"))

        # Apply operation
        if(op == "div") {
            ts.data.comp[name] <- subset(ts.data.comp, select=paste(attrib,'.l',sep=""))/subset(ts.data.comp, select=paste(attrib,'.r',sep=""))
        } else if(op == "sub") {
            ts.data.comp[name] <- subset(ts.data.comp, select=paste(attrib,'.l',sep="")) - subset(ts.data.comp, select=paste(attrib,'.r',sep=""))
        } else {
            print(paste("Error: Invalid comparsion operation [", op, "]. Check comparison types.", sep=""))
            quit("no", 1)
        }

        # Merge with output
        ts.data.out <- merge(ts.data.out, ts.data.comp, by=parsed$key, suffixes=c("",""))
    } else {
        if(parsed$verbose) print(paste("Warning: Could not find comparsion attribute [", attrib, "] in task stats. Check comparison types.", sep=""))
    }
}
if(parsed$timing) toc("Comparing")

# Write out processed data
out.file <- paste(gsub(". $", "", parsed$out), ".compared", sep="")
sink(out.file)
write.csv(ts.data.out, out.file, row.names=F)
sink()
if(parsed$verbose) print(paste("Wrote file:", out.file))

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

