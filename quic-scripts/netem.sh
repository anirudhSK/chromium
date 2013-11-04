#! /bin/bash

DIR="$( cd "$( dirname "$0" )" && pwd )"

# What network condition to change
if [ "$1" = "delay" ] || [ "$1" = loss ]
then
  sudo tc qdisc add dev lo root netem "$@"
elif [ "$1" = "bandwidth" ]
then
  sudo tc qdisc add dev lo root tbf rate $2 buffer 1600 limit 3000
fi

sudo tc -s qdisc ls dev lo

$DIR/transfer.sh page.html --log

sudo tc qdisc del dev lo root
