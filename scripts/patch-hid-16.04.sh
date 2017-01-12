#!/bin/bash -e
LINUX_BRANCH=$(uname -r) 

# Get the required tools and headers to build the kernel
sudo apt-get install libusb-1.0-0-dev
sudo apt-get install linux-headers-generic build-essential
sudo apt-get install libssl-dev

# Get the linux kernel and change into source tree
[ ! -d ubuntu-xenial ] && git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git --depth 1
cd ubuntu-xenial
sudo git reset --hard

# Apply UVC formats patch for RealSense devices
patch -p1 < ../scripts/realsense-hid-device_ubuntu16.patch

# Copy configuration
cp /usr/src/linux-headers-$(uname -r)/.config .
cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build so we can build the accel and gyro modules
make scripts oldconfig modules_prepare

# Build accel & gyro module
KBASE=`pwd`
cd drivers/media/usb/uvc
cp $KBASE/Module.symvers .
make -C $KBASE M=$KBASE/drivers/iio/accel modules
make -C $KBASE M=$KBASE/drivers/iio/gyro modules

# Copy to sane location
sudo cp $KBASE/drivers/iio/accel/hid-sensor-accel-3d.ko ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko
sudo cp $KBASE/drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko

# Unload existing module if installed 
echo "Unloading existing accel and gyro modules..."
sudo modprobe -r hid-sensor-accel-3d
sudo modprobe -r hid-sensor-gyro-3d

# Delete existing module
sudo rm /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
sudo rm /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko

# Copy out to module directory
sudo cp ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
sudo cp ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko

# load the new module
sudo modprobe hid-sensor-accel-3d
sudo modprobe hid-sensor-gyro-3d
echo "Script has completed."
