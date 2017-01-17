#!/bin/bash -e
#set -x
LINUX_BRANCH=$(uname -r) 

# Get the required tools and headers to build the kernel
source ./scripts/patch_utils.sh


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
	fi
fi

# Apply UVC formats patch for RealSense devices
patch -p1 < ../scripts/realsense-camera-formats_ubuntu-xenial.patch

# Copy configuration
cp /usr/src/linux-headers-$(uname -r)/.config .
cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Basic build so we can build just the uvcvideo module
#yes "" | make silentoldconfig modules_prepare
make silentoldconfig modules_prepare

# Build the uvc modules
KBASE=`pwd`
cd drivers/media/usb/uvc
cp $KBASE/Module.symvers .
make -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules

# Copy to sane location
sudo cp $KBASE/drivers/media/usb/uvc/uvcvideo.ko ~/$LINUX_BRANCH-uvcvideo.ko

# Unload existing module if installed 
echo "Unloading existing uvcvideo driver..."
sudo modprobe -r uvcvideo

#backup the existing uvcmodule
sudo cp /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko.bckup

# Copy the patched driver out to module directory
sudo cp ~/$LINUX_BRANCH-uvcvideo.ko /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko

# try to load the new uvc module
set --
modprobe_failed=0
echo "try to load the new uvc module"
sudo modprobe uvcvideo || modprobe_failed=$?

# Check and revert the original module if 'modprobe' operation crashed
if [ $modprobe_failed -ne 0 ];
then
    echo "Failed to insert the patched uvcmodule. Operation is aborted, the original uvcmodule is restored"
    echo "Verify that the current kernel version is aligned to the patched module versoin"
    sudo cp /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko.bckup /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
    sudo modprobe uvcvideo
    exit 1
else
	# Delete original module
	echo "Inserting the patched uvcmodule succeeded"
	sudo rm /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
fi

echo "Script has completed. Please consult the installation guide for further instruction."
