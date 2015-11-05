#!/bin/bash -e

# Obtain Linux Kernel 4.2.3 sources
echo "Cloning Linux source repository... (~1GB, make take a while)"
git clone https://git.launchpad.net/~ubuntu-kernel-test/ubuntu/+source/linux/+git/mainline-crack linux-4.2.3
cd linux-4.2.3
git checkout v4.2.3

# Obtain and apply Ubuntu patches
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.2.3-unstable/0001-base-packaging.patch
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.2.3-unstable/0002-debian-changelog.patch
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.2.3-unstable/0003-configs-based-on-Ubuntu-4.2.0-10.11.patch

patch -p1 < 0001-base-packaging.patch
patch -p1 < 0002-debian-changelog.patch
patch -p1 < 0003-configs-based-on-Ubuntu-4.2.0-10.11.patch

# Apply our RealSense specific patch
patch -p1 < ../realsense-camera-formats.patch

# Prepare to compile modules
cp /boot/config-`uname -r` .config
cp /usr/src/linux-headers-`uname -r`/Module.symvers .
make scripts oldconfig modules_prepare

# Compile UVC modules
KBASE=`pwd`
cd drivers/media/usb/uvc
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

# Install newly compiled UVC video module
sudo rmmod uvcvideo.ko
sudo cp uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
sudo insmod uvcvideo.ko
