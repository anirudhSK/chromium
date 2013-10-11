#! /bin/bash

# gclient runhooks

ninja -j7 -C out/Release quic_server quic_client

ninja -j7 -C out/Debug   quic_server quic_client
