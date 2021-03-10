#!/bin/bash -e
#
# Fedora 23 basically used Linux 4.4.x kernel already.
# This script downloads the kernel src RPM matched your current system,
# extracts the source then builds patched UVC module.

PATCH=`pwd`/`dirname ${BASH_SOURCE[0]}`/realsense-camera-formats.patch

# Install the fedora maintainer tool and kernel module development packages 
echo "Intall Fedora kernel development tools"
sudo dnf install fedora-packager kernel-devel

# Download and install kernel source RPM
RELEASE=`uname -r`	# e.g. 4.4.6-300.fc23.x86_64
RPMFILE=kernel-`echo $RELEASE | cut -d . -f 1-4`.src.rpm
if [ ! -f $RPMFILE ]; then
	echo "Download Fedora Linux kernel source RPM ... (~100MB)"
	koji download-build --arch=src $RPMFILE
fi

# Package dependency check
echo "Checking required packages ..."
sudo dnf builddep $RPMFILE

# Install kernel source and prepare build, take time and space ... (~1.2GB)
CONFIG_LOCATION=/usr/src/kernels/$RELEASE
rpm -Uvh $RPMFILE 2> /dev/null	# ignore warnings, install to ~/rpmbuild
cd ~/rpmbuild/SPECS
rpmbuild -bp --target=$(uname -m) kernel.spec 2> /dev/null

# Here is the trick, not build the whole RPM but patched UVC module only
cd `find ../BUILD -type d -name linux-$RELEASE`
cp $CONFIG_LOCATION/.config .
cp $CONFIG_LOCATION/Module.symvers .
patch -p1 < $PATCH
# TODO: This is to align vermagic parameter in the module;
#       there might be a straightforward way better than this
LOCAL=-`echo $RELEASE | cut -d - -f 2`	# e.g. -300.fc23.x86_64
LOCALVERSION=$LOCAL make modules_prepare
make M=drivers/media/usb/uvc modules

# Unload existing module if installed
# TODO: Fedora has enabled compressed module in xz format,
#       yet currently we insert a new uvcvideo.ko but not
#       backup nor replace original uvcvideo.ko.xz
echo "Replace existing uvcvideo driver"
sudo modprobe -r uvcvideo
sudo cp -f drivers/media/usb/uvc/uvcvideo.ko /lib/modules/$RELEASE/kernel/drivers/media/usb/uvc/
sudo modprobe uvcvideo

echo "Script has completed. Please consult the installation guide for further instruction."
