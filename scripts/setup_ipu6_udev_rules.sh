#!/bin/bash -e

# USAGE:
# Normal usecase - without any parameters.
#
# [optional parameters]:
# --uninstall : remove permissions for realsense devices.

install=true

for var in "$@"
do
    if [ "$var" = "--uninstall" ]; then
        install=false
    fi
done

if [ "$install" = true ]; then
    echo "Setting-up binding IPU6 and D4XX for RealSense devices"
else
    echo "Remove binding for RealSense devices"
fi

if [ "$install" = true ]; then
    sudo cp scripts/rs-enum.sh /usr/local/bin/rs-enum.sh
    sudo cp scripts/rs_ipu6_d457_bind.sh /usr/local/bin/rs_ipu6_d457_bind.sh
    sudo cp config/99-realsense-d4xx-ipu6.rules /etc/udev/rules.d/
else
    sudo rm /etc/udev/rules.d/99-realsense-d4xx-ipu6.rules
    sudo rm /usr/local/bin/rs_ipu6_d457_bind.sh
    sudo rm /usr/local/bin/rs-enum.sh
fi

sudo udevadm control --reload-rules && sudo udevadm trigger
if [ "$install" = true ]; then
    echo "udev-rules successfully installed"
else
    echo "udev-rules successfully uninstalled"
fi
