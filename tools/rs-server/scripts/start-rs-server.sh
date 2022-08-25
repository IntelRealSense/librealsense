#!/bin/sh

# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

IFACE_DEFAULT=""

echo "Delaying start for 5 sec."
sleep 5

echo "Staring the server."
# Start the RealSense server
while (:)
do
  if [ x${IFACE_DEFAULT} != x ]; then
    IFACE=${IFACE_DEFAULT}
  else
    IFACE=`ifconfig | grep BROADCAST | grep RUNNING | cut -f 1 -d: | grep -v usb | head -n 1`
  fi

  if [ x${IFACE} != x ]; then
    IFACE_ADDR=`ifconfig ${IFACE} | grep "inet " | cut -d\  -f10`
    if  [ x${IFACE_ADDR} != x ]; then
      echo "Binding on ${IFACE} with IP address ${IFACE_ADDR}."
      /usr/bin/nice -n -20 /usr/bin/rs-server -i ${IFACE_ADDR} 1> /dev/null 2> /dev/null
    else
      echo "Waiting for the interface ${IFACE}."
    fi
  else
    echo "Waiting for suitable network interface."
  fi

  echo "Restarting in 5 sec."
  sleep 5
done



