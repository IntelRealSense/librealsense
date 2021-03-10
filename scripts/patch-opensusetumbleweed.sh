#!/bin/bash -e

SRC_VERSION_NAME=linux

## from
## http://stackoverflow.com/questions/9293887/in-bash-how-do-i-convert-a-space-delimited-string-into-an-array

FULL_NAME=$( uname -r | tr "-" "\n")
read -a VERSION <<< $FULL_NAME

SRC_VERSION_ID=${VERSION[0]}  ## e.g. : 4.5.6
SRC_VERSION_REL=${VERSION[1]} ## e.g. : 1
LINUX_TYPE=${VERSION[2]}      ## e.g. : ARCH
#remove trailing.0
SRC_VERSION_ID=$(echo $SRC_VERSION_ID | sed -e 's/\.0$//' )

LINUX_BRANCH=opensuse-$SRC_VERSION_ID
KERNEL_NAME=linux-$SRC_VERSION_ID
PATCH_NAME=patch-$SRC_VERSION_ID

# ARCH Linux --  KERNEL_NAME=linux-$SRC_VERSION_ID-$SRC_VERSION_REL-$ARCH.pkg.tar.xz

mkdir kernel
cd kernel

## Get the kernel
wget https://www.kernel.org/pub/linux/kernel/v4.x/$KERNEL_NAME.tar.xz
wget https://www.kernel.org/pub/linux/kernel/v4.x/$PATCH_NAME.xz
#wget https://www.kernel.org/pub/linux/kernel/v4.x/$PATCH_NAME.sign

echo "Extract the kernel"
tar xf $KERNEL_NAME.tar.xz

cd $KERNEL_NAME

## Get the patch

# echo "Patching the kernel..."
### patch  not working ?
# xz -dc ../$PATCH_NAME.xz  | patch -p1


echo "RealSense patch..."

# Apply our RealSense specific patch
patch -p1 < ../../realsense-camera-formats-opensuse.patch

# Prepare to compile modules

## Get the config
# zcat /proc/config.gz > .config  ## Not the good one ?

cp /lib/modules/`uname -r`/build/.config .
cp /lib/modules/`uname -r`/build/Module.symvers .

echo "Prepare the build"

make scripts oldconfig modules_prepare

# Compile UVC modules
echo "Beginning compilation of uvc..."
#make modules
KBASE=`pwd`
cd drivers/media/usb/uvc
cp $KBASE/Module.symvers .
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

# Copy to sane location
#sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko
cd ../../../../../

cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ../uvcvideo.ko

# Unload existing module if installed
echo "Unloading existing uvcvideo driver..."
sudo modprobe -r uvcvideo

cd ..

## Not sure yet about deleting and copying...

# save the existing module

MODULE_NAME=/lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko

if [ -e $MODULE_NAME ]; then
    sudo cp $MODULE_NAME $MODULE_NAME.backup
    sudo rm $MODULE_NAME

    sudo cp uvcvideo.ko $MODULE_NAME
fi

if [ -e $MODULE_NAME.xz ]; then
    sudo cp $MODULE_NAME.xz $MODULE_NAME.xz.backup
    sudo rm $MODULE_NAME.xz

    # compress
    xz uvcvideo.ko
    sudo cp uvcvideo.ko.xz $MODULE_NAME
fi

if [ -e $MODULE_NAME.gz ]; then
    sudo cp $MODULE_NAME.gz $MODULE_NAME.gz.backup
    sudo rm $MODULE_NAME.gz

    # compress
    gzip uvcvideo.ko
    sudo cp uvcvideo.ko.gz $MODULE_NAME.gz
fi

# Copy out to module directory


sudo modprobe uvcvideo

rm -rf kernel

echo "Script has completed. Please consult the installation guide for further instruction."
