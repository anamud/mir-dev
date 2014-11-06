#!/usr/bin/python

import sys
import re
import operator
#import csv
from collections import defaultdict

scenario = {'worker':-1, 'schedule':'none', 'allocation':'none', 'time':0.0}
scenarios = []

def fast_unique(seq):
    seen = set()
    seen_add = seen.add
    return [ x for x in seq if x not in seen and not seen_add(x)]

def process_entry(entry):
    global scenario
    if(entry.startswith("-w")):
        worker = entry.split('=')[1]
        scenario['worker'] = int(worker)
    elif(entry.startswith("-s")):
        schedule = entry.split('=')[1]
        scenario['schedule'] = schedule
    elif(entry.startswith("-m")):
        allocation = entry.split('=')[1]
        scenario['allocation'] = allocation

def process_conf(line):
    #print(line)
    conf_str=re.findall(r'\"(.+?)\"', line)
    #print(conf_str[0])
    entries = re.split(" +", conf_str[0])
    for entry in entries:
        process_entry(entry)

#def write_csv():
    #keys = ['worker', 'schedule', 'allocation', 'time']
    #f = open(sys.argv[1]+'.csv', 'w')
    #dict_writer = csv.DictWriter(f, keys)
    #dict_writer.writer.writerow(keys)
    #dict_writer.writerows(scenarios)
    #f.close()

def get_times(schedule, allocation, worker):
    times = []
    for i, dict in enumerate(scenarios):
        if dict['schedule'] == schedule:
            if dict['allocation'] == allocation:
                if dict['worker'] == worker:
                    times.append(dict['time'])
    return times

def speedup(plot):
    splot = []
    for index, item in enumerate(plot):
        if(item == 0.0):
            splot.append(float(plot[0]))
        else:
            splot.append(float(plot[0])/float(item))
    return splot

def write_csv():
    temp_dict = defaultdict(set)
    for item in scenarios:
       for key, value in item.items():
           temp_dict[key].add(value)
    #print(temp_dict)
    print('worker,' + ','.join(map(str,sorted(temp_dict['worker']))))
    for schedule in sorted(temp_dict['schedule']):
        for allocation in sorted(temp_dict['allocation']):
            plot = []
            for worker in sorted(temp_dict['worker']):
                times = get_times(schedule, allocation, worker)
                if(len(times) == 0):
                    mtimes = 0.0
                else:
                    mtimes = sum(map(float,times))/float(len(times))
                plot.append(mtimes)
            #print(schedule, allocation, plot)
            plot = speedup(plot)
            print(schedule + '+' + allocation + ',' + ','.join(map(str,plot)))

def process(lines):
    global scenario, scenarios
    for line in lines:
        sline = line.strip('\n')
        sline = sline.strip()
        #print(sline)
        if(sline.startswith("MIR_CONF")):
            process_conf(sline)
        elif(re.match(r'\d+.\d+s', sline)):
            scenario['time'] = re.findall(r'\d+.\d+', sline)[0]
            #print(scenario)
            scenarios.append(scenario.copy())
    scenarios = sorted(scenarios, key=operator.itemgetter('worker'))

def main():
    if(len(sys.argv) != 2):
        print("Usage: %s results-file", sys.argv[0])
    else:
        result = open(sys.argv[1])
        lines = result.readlines()
        result.close()
        process(lines)
        write_csv()

if __name__ == '__main__':
    main()
