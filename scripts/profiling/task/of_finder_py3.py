#!/usr/bin/python

# NOTE: THIS SCRIPT REQUIRES PYTHON VERSION > 3

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

# Functions dynamically callable from outline functions
dynamically_callable_funcs = []

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
    command = 'objdump --syms  --section=.text {} | grep " F " | grep -E "{}"'.format(obj_fil,outline_func_pattern)
    child = subprocess.Popen([command], shell=True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    if(child.returncode == 0):
        return process(child.communicate()[0])
    else:
        return ""

def get_callable(obj_fil):
    # We will use objdump to get a list of function (F) symbols in the .text segment
    command = 'objdump --syms  --section=.text {} | grep " F " | grep -E -v "{}"'.format(obj_fil,outline_func_pattern)
    child = subprocess.Popen([command], shell=True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    if(child.returncode == 0):
        return process(child.communicate()[0])
    else:
        return ""

def get_dynamically_callable(obj_fil):
    # We will use objdump to get a list of dynamic function (DF) symbols
    command = 'objdump -T {} | grep " DF " | grep -E -v "{}"'.format(obj_fil,outline_func_pattern)
    child = subprocess.Popen([command], shell=True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    if(child.returncode == 0):
        return process(child.communicate()[0])
    else:
        return ""

def main():
    global outline_funcs, callable_funcs, dynamically_callable_funcs
    if sys.version_info < (3,0):
        print("Python version < 3. Aborting!")
        sys.exit(1)
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hve", ["help", "verbose", "export"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err) # will print something like "option -a not recognized"
        #usage()
        print('Usage: {} -hve objfiles'.format(sys.argv[0]))
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
            print('Usage: {} -hve objfiles'.format(sys.argv[0]))
            sys.exit()
        #elif o in ("-o", "--output"):
            #output = a
        else:
            assert False, "unhandled option"
    if verbose and export != True:
        print('Using "{}" as outline function name pattern'.format(outline_func_pattern))
    num_obj = len(args)
    # For each object file, extract names of outline and callable functions
    for i in range(0, num_obj):
        obj_fil = args[i]
        if verbose and export != True:
            print('Processing file: {}'.format(obj_fil))
        outline_funcs.append(get_outlined(obj_fil))
        callable_funcs.append(get_callable(obj_fil))
        dynamically_callable_funcs.append(get_dynamically_callable(obj_fil))
    # Remove multiple commas and spaces in outline_funcs list
    outline_funcs = ",".join(outline_funcs)
    outline_funcs = re.sub(' +', '', outline_funcs)
    outline_funcs = re.sub(',+', ',', outline_funcs)
    # Remove multiple commas and spaces in callable_funcs list
    callable_funcs = ",".join(callable_funcs)
    callable_funcs = re.sub(' +', '', callable_funcs)
    callable_funcs = re.sub(',+', ',', callable_funcs)
    # Remove multiple commas and spaces in callable_funcs list
    dynamically_callable_funcs = ",".join(dynamically_callable_funcs)
    dynamically_callable_funcs = re.sub(' +', '', dynamically_callable_funcs)
    dynamically_callable_funcs = re.sub(',+', ',', dynamically_callable_funcs)
    # Print out
    if export:
        print('export CHECKME_OUTLINE_FUNCTIONS=',end='')
    else:
        print('CHECKME_OUTLINE_FUNCTIONS=',end='')
    print(outline_funcs.strip().strip(','))
    if export:
        print('export CHECKME_CALLED_FUNCTIONS=',end='')
    else:
        print('CHECKME_CALLED_FUNCTIONS=',end='')
    print(callable_funcs.strip().strip(','))
    if export:
        print('export CHECKME_DYNAMICALLY_CALLED_FUNCTIONS=',end='')
    else:
        print('CHECKME_DYNAMICALLY_CALLED_FUNCTIONS=',end='')
    print(dynamically_callable_funcs.strip().strip(','))

if __name__ == '__main__':
    main()
