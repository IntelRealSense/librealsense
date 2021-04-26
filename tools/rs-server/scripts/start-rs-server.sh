#!/bin/sh

sleep 20s

# Start the RealSense server
while (:)
do
  ADDR=`ifconfig eth0 | grep inet\  | cut -d\  -f 10-11`
  if [ x$ADDR != x ]; then
    /usr/bin/nice -n -20 /usr/bin/rs-server -i $ADDR 1> /dev/null 2> /dev/null
  fi
  sleep 1
done
