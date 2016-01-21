#!/bin/bash -e

# Set working dir
DIR="$( cd "$( dirname "$0" )" && pwd )"

# Dpkg dev
sudo apt-get install dpkg-dev

# Cleanup from possibly failed installations
rm -fr ~/uvc_patch
mkdir -p ~/uvc_patch
cd ~/uvc_patch

# Get the Linux sources and headers for this kernel version
apt-get source linux-image-$(uname -r)
sudo apt-get build-dep linux-image-$(uname -r)
sudo apt-get install linux-headers-$(uname -r)

# Enter linux dir
cd linux-*

KBASE=`pwd`

# Apply our RealSense specific patch
patch -p1 < $DIR/realsense-camera-formats.patch

# Prepare and compile newly patched uvcvideo kernel object
cp /boot/config-`uname -r` .config
cp  /usr/src/linux-headers-`uname -r`/Module.symvers .
make scripts oldconfig modules_prepare
cd drivers/media/usb/uvc
cp $KBASE/Module.symvers .
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

# Install newly compiled UVC video module
sudo modprobe -r uvcvideo
sudo rm /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko

