#!/bin/sh

IFACE_DEFAULT=""

echo "Delaying start for 20 sec."
sleep 20s

echo "Staring the server."
# Start the RealSense server
while (:)
do
  if [ x${IFACE_DEFAULT} != x ]; then
    IFACE=${IFACE_DEFAULT}
  else
    IFACE=`ip link show up | grep BROADCAST | cut -f 2 -d: | head -n 1`
  fi

  IFACE_ADDR=`ip addr show dev ${IFACE} | grep "inet " | cut -d\  -f 6 | cut -d/ -f 1`
  if  [ x${IFACE_ADDR} != x ]; then
    echo "Binding on ${IFACE} with IP address ${IFACE_ADDR}."
    /usr/bin/nice -n -20 /usr/bin/rs-server -i ${IFACE_ADDR} 1> /dev/null 2> /dev/null
  else
    echo "Waiting for the interface."
  fi

  echo "Restarting in 5 sec."
  sleep 1
done

