#!/bin/bash -e

# Install gcc
echo && echo Installing gcc-4.9 && echo
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-4.9 g++-4.9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.9

# Some dependency on openssl to compile UVC module
sudo apt-get install libssl-dev

# Update the kernel to v4.4-wily
echo && echo Upgrading kernel to v4.4-wily, will reboot upon completion... && echo
# Get headers and images
KERNEL_FOLDER=v4.4.6-wily
KERNEL_VERSION=4.4.6-040406
KERNEL_DATE=201603161231
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$KERNEL_FOLDER/linux-headers-${KERNEL_VERSION}_${KERNEL_VERSION}.${KERNEL_DATE}_all.deb -P /tmp/
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$KERNEL_FOLDER/linux-headers-${KERNEL_VERSION}-generic_${KERNEL_VERSION}.${KERNEL_DATE}_amd64.deb -P /tmp/
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/$KERNEL_FOLDER/linux-image-${KERNEL_VERSION}-generic_${KERNEL_VERSION}.${KERNEL_DATE}_amd64.deb -P /tmp/

# Install
sudo dpkg -i /tmp/*.deb

# Update grub
sudo update-grub

echo "Finished! In order to complete the Kernel upgrade you must reboot."
