#! /bin/bash
rm all_stalls.txt
for file in `ls  chromium*.txt`; do
  cat $file | python print_stalls.py >> all_stalls.txt 
done

# parse all_stalls.txt
cat all_stalls.txt | python stall_stats.py
