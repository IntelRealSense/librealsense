#!/bin/bash -e
LINUX_BRANCH=$(uname -r) 

# Get the required tools and headers to build the kernel
sudo apt-get build-dep linux
sudo apt-get install linux-headers-generic build-essential 

# Get the linux kernel and change into source tree
apt-get source linux
cd linux-*

# Apply the one, simple UVC format patch
patch -p1 < ../scripts/realsense-camera-formats.patch

# Copy configuration
cp /usr/src/linux-headers-$(uname -r)/.config .
cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build so we can build just the uvcvideo module
make scripts oldconfig modules_prepare

# Build the uvc modules
KBASE=`pwd`
cd drivers/media/usb/uvc
cp $KBASE/Module.symvers .
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

# Copy to sane location
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko

# Unload existing module if installed 
echo "Unloading existing uvcvideo driver..."
sudo modprobe -r uvcvideo

# Delete existing module
sudo rm /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko

# Copy out to module directory
sudo cp ~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko

# load the new module
sudo modprobe uvcvideo
echo "Script has completed. Please consult the installation guide for further instruction."
