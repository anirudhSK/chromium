#! /bin/bash

sudo tc qdisc del dev lo root

killall -q quic_client quic_server
