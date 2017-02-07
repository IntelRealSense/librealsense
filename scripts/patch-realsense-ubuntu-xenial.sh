#!/bin/bash -e

# Get the required tools and headers to build the kernel
sudo apt-get install linux-headers-generic build-essential git

if [ $(ls /dev/video* | wc -l) -ne 0 ];
then
	echo -e "\e[32m"
	read -p "First, remove all RealSense cameras attached. Hit any key when ready"
	echo -e "\e[0m"
fi

#Include usability functions
source ./scripts/patch-utils.sh

#Additional packages to build patch
require_package libusb-1.0-0-dev
require_package libssl-dev

LINUX_BRANCH=$(uname -r)
kernel_name="ubuntu-xenial"

# Download the latest the linux kernel sources from ubuntu-xenial tree
[ ! -d ${kernel_name} ] && git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git --depth 1
cd ${kernel_name}

# Verify that there are no trailing changes., warn the user to make corrective action if needed
if [ $(git status | grep 'modified:' | wc -l) -ne 0 ];
then
	echo -e "\e[36mThe kernel has modified files:\e[0m"
	git status | grep 'modified:'
	echo -e "\e[36mProceeding will reset all local kernel changes. Press 'n' within 10 seconds to abort the procedure"
	set +e
	read -t 10 -r -p "Do you want to proceed? [Y/n]" response
	set -e
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
	#Patching kernel for RealSense devices
	echo -e "\e[32mApplying F200 formats patch patch\e[0m"
	patch -p1 < ../"$( dirname "$0" )"/0001-Add-video-formats-for-Intel-real-sense-F200-camera-new.patch
	echo -e "\e[32mApplying ZR300 SR300 and LR200 formats patch\e[0m"
	patch -p1 < ../"$( dirname "$0" )"/0002-LR200-ZR300-and-SR300-Pixel-Formats.patch
fi

# Copy configuration
sudo cp /usr/src/linux-headers-$(uname -r)/.config .
sudo cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build so we can build just the uvcvideo module
#yes "" | make silentoldconfig modules_prepare
make scripts silentoldconfig modules_prepare

# Build the uvc, accel and gyro modules
KBASE=`pwd`
cd drivers/media/usb/uvc
sudo cp $KBASE/Module.symvers .
echo -e "\e[32mCompiling uvc module\e[0m"
sudo make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

# Copy the patched modules to a sane location
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko

echo -e "\e[32mPatched kernel module created successfully\n\e[0m"

# Load the newly built module(s)
try_module_insert uvcvideo	~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko

echo -e "\e[92m\n\e[1mScript has completed successfully. Please consult the installation guide for further instruction.\n\e[0m"
