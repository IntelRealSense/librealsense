#!/bin/bash -e
#set -x
LINUX_BRANCH=$(uname -r) 

# Get the required tools and headers to build the kernel
source ./scripts/patch-utils.sh


#Additional packages to build patch
require_package libusb-1.0-0-dev
require_package libssl-dev

sudo apt-get install linux-headers-generic build-essential

kernel_name="ubuntu-xenial"
# Get the linux kernel and change into source tree
[ ! -d ${kernel_name} ] && git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git --depth 1
cd ${kernel_name}

# Verify that there are no trailing changes., warn the user to make corrective action if needed
if [ $(git status | grep 'modified:' | wc -l) -ne 0 ];
then
	echo "The kernel sources include modified files that will be lost if you continue:"
	git status | grep 'modified:'	
	read -r -p "Are you sure to proceed ? [Y/n] " response	
	response=${response,,}    # tolower
	if [[ $response =~ ^(n|N)$ ]]; 
	then
		printf "Script has been aborted on user requiest. Please resolve the modified files are rerun"
		exit 1
	else
		printf "Resetting local changes in %s folder\n " ${kernel_name}
		git reset --hard
		printf "Update the folder content with the latest from mainline branch\n "
		git pull origin master
	fi
fi

# Apply UVC formats patch for RealSense devices
echo "Applying realsense-uvc patch"
patch -p1 < ../scripts/realsense-camera-formats_ubuntu-xenial.patch
echo "Applying realsense-hid patch"
patch -p1 < ../scripts/realsense-hid-ubuntu-xenial.patch

# Copy configuration
cp /usr/src/linux-headers-$(uname -r)/.config .
cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build so we can build just the uvcvideo module
#yes "" | make silentoldconfig modules_prepare
make silentoldconfig modules_prepare

# Build the uvc, accel and gyro modules
KBASE=`pwd`
cd drivers/media/usb/uvc
cp $KBASE/Module.symvers .
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules
make -C $KBASE M=$KBASE/drivers/iio/accel modules
make -C $KBASE M=$KBASE/drivers/iio/gyro modules

# Copy the patched modules to a sane location
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko
sudo cp $KBASE/drivers/iio/accel/hid-sensor-accel-3d.ko ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko
sudo cp $KBASE/drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko

# Load the newly built modules
try_module_insert uvcvideo				~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
try_module_insert hid-sensor-accel-3d 	~/$LINUX_BRANCH-hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
try_module_insert hid-sensor-gyro-3d	~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko

## Unload existing modules if resident
#echo "Unloading existing uvcvideo driver..."
#sudo modprobe -r uvcvideo
#echo "Unloading existing accel and gyro modules..."
#sudo modprobe -r hid-sensor-accel-3d
#sudo modprobe -r hid-sensor-gyro-3d

##backup the existing modules
#sudo cp /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko.bckup
#sudo cp /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko.bckup
#sudo cp /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko.bckup

## Copy the patched modules out to target directory for deployment
#sudo cp ~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
#sudo cp ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
#sudo cp ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko

#sudo modprobe hid-sensor-accel-3d
#sudo modprobe hid-sensor-gyro-3d

## try to load the new uvc module
#set --
#modprobe_failed=0
#echo "try to load the new uvc module"
#sudo modprobe uvcvideo || modprobe_failed=$?

## Check and revert the original module if 'modprobe' operation crashed
#if [ $modprobe_failed -ne 0 ];
#then
#    echo "Failed to insert the patched uvcmodule. Operation is aborted, the original uvcmodule is restored"
#    echo "Verify that the current kernel version is aligned to the patched module versoin"
#    sudo cp /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko.bckup /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
#    sudo modprobe uvcvideo
#    exit 1
#else
#	# Delete original module
#	echo "Inserting the patched uvcmodule succeeded"
#	sudo rm /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
#fi

echo "Script has completed. Please consult the installation guide for further instruction."
