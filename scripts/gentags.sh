#!/bin/bash
# Works great with autotag.vim plugin
ctags -R .

# Cscope is deprecated/un-usable
#ackack -n --cc -f > cscope.files
#ctags -L cscope.files
#ctags -e -L cscope.files 
#cscope -ub -i cscope.files

