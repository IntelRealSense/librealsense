#!/bin/bash

set -e

echo -e "\033[0;31m\nThis script is solely for DSO-1369 - Failing Reliability Testing."
echo -e "\033[0;31mThis script will attempt to install Linux Kernel on this machine."
echo -e "\033[0;31m\n**RUNNING THIS SCRIPT IS AT YOUR OWN RISK!**\n"
read -r -p "Do you want to proceed? [Y/n]" response
response=${response,,}    # tolower
if [[ $response =~ ^(n|N)$ ]]; 
then
echo -e "\033[0;31m\nScript Aborted!"
exit 1
fi

set -e

#Include usability functions
source ./scripts/patch-utils.sh

#Additional packages to build patch
require_package libusb-1.0-0-dev
require_package libssl-dev

# Get the required tools and headers to build the kernel
sudo apt-get install linux-headers-generic build-essential git

kernel_name="ubuntu-xenial"
# Get the linux kernel and change into source tree
[ ! -d ${kernel_name} ] && git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git --depth 1
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

# LibRealSense and XHCI patches
echo -e "\e[32mApplying realsense-uvc patch\e[0m"
patch -p1 < "../scripts/realsense-camera-formats_ubuntu-xenial.patch"
echo -e "\e[32mApplying realsense-metadata patch\e[0m"
patch -p1 < "../scripts/realsense-metadata-ubuntu-xenial.patch"
echo -e "\e[32mApplying realsense-hid patch\e[0m"
patch -p1 < "../scripts/realsense-hid-ubuntu-xenial.patch"
echo -e "\e[32mApplying 01-xhci-Add-helper-to-get-hardware-dequeue-pointer-for patch\e[0m"
patch -p1 < "../scripts/01-xhci-Add-helper-to-get-hardware-dequeue-pointer-for.patch"
echo -e "\e[32mApplying 02-xhci-Add-stream-id-to-to-xhci_dequeue_state-structur patch\e[0m"
patch -p1 < "../scripts/02-xhci-Add-stream-id-to-to-xhci_dequeue_state-structur.patch"
echo -e "\e[32mApplying 03-xhci-Find-out-where-an-endpoint-or-stream-stopped-fr patch\e[0m"
patch -p1 < "../scripts/03-xhci-Find-out-where-an-endpoint-or-stream-stopped-fr.patch"
echo -e "\e[32mApplying 04-xhci-remove-unused-stopped_td-pointer patch\e[0m"
patch -p1 < "../scripts/04-xhci-remove-unused-stopped_td-pointer.patch"

# Copy Kernel configuration files
sudo cp /usr/src/linux-headers-$(uname -r)/.config .
sudo cp /usr/src/linux-headers-$(uname -r)/Module.symvers .

# Compile and Install the patched Kernel
make silentoldconfig modules_prepare
sudo make -j8
sudo make modules -j8
sudo make modules_install -j8
sudo make install

echo -e "\e[92m\n\e[1m`sudo make kernelrelease` Kernel has been successfully installed."
echo -e "\e[92m\n\e[1mScript has completed. Please reboot and load the newly installed Kernel from GRUB list.\n\e[0m"

