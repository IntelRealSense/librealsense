#!/bin/bash -e

# USAGE:
# Normal usecase - without any parameters.
#
# [optional parameters]:
# --uninstall : remove permissions for realsense devices.
# --auto_power_off : add script for automatic power off.

install=true
auto_power_off=false

for var in "$@"
do
    if [ "$var" = "--uninstall" ]; then
        install=false
    fi

    if [ "$var" = "--auto_power_off" ]; then
        auto_power_off=true
    fi
done

if [ "$install" = true ]; then
    echo "Setting-up permissions for RealSense devices"
    if [ "$auto_power_off" = true ]; then
        echo "Setting-up RealSense Device auto power off."
        sudo apt install -q=3 at || (echo "Failed to install package 'at'. Remove flag --auto_power_off and run again." && exit 1)
    fi
else
    echo "Remove permissions for RealSense devices"
fi

exec 3>&2
exec 2> /dev/null
con_dev=$(ls /dev/video* | wc -l)
exec 2>&3

if [ "$install" = true ]; then
    if [ $con_dev -ne 0 ];
    then
        echo -e "\e[32m"
        read -p "Remove all RealSense cameras attached. Hit any key when ready"
        echo -e "\e[0m"
    fi
    sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/
    if [ "$auto_power_off" = true ]; then
        echo | sudo tee -a /etc/udev/rules.d/99-realsense-libusb.rules > /dev/null
        echo "KERNEL==\"iio*\", ATTRS{idVendor}==\"8086\", ATTRS{idProduct}==\"0ad5|0afe|0aff|0b00|0b01|0b3a|0b3d\", RUN+=\"/bin/sh -c 'enfile=/sys/%p/buffer/enable && echo \\\"COUNTER=0; while [ \\\$COUNTER -lt 500 ] && grep -q 0 \$enfile; do sleep 0.01; COUNTER=\\\$((COUNTER+1)); done && echo 0 > \$enfile\\\" | at now'\"" | sudo tee -a /etc/udev/rules.d/99-realsense-libusb.rules > /dev/null
    fi
else
    sudo rm /etc/udev/rules.d/99-realsense-libusb.rules
fi

sudo udevadm control --reload-rules && udevadm trigger
if [ "$install" = true ]; then
    echo "udev-rules successfully installed"
else
    echo "udev-rules successfully uninstalled"
fi
