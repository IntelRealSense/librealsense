#!/bin/bash

if [ $(ls /dev/video* | wc -l) -ne 0 ];
then
	read -p "Remove all RealSense cameras attached. Hit any key when ready"
fi

#Include usability functions
source ./scripts/patch-utils.sh

#Additional packages to build patch
require_package libusb-1.0-0-dev
require_package libssl-dev

# Get the required tools and headers to build the kernel
sudo apt-get install linux-headers-generic build-essential git

LINUX_BRANCH=$(uname -r)

kernel_name="xenial"
# Get the linux kernel and change into source tree
[ ! -d ${kernel_name} ] && git clone https://git.launchpad.net/~wenchien/+git/xenial --depth 1
cd ${kernel_name}

# Verify that there are no trailing changes., warn the user to make corrective action if needed
if [ $(git status | grep 'modified:' | wc -l) -ne 0 ];
then
	echo -e "\e[36mThe kernel has modified files:\e[0m"
	git status | grep 'modified:'
	echo -e "\e[36mProceeding will reset all local kernel changes. Press 'n' within 10 seconds to abort the procedure"
	read -t 10 -r -p "Do you want to proceed? [Y/n]" response	
	response=${response,,}    # tolower
	if [[ $response =~ ^(n|N)$ ]]; 
	then
		echo -e "\e[41mScript has been aborted on user requiest. Please resolve the modified files are rerun\e[0m"
		exit 1
	else
		echo -e "\e[0m"
		printf "Resetting local changes in %s folder\n " ${kernel_name}
		git reset --hard
		echo -e "\e[32mUpdate the folder content with the latest from mainline branch\e[0m"
		git pull origin master
	fi
fi

#Check if we need to apply patches or get reload stock drivers (Developers' option)
[ "$#" -ne 0 -a "$1" == "reset" ] && reset_driver=1 || reset_driver=0

if [ $reset_driver -eq 1 ];
then 
	echo -e "\e[43mUser requested to rebuild and reinstall ubuntu-xenial stock drivers\e[0m"	
else
	# Patching kernel for RealSense devices
	echo -e "\e[32mApplying realsense-metadata patch\e[0m"
	patch -p1 < ../scripts/realsense-metadata-ubuntu-xenial-joule.patch	
	echo -e "\e[32mApplying realsense-hid patch\e[0m"
	patch -p1 < ../scripts/realsense-hid-ubuntu-xenial-joule.patch
fi

# Copy configuration
sudo cp /usr/src/linux-headers-$(uname -r)/.config .
sudo cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build so we can build just the uvcvideo module
#yes "" | make silentoldconfig modules_prepare
make silentoldconfig modules_prepare

# Build the uvc, accel and gyro modules
KBASE=`pwd`
cd drivers/media/usb/uvc
sudo cp $KBASE/Module.symvers .
echo -e "\e[32mCompiling uvc module\e[0m"
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules
echo -e "\e[32mCompiling accelerometer and gyro modules\e[0m"
make -C $KBASE M=$KBASE/drivers/iio/accel modules
make -C $KBASE M=$KBASE/drivers/iio/gyro modules

# Copy the patched modules to a sane location
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko
sudo cp $KBASE/drivers/iio/accel/hid-sensor-accel-3d.ko ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko
sudo cp $KBASE/drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko

echo -e "\e[32mPatched kernels modules created successfully\n\e[0m"

# Load the newly built modules
try_module_insert uvcvideo				~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
try_module_insert hid-sensor-accel-3d 	~/$LINUX_BRANCH-hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
try_module_insert hid-sensor-gyro-3d	~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko


echo -e "\e[92m\n\e[1mScript has completed. Please consult the installation guide for further instruction.\n\e[0m"
