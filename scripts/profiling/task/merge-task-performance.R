# Clear workspace
rm(list=ls())

# Timing functions
tic <- function(gcFirst = TRUE, type=c("elapsed", "user.self", "sys.self"))
{
  type <- match.arg(type)
  assign(".type", type, envir=baseenv())
  if(gcFirst) gc(FALSE)
  tic <- proc.time()[type]         
  assign(".tic", tic, envir=baseenv())
  invisible(tic)
}

toc <- function(message)
{
  type <- get(".type", envir=baseenv())
  toc <- proc.time()[type]
  tic <- get(".tic", envir=baseenv())
  print(sprintf("%s: %f sec", message, toc - tic))
  invisible(toc)
}

# Parse args
require(optparse, quietly=TRUE)
option_list <- list(
make_option(c("-v", "--verbose"), action="store_true", default=TRUE, help="Print output [default]"),
make_option(c("-q", "--quiet"), action="store_false", dest="verbose", help="Print little output"),
make_option(c("-l","--left"), help = "Table 1", metavar="FILE"),
make_option(c("-r","--right"), help = "Table 2", metavar="FILE"),
make_option(c("-o","--out"), default="merged-task-perf", help = "Output file name [default \"%default\"]", metavar="STRING"),
make_option(c("-k","--key"), help = "Column used for merging"))
parsed <- parse_args(OptionParser(option_list = option_list), args = commandArgs(TRUE))
if(!exists("left", where=parsed) | !exists("right", where=parsed) | !exists("key", where=parsed))
{
    print("Error: Invalid arguments. Check help (-h)")
    quit("no", 1)
}

# Read data
if(parsed$verbose) tic(type="elapsed")
dleft <- read.csv(parsed$left, header=TRUE)
dright <- read.csv(parsed$right, header=TRUE)
if(parsed$verbose) toc("Read data ")

# Sanity check for key
if(parsed$verbose) tic(type="elapsed")
if(!(parsed$key %in% colnames(dleft)) | !(parsed$key %in% colnames(dright)))
{
    print("Error: Key not found in tables. Aborting!")
    quit("no", 1)
}

# Merge while checking for common columns
common <- intersect(colnames(dleft)[colnames(dleft) != parsed$key], colnames(dright)[colnames(dright) != parsed$key])
if(length(common) > 0)
{
    print("Tables contain common columns: ")
    print(common)
    print("Enter choice for merging common columns: both [b], all left [l], all right [r], avoid [a] or selection [s].")
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
        for(c in common)
        {
            print(paste("Enter choice for merging column <", c, ">: both [b], left [l], right [r] or avoid [a]."))
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
                print("Error: Invalid choice. Aborting!")
                quit("no", 1)
            }
        }
    } else {
        print("Error: Invalid choice. Aborting!")
        quit("no", 1)
    }

    dmerge <- merge(dleft, dright, by=parsed$key, all=T, suffixes=c(".left", ".right"))
} else {
    dmerge <- merge(dleft, dright, by=parsed$key, all=T, suffixes=c(".left", ".right"))
}

# Handle NAs
row.has.na <- apply(dmerge, 1, function(x){any(is.na(x))})
sum.row.has.na <- sum(row.has.na)
if(sum.row.has.na > 0)
{
    print(sprintf("Warning: %d rows contained NAs in the merged table", sum.row.has.na ))
}
if(parsed$verbose) toc("Merge")

# Write out csv
if(parsed$verbose) tic(type="elapsed")
print(paste("Writing file:", parsed$out))
write.csv(dmerge, parsed$out, row.names=FALSE)
if(parsed$verbose) toc("Write output")

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)

