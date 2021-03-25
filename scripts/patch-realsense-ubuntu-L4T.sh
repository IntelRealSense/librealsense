#!/bin/bash
# The script utilizes `sources_sync.sh` provided as
# part of NVidia SDK installer

echo -e "\e[32mThe script patches and applies in-tree kernel modules required for Librealsense SDK\e[0m"
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

# Get the required tools to build the patched modules
sudo apt-get install build-essential git -y

#Activate fan to prevent overheat during KM compilation
if [ -f /sys/devices/pwm-fan/target_pwm ]; then
	echo 200 | sudo tee /sys/devices/pwm-fan/target_pwm || true
fi

#Tegra-specific
KERNEL_RELEASE="4.9"
#Identify the Jetson board
JETSON_BOARD=$(tr -d '\0' </proc/device-tree/model)
echo -e "\e[32mJetson Board (proc/device-tree/model): ${JETSON_BOARD}\e[0m"

JETSON_L4T=""
# With L4T 32.3.1, NVIDIA added back /etc/nv_tegra_release
if [ -f /etc/nv_tegra_release ]; then
	JETSON_L4T_STRING=$(head -n 1 /etc/nv_tegra_release)
	JETSON_L4T_RELEASE=$(echo $JETSON_L4T_STRING | cut -f 2 -d ' ' | grep -Po '(?<=R)[^;]+')
	# Extract revision + trim trailing zeros to convert 32.5.0 => 32.5 to match git tags
	JETSON_L4T_REVISION=$(echo $JETSON_L4T_STRING | cut -f 2 -d ',' | grep -Po '(?<=REVISION: )[^;]+' | sed 's/.0$//g')
	JETSON_L4T_VERSION=$JETSON_L4T_RELEASE.$JETSON_L4T_REVISION
	echo -e "\e[32mJetson L4T version: ${JETSON_L4T_VERSION}\e[0m"
else
	echo -e "\e[41m/etc/nv_tegra_release not present, aborting script\e[0m"
	exit;
fi

PATCHES_REV=""
#Select the kernel patches revision that matches the paltform configuration
case ${JETSON_L4T_VERSION} in

  "32.2.1" | "32.3.1" | "32.4.3")
    PATCHES_REV="4.4"		# Baseline for the patches
    echo -e "\e[32mNote: the patch makes changes to kernel device tree to support HID IMU sensors\e[0m"
    ;;
  "32.4.4" | "32.5")
    PATCHES_REV="4.4.1"	# JP 4.4.1
    ;;
  *)
    echo -e "\e[41mUnsipported JetPack revision ${JETSON_L4T_VERSION} aborting script\e[0m"
    exit;
    ;;
esac
echo -e "\e[32mL4T ${JETSON_L4T_VERSION} to use patches revision ${PATCHES_REV}\e[0m"


# Get the linux kernel repo, extract the L4T tag
echo -e "\e[32mRetrieve the corresponding L4T git tag the kernel source tree\e[0m"
l4t_gh_dir=../linux-${KERNEL_RELEASE}-source-tree
if [ ! -d ${l4t_gh_dir} ]; then
	mkdir ${l4t_gh_dir}
	pushd ${l4t_gh_dir}
	git init
	git remote add origin git://nv-tegra.nvidia.com/linux-${KERNEL_RELEASE}
	# Use Nvidia script instead to synchronize source tree and peripherals
	#git clone git://nv-tegra.nvidia.com/linux-${KERNEL_RELEASE}
	popd
else
	echo -e "Directory ${l4t_gh_dir} is present, skipping initialization...\e[0m"
fi

#Search the repository for the tag that matches the maj.min for L4T
pushd ${l4t_gh_dir}
TEGRA_TAG=$(git ls-remote --tags origin | grep ${JETSON_L4T_VERSION} | grep '[^^{}]$' | tail -n 1 | awk -F/ '{print $NF}')
echo -e "\e[32mThe matching L4T source tree tag is \e[47m${TEGRA_TAG}\e[0m"
popd


