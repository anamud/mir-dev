#!/usr/bin/python2

# NOTE: THIS SCRIPT REQUIRES PYTHON VERSION 2

import sys
import os
import subprocess

# Seperate pattern with a "|"
outline_func_pattern = '._omp_fn.|omp_fn.|^ol_'

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
    if(len(sys.argv) == 1):
        print 'Usage: {0} objfiles'.format(sys.argv[0])
        sys.exit(1)
    print 'Using "{0}" as outline function name pattern'.format(outline_func_pattern)
    num_obj = len(sys.argv) - 1
    # For each object file, extract names of outline and callable functions
    for i in range(0, num_obj):
        obj_fil = sys.argv[i+1]
        print 'Processing file: {0}'.format(obj_fil)
        outline_funcs.append(get_outlined(obj_fil))
        callable_funcs.append(get_callable(obj_fil))
    print('CHECKME_OUTLINE_FUNCTIONS=',end=''),
    print ", ".join(outline_funcs).strip().replace(',,',',').replace(' ','').strip(',')
    print('CHECKME_CALLED_FUNCTIONS=',end=''),
    print ", ".join(callable_funcs).strip().replace(',,',',').replace(' ','').strip(',')


if __name__ == '__main__':
    main()
