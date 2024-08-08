#!/bin/bash -e

echo 8388608 > /proc/sys/net/core/wmem_default
echo 8388608 > /proc/sys/net/core/rmem_default

echo "Setting-up network queues successfully changed"
