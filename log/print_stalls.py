#! /usr/bin/python
import sys
fh=sys.stdin
stall_num=0
for line in fh.readlines():
  records=line.split(' ');
  if (records[0] != "#Stall"):
    continue
  else :
    stall_duration = records[2];
    stall_position = records[7];
    stall_num = stall_num+1;
    print stall_num, stall_duration, stall_position
