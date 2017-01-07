# Clear workspace
rm(list=ls())

# Strings as factors
options(stringsAsFactors = F)

# Import
mir_root <- Sys.getenv("MIR_ROOT")
source(paste(mir_root,"/scripts/profiling/task/common.R",sep=""))

# Libary
library(optparse, quietly=TRUE)
suppressMessages(library(dplyr))

# Read arguments
Rstudio_mode <- F
if (Rstudio_mode) {
    parsed <- list(data="loop-task-stats.processed",
                   inflation=10,
                   frequency=800,
                   verbose=T,
                   timing=F)
} else {
    option_list <- list(
                        make_option(c("-d","--data"), help = "Processed task stats.", metavar="FILE"),
                        make_option(c("-i","--inflation"), default=10, help = "Percentage amount of chunk work inflation", metavar="FLOAT"),
                        make_option(c("-f","--frequency"), default=800, help = "Processor frequency in MHz.", metavar="INT"),
                        make_option(c("--verbose"), action="store_true", default=TRUE, help="Print output [default]."),
                        make_option(c("--timing"), action="store_true", default=FALSE, help="Print timing information."),
                        make_option(c("--quiet"), action="store_false", dest="verbose", help="Print little output."))

    parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))

    if (!exists("data", where=parsed)) {
        my_print("Error: Invalid arguments. Check help (-h).")
        quit("no", 1)
    }
}

# Read data
if (parsed$verbose) my_print(paste("Reading file", parsed$data))
d <- read.csv(parsed$data, header=TRUE, comment.char='#', na.strings="NA")

# Filter and group by outline function and idle join
e <- d %>% filter(!is.na(metadata) & grepl(glob2rx("chunk_*_*"), metadata)) %>% group_by(outline_function,idle_join)
e1 <- e %>% summarize(count = n(), chunk_work_cpu_balance = median(chunk_work_cpu_balance))
out_file <- "loop-schedule.info"
write.csv(e1, file=out_file, row.names=F)
my_print(paste("Wrote file:", out_file))

# Print each group to file
for (i in seq(1:nrow(e1)))
{
    of <- e1[i,]$outline_function
    ij <- e1[i,]$idle_join

    f <- e %>% filter(outline_function == of & idle_join == ij)
    # Convert work cycles to time.
    # The conversion is necessary to keep constraint search within integer bounds.
    g <- f %>% rowwise() %>% mutate(work_cycles = (work_cycles + work_cycles*(parsed$inflation/100)))
    g <- f %>% rowwise() %>% mutate(work_time_us = ceiling(work_cycles * (1/parsed$frequency)), chunk_start = as.numeric(unlist(strsplit(metadata, "_"))[2]), chunk_end = as.numeric(unlist(strsplit(metadata, "_"))[3]))

    out_file <- paste(paste("loop", of, ij, sep="_"), "schedule", sep=".")
    write.csv(select(g, chunk_start, chunk_end, cpu_id, work_time_us), file=out_file, row.names=F)
    my_print(paste("Wrote file:", out_file))
}

# Print warnings
wa <- warnings()
if (class(wa) != "NULL")
    print(wa)
