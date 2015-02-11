suppressMessages(library(dplyr, quietly=TRUE))
library(ggplot2, quietly=TRUE)
library(reshape2, quietly=TRUE)
library(scales, quietly=TRUE)
library(RColorBrewer, quietly=TRUE)
options("scipen"=999) # big number of digits

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

# Useful functions
as.numeric.factor <- function(x) {as.numeric(levels(x))[x]}

# Plotting helpers
bar_plotter <- function(df, log=F, xt, yt, mt=" ", tilt45=F)
{
    colnames(df) <- c("subject", "variable")
    df.melt <- melt(df, id.vars="subject")
    
    if(tilt45 == T)
    {
    barp_t <- theme(axis.text.x = element_text(angle = 45, hjust = 1))
    } else {
    barp_t <- theme()
    }
    barp <- ggplot(df.melt[complete.cases(df.melt),], aes(x=subject,y=value)) + 
      geom_bar(stat="identity", width = 0.8, position = position_dodge(width = 0.8)) 
    if(log == T) {
      barp_l <- labs(x=xt, y=paste(yt,"(log scale)"),title=mt)
      barp <- barp + scale_y_log10() + barp_l + barp_t
    } else {
      barp_l <- labs(x=xt, y=yt,title=mt)
      barp <- barp + scale_y_continuous(labels=scientific) + barp_l + barp_t
    }          
    print(barp)
}

box_plotter <- function(v, xt, yt, mt=" ")
{
  v_comp <- v[complete.cases(v)] 
  v_comp <- v_comp[is.finite(v_comp)]
  v_comp <- v_comp[!is.nan(v_comp)]
  
  if(length(v_comp) > 0) {
    v.melt <- melt(v)
    boxp <- ggplot(v.melt, aes(y=value,x=factor(0))) + 
        geom_boxplot() +
        labs(x=xt,y=yt,title=mt) +
        scale_y_continuous(labels=scientific) +
        coord_flip()
    print(boxp)
  } else { 
      print("Cannot plot. No complete cases.")
  }
}
