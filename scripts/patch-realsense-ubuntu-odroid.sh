#!/bin/bash

#This script is almost an exact copy of the patch-realsense-ubuntu-lts.sh script, tested on 4.14.47-132.
#There only two small additions / checks. Actually, this script also works for normal ubuntu-lts

#Break execution on any error received
set -e

#Locally suppress stderr to avoid raising not relevant messages
exec 3>&2
exec 2> /dev/null
con_dev=$(ls /dev/video* | wc -l)
exec 2>&3

if [ $con_dev -ne 0 ];
then
	echo -e "\e[32m"
	read -p "Remove all RealSense cameras attached. Hit any key when ready"
	echo -e "\e[0m"
fi

#Include usability functions
source ./scripts/patch-utils.sh

# Get the required tools and headers to build the kernel
sudo apt-get install linux-headers-generic build-essential git

#Packages to build the patched modules
require_package libusb-1.0-0-dev
require_package libssl-dev

retpoline_retrofit=0

LINUX_BRANCH=$(uname -r)
PLATFORM=$(uname -n)
if [ "$PLATFORM" = "odroid" ]; then
	kernel_branch="hwe"
else
	kernel_branch=$(choose_kernel_branch $LINUX_BRANCH)	
fi
echo "Kernel branch: " $kernel_branch
# Construct branch name from distribution codename {xenial,bionic,..} and kernel version
ubuntu_codename=`. /etc/os-release; echo ${UBUNTU_CODENAME/*, /}`
if [ -z "$UBUNTU_CODENAME" ];
then
	# Trusty Tahr shall use xenial code base
	ubuntu_codename="xenial"
	retpoline_retrofit=1
fi

kernel_name="ubuntu-${ubuntu_codename}-$kernel_branch"

#Distribution-specific packages
if [ ${ubuntu_codename} == "bionic" ];
then
	require_package libelf-dev
	require_package elfutils
fi


# Get the linux kernel and change into source tree
[ ! -d ${kernel_name} ] && git clone -b $kernel_branch git://kernel.ubuntu.com/ubuntu/ubuntu-${ubuntu_codename}.git --depth 1 ./${kernel_name}
cd ${kernel_name}

# Verify that there are no trailing changes., warn the user to make corrective action if needed
if [ $(git status | grep 'modified:' | wc -l) -ne 0 ];
then
	echo -e "\e[36mThe kernel has modified files:\e[0m"
	git status | grep 'modified:'
	echo -e "\e[36mProceeding will reset all local kernel changes. Press 'n' within 10 seconds to abort the operation"
	set +e
	read -n 1 -t 10 -r -p "Do you want to proceed? [Y/n]" response
	set -e
	response=${response,,}    # tolower
	if [[ $response =~ ^(n|N)$ ]]; 
	then
		echo -e "\e[41mScript has been aborted on user requiest. Please resolve the modified files are rerun\e[0m"
		exit 1
	else
		echo -e "\e[0m"
		echo -e "\e[32mUpdate the folder content with the latest from mainline branch\e[0m"
		git fetch origin $kernel_branch --depth 1
		printf "Resetting local changes in %s folder\n " ${kernel_name}
		git reset --hard $kernel_branch
	fi
fi

#Check if we need to apply patches or get reload stock drivers (Developers' option)
[ "$#" -ne 0 -a "$1" == "reset" ] && reset_driver=1 || reset_driver=0

if [ $reset_driver -eq 1 ];
then 
	echo -e "\e[43mUser requested to rebuild and reinstall ubuntu-${ubuntu_codename} stock drivers\e[0m"
else
	# Patching kernel for RealSense devices
	echo -e "\e[32mApplying realsense-uvc patch\e[0m"
	patch -p1 < ../scripts/realsense-camera-formats-${ubuntu_codename}-${kernel_branch}.patch
	echo -e "\e[32mApplying realsense-metadata patch\e[0m"
	patch -p1 < ../scripts/realsense-metadata-${ubuntu_codename}-${kernel_branch}.patch
	echo -e "\e[32mApplying realsense-hid patch\e[0m"
	patch -p1 < ../scripts/realsense-hid-${ubuntu_codename}-${kernel_branch}.patch
	echo -e "\e[32mApplying realsense-powerlinefrequency-fix patch\e[0m"
	patch -p1 < ../scripts/realsense-powerlinefrequency-control-fix.patch
