#! /bin/bash

DIR="$( cd "$( dirname "$0" )" && pwd )"

# What network condition to change
sudo tc qdisc add dev lo root netem "$@"

sudo tc -s qdisc ls dev lo

$DIR/transfer.sh page.html --no-log --debug

sudo tc qdisc del dev lo root
