#! /bin/bash

F_OPEN_LOGS=0

# What file to GET
if [ -z "$1" ]
then
  GET_FILE=rock.jpg
else
  GET_FILE=$1
fi

# -l for logging, anything else for no logging
if [ "$2" = "--log" ]
then
  SERVER_LOG_FILE=/tmp/server.log
  CLIENT_LOG_FILE=/tmp/client.log
  F_OPEN_LOGS=1
else
  SERVER_LOG_FILE=/dev/null
  CLIENT_LOG_FILE=/dev/null
fi

# -r for Release, anything else for Debug
if [ "$3" = "--release" ]
then
  BUILD=Release
else
  BUILD=Debug
fi

./out/$BUILD/quic_server --quic_in_memory_cache_dir=tmp-quic-server --port=6121 > $SERVER_LOG_FILE 2>&1 &

time ./out/$BUILD/quic_client --port=6121 http://tmp-quic-server/$GET_FILE > $CLIENT_LOG_FILE 2>&1

killall -q quic_client quic_server

echo "GET_FILE:       " $GET_FILE
echo "SERVER_LOG_FILE:" $SERVER_LOG_FILE
echo "CLIENT_LOG_FILE:" $CLIENT_LOG_FILE
echo "BUILD:          " $BUILD

if [ $F_OPEN_LOGS -eq 1 ]
then
  sleep 1s
  vim /tmp/server.log /tmp/client.log
fi

