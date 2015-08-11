#!/usr/bin/python2

# NOTE: THIS SCRIPT REQUIRES PYTHON VERSION 2

import sys
import os
import subprocess
import getopt
import re
import string

# Seperate pattern with a "|"
outline_func_pattern = '._omp_fn.|omp_fn.| ol_'

# Outline functions
outline_funcs = []

# Functions callable from outline functions
callable_funcs = []

def process(raw_out):
    # We will extract function names from objdump output
    str_out = raw_out.decode('utf-8')
    str_out_lines = str_out.split('\n')
    funcs = []
    for line in str_out_lines:
        if(len(line)>0):
            funcs.append(line.rpartition(' ')[2])
    return ",".join(funcs)

def get_outlined(obj_fil):
    # We will use objdump to get a list of function (F) symbols in the .text segment
    # We will then filter that list for known outline function patterns
    command = 'objdump --syms  --section=.text {0} | grep " F " | grep -E "{1}"'.format(obj_fil,outline_func_pattern)
    raw_out = subprocess.Popen([command], shell=True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT).communicate()[0]
    return process(raw_out)

def get_callable(obj_fil):
    # We will use objdump to get a list of function (F) symbols in the .text segment
    command = 'objdump --syms  --section=.text {0} | grep " F " | grep -E -v "{1}"'.format(obj_fil,outline_func_pattern)
    raw_out = subprocess.Popen([command], shell=True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT).communicate()[0]
    return process(raw_out)

def main():
    global outline_funcs, callable_funcs
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hve", ["help", "verbose", "export"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err) # will print something like "option -a not recognized"
        #usage()
        print 'Usage: {0} -hve objfiles'.format(sys.argv[0])
        sys.exit(2)
    verbose = False
    export = False
    for o, a in opts:
        if o == "-v":
            verbose = True
        elif o == "-e":
            export = True
        elif o in ("-h", "--help"):
            #usage()
            print 'Usage: {0} -hve objfiles'.format(sys.argv[0])
            sys.exit()
        #elif o in ("-o", "--output"):
            #output = a
        else:
            assert False, "unhandled option"
    if verbose and export != True:
        print 'Using "{0}" as outline function name pattern'.format(outline_func_pattern)
    num_obj = len(args)
    # For each object file, extract names of outline and callable functions
    for i in range(0, num_obj):
        obj_fil = args[i]
        if verbose and export != True:
            print 'Processing file: {0}'.format(obj_fil)
        outline_funcs.append(get_outlined(obj_fil))
        callable_funcs.append(get_callable(obj_fil))
    # Remove multiple commas and spaces in outline_funcs list
    outline_funcs = string.join(outline_funcs, ",")
    outline_funcs = re.sub(' +', '', outline_funcs)
    outline_funcs = re.sub(',+', ',', outline_funcs)
    # Remove multiple commas and spaces in callable_funcs list
    callable_funcs = string.join(callable_funcs, ",")
    callable_funcs = re.sub(' +', '', callable_funcs)
    callable_funcs = re.sub(',+', ',', callable_funcs)
    # Print out
    if export:
        sys.stdout.write('export CHECKME_OUTLINE_FUNCTIONS=')
    else:
        sys.stdout.write('CHECKME_OUTLINE_FUNCTIONS=')
    print outline_funcs.strip().strip(',')
    if export:
        sys.stdout.write('export CHECKME_CALLED_FUNCTIONS=')
    else:
        sys.stdout.write('CHECKME_CALLED_FUNCTIONS=')
    print callable_funcs.strip().strip(',')

if __name__ == '__main__':
    main()