fi

# Copy configuration
sudo cp /usr/src/linux-headers-$(uname -r)/.config .
sudo cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build for kernel modules
echo -e "\e[32mPrepare kernel modules configuration\e[0m"
#Retpoline script manual retrieval. based on https://github.com/IntelRealSense/librealsense/issues/1493
#Required since the retpoline patches were introduced into Ubuntu kernels
if [ ! -f scripts/ubuntu-retpoline-extract-one ]; then
	pwd
	for f in $(find . -name 'retpoline-extract-one'); do cp ${f} scripts/ubuntu-retpoline-extract-one; done;
	echo $$$
fi
sudo make silentoldconfig modules_prepare

#Vermagic identity is required
IFS='.' read -a kernel_version <<< "$LINUX_BRANCH"
sudo sed -i "s/\".*\"/\"$LINUX_BRANCH\"/g" ./include/generated/utsrelease.h
sudo sed -i "s/.*/$LINUX_BRANCH/g" ./include/config/kernel.release
#Patch for Trusty Tahr (Ubuntu 14.05) with GCC not retrofitted with the retpoline patch.
[ $retpoline_retrofit -eq 1 ] && sudo sed -i "s/#ifdef RETPOLINE/#if (1)/g" ./include/linux/vermagic.h

# Build the uvc, accel and gyro modules
KBASE=`pwd`
cd drivers/media/usb/uvc
sudo cp $KBASE/Module.symvers .

echo -e "\e[32mCompiling uvc module\e[0m"
sudo make -j -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules
echo -e "\e[32mCompiling accelerometer and gyro modules\e[0m"
sudo make -j -C $KBASE M=$KBASE/drivers/iio/accel modules
sudo make -j -C $KBASE M=$KBASE/drivers/iio/gyro modules
echo -e "\e[32mCompiling v4l2-core modules\e[0m"
sudo make -j -C $KBASE M=$KBASE/drivers/media/v4l2-core modules

# Copy and load the patched modules to a sane location
if [ -f $KBASE/drivers/media/usb/uvc/uvcvideo.ko ]; then
	echo "Copying uvcvideo.ko"
	sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko
	try_module_insert uvcvideo ~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
fi

if [ -f $KBASE/drivers/iio/accel/hid-sensor-accel-3d.ko ]; then
	echo "Copying hid-sensor-accel-3d.ko" 
	sudo cp $KBASE/drivers/iio/accel/hid-sensor-accel-3d.ko ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko
	try_module_insert hid_sensor_accel_3d ~/$LINUX_BRANCH-hid-sensor-accel-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
fi
if [ -f $KBASE/drivers/iio/gyro/hid-sensor-gyro-3d.ko ]; then
	echo "Copying hid-sensor-gyro-3d.ko"
	sudo cp $KBASE/drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko
	try_module_insert hid_sensor_gyro_3d ~/$LINUX_BRANCH-hid-sensor-gyro-3d.ko /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko
fi
if [ -f $KBASE/drivers/media/v4l2-core/videodev.ko ]; then
	echo "Copying videodev.ko"
	sudo cp $KBASE/drivers/media/v4l2-core/videodev.ko ~/$LINUX_BRANCH-videodev.ko
	try_module_insert videodev ~/$LINUX_BRANCH-videodev.ko /lib/modules/`uname -r`/kernel/drivers/media/v4l2-core/videodev.ko
fi
echo -e "\e[32mPatched kernels modules were created successfully\n\e[0m"


echo -e "\e[92m\n\e[1mScript has completed. Please consult the installation guide for further instruction.\n\e[0m"