#Retrieve tegra tag version for sync, required for get and sync kernel source with Jetson:
#https://forums.developer.nvidia.com/t/r32-1-tx2-how-can-i-build-extra-module-in-the-tegra-device/72942/9
#Download kernel and peripheral sources as the L4T github repo is not self-contained to build kernel modules
sdk_dir=$(pwd)
echo -e "\e[32mCreate the sandbox - NVidia L4T source tree(s)\e[0m"
mkdir -p ${sdk_dir}/Tegra
cp ./scripts/Tegra/source_sync.sh ${sdk_dir}/Tegra
#Download NVidia source, disregard errors on module tag sync
sudo ./Tegra/source_sync.sh -k ${TEGRA_TAG} || true
KBASE=./Tegra/sources/kernel/kernel-4.9
echo ${KBASE}
pushd ${KBASE}

echo -e "\e[32mCopy LibRealSense patches to the sandbox\e[0m"
L4T_Patches_Dir=${sdk_dir}/scripts/Tegra/LRS_Patches/
if [ ! -d ${L4T_Patches_Dir} ]; then
	echo -e "\e[41mThe L4T kernel patches directory  ${L4T_Patches_Dir} was not found, aborting\e[0m"
	exit 1
else
	sudo cp -r ${L4T_Patches_Dir} .
fi

#Clean the kernel WS
echo -e "\e[32mPrepare workspace for kernel build\e[0m"
sudo make ARCH=arm64 mrproper -j$(($(nproc)-1)) && sudo make ARCH=arm64 tegra_defconfig -j$(($(nproc)-1))

#Reuse existing module.symver
kernel_ver=`uname -r`
sudo cp /usr/src/linux-headers-${kernel_ver}-ubuntu18.04_aarch64/kernel-4.9/Module.symvers .

#Jetpack prior to 4.4.1 requires manual reconfiguration of kernel
if [ "4.4" == PATCHES_REV ]; then
	echo -e "\e[32mUpdate the kernel tree to support HID IMU sensors\e[0m"
	sudo sed -i '/CONFIG_HID_SENSOR_ACCEL_3D/c\CONFIG_HID_SENSOR_ACCEL_3D=m' .config
	sudo sed -i '/CONFIG_HID_SENSOR_GYRO_3D/c\CONFIG_HID_SENSOR_GYRO_3D=m' .config
	sudo sed -i '/CONFIG_HID_SENSOR_IIO_COMMON/c\CONFIG_HID_SENSOR_IIO_COMMON=m\nCONFIG_HID_SENSOR_IIO_TRIGGER=m' .config
fi
sudo make ARCH=arm64 prepare modules_prepare  -j$(($(nproc)-1))

#Remove previously applied patches
sudo git reset --hard
echo -e "\e[32mApply Librealsense Kernel Patches\e[0m"
sudo -s patch -p1 < ./LRS_Patches/01-realsense-camera-formats-L4T-${PATCHES_REV}.patch
sudo -s patch -p1 < ./LRS_Patches/02-realsense-metadata-L4T-${PATCHES_REV}.patch
if [ "4.4" == PATCHES_REV ]; then # for Jetpack 4.4 and older
	sudo -s patch -p1 < ./LRS_Patches/03-realsense-hid-L4T-4.9.patch
fi
sudo -s patch -p1 < ./LRS_Patches/04-media-uvcvideo-mark-buffer-error-where-overflow.patch
sudo -s patch -p1 < ./LRS_Patches/05-realsense-powerlinefrequency-control-fix.patch

echo -e "\e[32mCompiling uvcvideo kernel module\e[0m"
#sudo -s make -j -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules
sudo -s make -j$(($(nproc)-1)) ARCH=arm64 M=drivers/media/usb/uvc/ modules
echo -e "\e[32mCompiling v4l2-core modules\e[0m"
#sudo -s make -j -C $KBASE M=$KBASE/drivers/media/v4l2-core modules
sudo -s make -j$(($(nproc)-1)) ARCH=arm64  M=drivers/media/v4l2-core modules
if [ "4.4" == PATCHES_REV ]; then # for Jetpack 4.4 and older
	echo -e "\e[32mCompiling accelerometer and gyro modules\e[0m"
	sudo -s make -j$(($(nproc)-1)) ARCH=arm64  M=drivers/iio modules
