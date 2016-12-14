#!/bin/sh
#
# DKMS PRE_BUILD script
# Script to be run before a build is performed.
#
# This script checks if the existing loadable kernel module
# needs to be patched. Applies the correct patch if necessary.
#

set -e

echo ""
echo "dkms: Pre-build script started..."
echo ""

if [ ! -z ${1-x} ]
then
	kernelver="$1"
else
    echo "Missing kernelver, can not build"
    exit 2
fi

PACKAGE_NAME="uvcvideo"
PACKAGE_VERSION="1.1.1-3-realsense"

PATCH_DIR="/usr/src/${PACKAGE_NAME}-${PACKAGE_VERSION}/patches/"
PATCH="/dev/null"

# Clean up old source link first
if [ -L linux-src ]
then
    /bin/rm -rf $(readlink linux-src)
    /bin/rm -rf linux-src
fi

echo "Downloading kernel sources..."
# Get the linux kernel and change into source tree
apt-get source linux-image-${kernelver} | egrep -v '^ ' || true
workdir=$(pwd)
cd linux-* 1>/dev/null 2>/dev/null || true
currentdir=$(pwd)
if [ ${workdir} = ${currentdir} ]
then
    echo ""
    echo "ERROR: Could not download the required kernel sources to install RealSense camera support."
    echo ""
    echo "       To resolve, please follow the installation directions at:"
    echo "           http://wiki.ros.org/librealsense#Installation_Prerequisites"
    echo ""
    exit 86
fi

# Create a known path for the kernel sources
/bin/ln -s $(basename $(pwd)) ../linux-src

# Check for uvcvideo patches in drivers/media/usb/uvc/uvc_driver.c
UVC_MODULE_PATH="drivers/media/usb/uvc"
PATCHED_FORMATS=$(/bin/egrep '\((Y8I|Y12I|Z16|SRGGB10P|RAW8|RW16|INVZ|INZI|INVR|INRI|INVI|RELI|L8|L16|D16)\)' \
    ${UVC_MODULE_PATH}/uvc_driver.c | /usr/bin/wc -l)
case "${PATCHED_FORMATS}" in
  15)
    echo "INFO: Intel RealSense(TM) F200, SR300, R200, LR200, and ZR300 cameras are already supported."
  ;;
  4)
    PATCH="${PATCH_DIR}/realsense-uvcvideo-add-to-upstreamed.patch"
    echo "INFO: Only Intel RealSense(TM) R200 camera is currently supported."
  ;;
  *)
    PATCH="${PATCH_DIR}/realsense-uvcvideo-no-upstreamed.patch"
    echo "INFO: No Intel RealSense(TM) cameras are currently supported."
  ;;
esac

if [ ${PATCH} != "/dev/null" ]
then
    echo "Patching uvcvideo sources..."
    # Apply the one, simple UVC format patch
    patch -p1 < ${PATCH}
fi

# Copy configuration
cp /usr/src/linux-headers-${kernelver}/.config .
cp /usr/src/linux-headers-${kernelver}/Module.symvers .

# Basic build so we can build just the uvcvideo module
# Ensure 'olddefconfig' is first so default choices are selected
make olddefconfig scripts modules_prepare

exit 0
