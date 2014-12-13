# Clear workspace
rm(list=ls())

# Library
require(data.table, quietly=TRUE)

# Read sytem config
d <- read.table(textConnection(system("lscpu -p=cpu,core,socket,node", intern=T, ignore.stderr=T)),sep=",")
colnames(d) <- c("sys_cpu","core","socket","node")

# Remove hyperthreads
ud <- d[!duplicated(data.table(d), by=c("core","socket","node")),]

# Add log cpu column and reorder
ud["log_cpu"] <- seq(0, length(ud$core) - 1)
ud <- ud[c("log_cpu","sys_cpu","core","socket","node")]

# Write out
outfile <- paste(Sys.getenv("MIR_ROOT"), "/src/arch/topology_this", sep="")
print(paste("Writing file:", outfile))
write.table(ud, file=outfile, sep=",", row.names=F)

# Warn
wa <- warnings()
if(class(wa) != "NULL")
    print(wa)
