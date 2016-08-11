#!/bin/bash -e

LINUX_BRANCH=$1

# Obtain and apply Ubuntu patches
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/SOURCES

THE_BRANCH=`echo $a | awk 'NF>1{print $(NF-1)}' SOURCES`

# Obtain Linux Kernel sources
echo "Shallow cloning Linux source repository... (~100mb)"
git clone --verbose git://git.launchpad.net/~ubuntu-kernel-test/ubuntu/+source/linux/+git/mainline-crack linux-$LINUX_BRANCH --branch $THE_BRANCH --depth 1
cd linux-$LINUX_BRANCH

# Produce index.html
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/

# Get the debian package
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/`grep 'linux-headers-[^"]*_all.deb' index.html -o | sed -n '1P'`
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/`grep 'linux-headers-[^"]*-generic[^"]*_amd64.deb' index.html -o | sed -n '1P'`

# Install the package
sudo dpkg -i linux-headers-*.deb

RAW_TAG=`echo $THE_BRANCH | cut -c 2-`
CONFIG_LOCATION=/usr/src/linux-headers-$RAW_TAG*-generic/

# Now can get symvers from /usr/src/....

PATCH_A=`echo $a | sed -n '2p' < ../SOURCES`
PATCH_B=`echo $a | sed -n '3p' < ../SOURCES`
PATCH_C=`echo $a | sed -n '4p' < ../SOURCES`

echo "The Branch Is: " $THE_BRANCH

git checkout $THE_BRANCH

wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/$PATCH_A
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/$PATCH_B
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$LINUX_BRANCH/$PATCH_C

patch -p1 < $PATCH_A
patch -p1 < $PATCH_B
patch -p1 < $PATCH_C

# Apply our RealSense-specific patch
patch -p1 < ../scripts/realsense-camera-formats.patch

# Prepare to compile modules
cp $CONFIG_LOCATION/.config .
cp $CONFIG_LOCATION/Module.symvers .

make scripts oldconfig modules_prepare

# Compile UVC modules
echo "Beginning compilation of uvc..."
#make modules
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

echo "Script has completed. Please consult the installation guide for further instruction."