fi

echo -e "\e[32mCopying the patched modules to (~/) \e[0m"
sudo cp drivers/media/usb/uvc/uvcvideo.ko ~/${TEGRA_TAG}-uvcvideo.ko
sudo cp drivers/media/v4l2-core/videobuf-vmalloc.ko ~/${TEGRA_TAG}-videobuf-vmalloc.ko
sudo cp drivers/media/v4l2-core/videobuf-core.ko ~/${TEGRA_TAG}-videobuf-core.ko
if [ "4.4" == PATCHES_REV ]; then # for Jetpack 4.4 and older
	sudo cp drivers/iio/common/hid-sensors/hid-sensor-iio-common.ko ~/${TEGRA_TAG}-hid-sensor-iio-common.ko
	sudo cp drivers/iio/common/hid-sensors/hid-sensor-trigger.ko ~/${TEGRA_TAG}-hid-sensor-trigger.ko
	sudo cp drivers/iio/accel/hid-sensor-accel-3d.ko ~/${TEGRA_TAG}-hid-sensor-accel-3d.ko
	sudo cp drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/${TEGRA_TAG}-hid-sensor-gyro-3d.ko
fi
popd

echo -e "\e[32mMove the modified modules into the modules tree\e[0m"
if [ "4.4" == PATCHES_REV ]; then # for Jetpack 4.4 and older
#Optional - create kernel modules directories in kernel tree
	sudo mkdir -p /lib/modules/`uname -r`/kernel/drivers/iio/accel
	sudo mkdir -p /lib/modules/`uname -r`/kernel/drivers/iio/gyro
	sudo mkdir -p /lib/modules/`uname -r`/kernel/drivers/iio/common/hid-sensors
	sudo cp  ~/${TEGRA_TAG}-hid-sensor-accel-3d.ko     /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
	sudo cp  ~/${TEGRA_TAG}-hid-sensor-gyro-3d.ko      /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko
	sudo cp  ~/${TEGRA_TAG}-hid-sensor-iio-common.ko   /lib/modules/`uname -r`/kernel/drivers/iio/common/hid-sensors/hid-sensor-iio-common.ko
	sudo cp  ~/${TEGRA_TAG}-hid-sensor-trigger.ko      /lib/modules/`uname -r`/kernel/drivers/iio/common/hid-sensors/hid-sensor-trigger.ko
fi

# update kernel module dependencies
sudo depmod

echo -e "\e[32mInsert the modified kernel modules\e[0m"
try_module_insert uvcvideo              ~/${TEGRA_TAG}-uvcvideo.ko                /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
if [ "4.4" == PATCHES_REV ]; then # for Jetpack 4.4 and older
	try_module_insert hid_sensor_accel_3d   ~/${TEGRA_TAG}-hid-sensor-accel-3d.ko     /lib/modules/`uname -r`/kernel/drivers/iio/accel/hid-sensor-accel-3d.ko
	try_module_insert hid_sensor_gyro_3d    ~/${TEGRA_TAG}-hid-sensor-gyro-3d.ko      /lib/modules/`uname -r`/kernel/drivers/iio/gyro/hid-sensor-gyro-3d.ko
	#Preventively unload all HID-related modules
	try_unload_module hid_sensor_accel_3d
	try_unload_module hid_sensor_gyro_3d
	try_unload_module hid_sensor_trigger
	try_unload_module hid_sensor_trigger
	try_module_insert hid_sensor_trigger    ~/${TEGRA_TAG}-hid-sensor-trigger.ko      /lib/modules/`uname -r`/kernel/drivers/iio/common/hid-sensors/hid-sensor-trigger.ko
	try_module_insert hid_sensor_iio_common ~/${TEGRA_TAG}-hid-sensor-iio-common.ko   /lib/modules/`uname -r`/kernel/drivers/iio/common/hid-sensors/hid-sensor-iio-common.ko
fi
echo -e "\e[92m\n\e[1mScript has completed. Please consult the installation guide for further instruction.\n\e[0m"
