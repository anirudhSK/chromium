#! /usr/bin/python
import sys
import numpy
fh=sys.stdin

# Two dictionaries to maintain stats across stalls
duration_stats = dict();
position_stats = dict();

for line in fh.readlines():
  records=line.split()
  stall_num      = float(records[0])
  stall_duration = float(records[1])
  stall_position = float(records[2])
  if (stall_num in duration_stats) :
    duration_stats[stall_num] += [stall_duration];
    position_stats[stall_num] += [stall_position];
  else :
    duration_stats[stall_num] = [stall_duration];
    position_stats[stall_num] = [stall_position];

for stall_num in duration_stats:
  print "Duration stats: stall #", stall_num,"mean stall duration: ",numpy.mean(duration_stats[stall_num]),"ms","sd stall duration: ",numpy.std(duration_stats[stall_num]),"ms", len(duration_stats[stall_num]), "samples"

for stall_num in position_stats:
  print "Position stats: stall #", stall_num,"mean stall position: ",numpy.mean(position_stats[stall_num]),"sd stall position: ",numpy.std(position_stats[stall_num]), len(position_stats[stall_num]), "samples"
