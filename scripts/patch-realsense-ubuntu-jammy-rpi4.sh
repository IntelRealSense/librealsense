#!/bin/bash

#Break execution on any error received
set -e

# Note: In contrast to many of the other patch-* variants in this scripts folder
# this only targets a single Ubuntu release and a single platform
#    Ubuntu 22.04 jammy
#    RaspberryPi4
# Arguably, I think this script is much cleaner than trying to support a wide
# range of Ubuntu releases and a wide range of board. I think this is clear
# enough that it would be a good launching point for trying to adapt to your
# distribution and your board.

# Some background on the patched kernel modules. I had to go hunting for this information in a bunch
# of forums and github Issues (and hope I am coallescing the info correctly).
# There are two ways for Linux to communicate with the Intel Realsense cameras. 
#  1. libusb/RS_USB: this is selected at librealsense build time with the RS_USB option
#  2. v4l (Video4Linux): this is the method that requires patched kernel modules and communicates with
#     the camera using the v4l standard of /dev/video* 
# The libusb/RSUSB method has some limitations in that even though you can set every(?) parameter of
# the camera, you can't read all of them. One particular shortcoming is that you can't read the current
# exposure when the camera is in auto-exposure mode.
# The v4l interface exposes all the functionality of the camera. The forums and Issues seem to indicate
# that v4l is also the more maintained method of interfacing with the camera and have better performacen, 
# despite it being harder to get up and running on non-standard platforms.

# Install all dependencies necessary to build kernel modules
sudo apt install bison flex libssl-dev make libncurses-dev
sudo apt-get build-dep linux-image-raspi
apt-get source linux-image-$(uname -r)  
cd linux-raspi-5.15.0/


# For x86_64
#sudo apt-get build-dep linux-image-unsigned-$(uname -r)
#apt-get source linux-image-unsigned-$(uname -r)
#export vershort=`echo $(uname -r) | cut -f1 -d"-"`
#cd linux-$vershort

# Note: the commands above should download the sources for current running kernel into the current folder
#       Unfortunately, the folder name often doesn't have rhyme or reason ascertainable from 'uname -r'

# I combined all the split patches into a single patch. This is an adaptation/combination of
#   realsense-camera-formats-*.patch
#   realsense-metadata-*.patch
#   realsense-powerlinefrequency-control0fix.patch
#   uvcvideo_increase_UVC_URBS.patch
# I found that the following patch from previous Ubuntu-related patches was already in the 5.15 kernel
#   realsense-hid-*.patch
export LINUX_BRANCH=$(uname -r)
patch -p1 <  ../scripts/realsense-combined-jammy-rpi4-5.15.0_1006.patch
chmod +x scripts/*.sh   # This one seemed bizarre, but you have to do it, or some scripts won't run

# This step copies the configuration of the current running module and the symbol versions of the currently
# running kernel into this source tree so that after building the modules will have matched symbol versions
# and will actually load correctly
sudo cp /usr/src/linux-headers-$(uname -r)/.config .
sudo cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# This step runs the preparation steps of the automake build system, using the old configs as a staring point
sudo make olddefconfig modules_prepare

# Once again, you have to make sure the kernel version from the installed kernel matches the kernel version
# in the rebuilt modules, otherwise they will be warnings and/or errors. Remember, we are only rebuilding a
# few kernel modules, not the entire kernel.
sudo sed -i "s/\".*\"/\"$LINUX_BRANCH\"/g" ./include/generated/utsrelease.h
sudo sed -i "s/.*/$LINUX_BRANCH/g" ./include/config/kernel.release

# Build all the patched kernel modules and copy them to the home folder for later installation
# I am not enough of a kernel module expert to know what the focal kernels on x86 didn't need the videobuf2
# kernel modules to load successfully, but after a lot of debugging found that jammy on rpi4 does need them
# Remaining question: I don't know that we need to rebuilt the hid-senser-* modules because, as I mentioned
# above, these seemed to already be patched in the 5.15 kernel that is the default for jammy. I build and 
# install them anyway.
# TODO: I really don't like copying the kernels to the home folder, but that is how the existing scripts in
#       librealsense github do it, so I followed suit.
export KBASE=`pwd`
cd drivers/media/usb/uvc
sudo cp $KBASE/Module.symvers .
sudo make -j -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules
sudo make -j -C $KBASE M=$KBASE/drivers/iio/accel modules
sudo make -j -C $KBASE M=$KBASE/drivers/iio/gyro modules
sudo make -j -C $KBASE M=$KBASE/drivers/media/v4l2-core modules
sudo make -j -C $KBASE M=$KBASE/drivers/media/common/videobuf2 modules
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko
sudo cp $KBASE/drivers/iio/accel/hid-sensor-accel-3d.ko ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko
sudo cp $KBASE/drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko
sudo cp $KBASE/drivers/media/v4l2-core/videodev.ko ~/$LINUX_BRANCH-videodev.ko
sudo cp $KBASE/drivers/media/common/videobuf2/videobuf2-common.ko ~/$LINUX_BRANCH-videobuf2-common.ko
sudo cp $KBASE/drivers/media/common/videobuf2/videobuf2-v4l2.ko ~/$LINUX_BRANCH-videobuf2-v4l2.ko

# Install all of the kernel modules into the /lib/modules/`uname -r`/kernel/drivers directory tree
# Note 1: compared to the scripts in the other existing Ubuntu LTS scripts for patching kernels, we 
#       have to unload more kernel modules that are related specifically to rpi4. The bcm2835 is 
#       the chip on the rpi4 that handles the rpi camera.
# Note 2: The utilities try_unload_module and try_module_insert are provided by the patch-utils.sh
#         try_module_insert will copy the existing .ko file in the destination folder to a backup file
#         and then copy the new module over the top of the original .ko. If you run this script twice,
#         you will lose the original modules, because the backups get copied over.
source $KBASE/../scripts/patch-utils.sh
try_unload_module uvcvideo
try_unload_module bcm2835_v4l2
try_unload_module videobuf2_v4l2
try_unload_module bcm2835_codec
try_unload_module bcm2835_isp
try_unload_module videobuf2_v4l2
try_unload_module videobuf2_common
try_unload_module videodev

try_module_insert videodev              ~/$LINUX_BRANCH-videodev.ko             /lib/modules/`uname -r`/kernel/drivers/media/v4l2-core/videodev.ko
try_module_insert videobuf2-common      ~/$LINUX_BRANCH-videobuf2-common.ko     /lib/modules/`uname -r`/kernel/drivers/media/common/videobuf2/videobuf2-common.ko
try_module_insert videobuf2-v4l2        ~/$LINUX_BRANCH-videobuf2-v4l2.ko       /lib/modules/`uname -r`/kernel/drivers/media/common/videobuf2/videobuf2-v4l2.ko
try_module_insert uvcvideo              ~/$LINUX_BRANCH-uvcvideo.ko             /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
try_module_insert hid_sensor_accel_3d   ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko  /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
try_module_insert hid_sensor_gyro_3d    ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko   /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko
