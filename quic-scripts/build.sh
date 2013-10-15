#! /bin/bash

if [ "$1" = "--update" ]
then
gclient runhooks
fi

ninja -j7 -C out/Release quic_server quic_client

ninja -j7 -C out/Debug   quic_server quic_client
