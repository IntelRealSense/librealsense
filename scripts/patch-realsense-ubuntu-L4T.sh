#!/bin/bash
# The script utilizes `sources_sync.sh` provided as
# part of NVIDIA(R) SDK installer

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
source ./scripts/patch-utils-hwe.sh

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
	"32.2.1" | "32.2.3" | "32.3.1" | "32.4.3")
		PATCHES_REV="4.4"		# Baseline for the patches
		echo -e "\e[32mNote: the patch makes changes to kernel device tree to support HID IMU sensors\e[0m"
	;;
	"32.4.4" | "32.5" | "32.5.1" | "32.6.1" | "32.7.1")
		PATCHES_REV="4.4.1"	# JP 4.4.1, 32.7.1 is JP 4.6.1
	;;
	"35.1")
		PATCHES_REV="5.0.2"	# JP 5.0.2
		KERNEL_RELEASE="5.10"
	;;
	"36.3")
		PATCHES_REV="6.0"	# JP 6.0
		KERNEL_RELEASE="5.15"
	;;
  *)
	echo -e "\e[41mUnsupported JetPack revision ${JETSON_L4T_VERSION} aborting script\e[0m"
	exit;
	;;
esac
echo -e "\e[32mL4T ${JETSON_L4T_VERSION} to use patches revision ${PATCHES_REV}\e[0m"

# Get the required tools to build the patched modules
sudo apt-get install build-essential git libssl-dev -y
if [ "6.0" != "$PATCHES_REV" ]; then
# Get the linux kernel repo, extract the L4T tag
	echo -e "\e[32mRetrieve the corresponding L4T git tag the kernel source tree\e[0m"
	l4t_gh_dir=../linux-${KERNEL_RELEASE}-source-tree
	if [ ! -d ${l4t_gh_dir} ]; then
		mkdir ${l4t_gh_dir}
		pushd ${l4t_gh_dir}
		git init
		git remote add origin git://nv-tegra.nvidia.com/linux-${KERNEL_RELEASE}
		# Use NVIDIA script instead to synchronize source tree and peripherals
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
fi

#Retrieve tegra tag version for sync, required for get and sync kernel source with Jetson:
#https://forums.developer.nvidia.com/t/r32-1-tx2-how-can-i-build-extra-module-in-the-tegra-device/72942/9
#Download kernel and peripheral sources as the L4T github repo is not self-contained to build kernel modules
sdk_dir=$(pwd)
echo -e "\e[32mCreate the sandbox - NVIDIA L4T source tree(s)\e[0m"
mkdir -p ${sdk_dir}/Tegra
TEGRA_SOURCE_SYNC_SH="source_sync.sh"
if [ "5.0.2" = "$PATCHES_REV" ]; then
	TEGRA_SOURCE_SYNC_SH="source_sync_5.0.2.sh"
fi
if [ "6.0" = "$PATCHES_REV" ]; then
	TEGRA_SOURCE_SYNC_SH="source_sync_6.0.sh"
	TEGRA_TAG="jetson_36.3"
fi
cp ./scripts/Tegra/$TEGRA_SOURCE_SYNC_SH ${sdk_dir}/Tegra
#Download NVIDIA source, disregard errors on module tag sync
./Tegra/$TEGRA_SOURCE_SYNC_SH -k ${TEGRA_TAG} || true
if [ "6.0" = "$PATCHES_REV" ]; then
	KBASE=./Tegra/kernel/kernel-jammy-src
else
	KBASE=./Tegra/sources/kernel/kernel-$KERNEL_RELEASE
fi

echo ${KBASE}
pushd ${KBASE}

echo -e "\e[32mCopy LibRealSense patches to the sandbox\e[0m"
L4T_Patches_Dir=${sdk_dir}/scripts/Tegra/LRS_Patches/
if [ ! -d ${L4T_Patches_Dir} ]; then
	echo -e "\e[41mThe L4T kernel patches directory  ${L4T_Patches_Dir} was not found, aborting\e[0m"
	exit 1
else
	cp -r ${L4T_Patches_Dir} .
fi

#Clean the kernel WS
echo -e "\e[32mPrepare workspace for kernel build\e[0m"

if [ "6.0" = "$PATCHES_REV" ]; then
	make ARCH=arm64 mrproper -j$(($(nproc)-1)) 
	echo -e "\e[32mUpdate the kernel tree to support HID IMU sensors\e[0m"
	# appending config to defconfig so later .config will be generated with all necessary dependencies
	echo 'CONFIG_HID_SENSOR_HUB=m' >> ./arch/arm64/configs/defconfig
	echo 'CONFIG_HID_SENSOR_ACCEL_3D=m' >> ./arch/arm64/configs/defconfig
	echo 'CONFIG_HID_SENSOR_GYRO_3D=m' >> ./arch/arm64/configs/defconfig
	echo 'CONFIG_HID_SENSOR_IIO_COMMON=m' >> ./arch/arm64/configs/defconfig
	echo 'CONFIG_HID_SENSOR_IIO_TRIGGER=m' >> ./arch/arm64/configs/defconfig
	make ARCH=arm64 defconfig -j$(($(nproc)-1))
else
	make ARCH=arm64 mrproper -j$(($(nproc)-1)) && make ARCH=arm64 tegra_defconfig -j$(($(nproc)-1))
fi
#Reuse existing module.symver
kernel_ver=`uname -r`
LINUX_HEADERS_NAME="linux-headers-${kernel_ver}-ubuntu18.04_aarch64"
if [ "5.0.2" = "$PATCHES_REV" ]; then
  LINUX_HEADERS_NAME="linux-headers-${kernel_ver}-ubuntu20.04_aarch64"
fi
if [ "6.0" = "$PATCHES_REV" ]; then
  LINUX_HEADERS_NAME="linux-headers-${kernel_ver}-ubuntu22.04_aarch64"
fi

if [ "6.0" = "$PATCHES_REV" ]; then
	# we will skip copy module symbols as it will result in failing to compile iio core modules
	#cp /usr/src/$LINUX_HEADERS_NAME/3rdparty/canonical/linux-jammy/kernel-source/Module.symvers .
	#cp /usr/src/$LINUX_HEADERS_NAME/3rdparty/canonical/linux-jammy/kernel-source/Makefile .
	echo ""
else
	cp /usr/src/$LINUX_HEADERS_NAME/kernel-$KERNEL_RELEASE/Module.symvers .
	cp /usr/src/$LINUX_HEADERS_NAME/kernel-$KERNEL_RELEASE/Makefile .
fi

#Jetpack prior to 4.4.1 requires manual reconfiguration of kernel
if [ "4.4" = "$PATCHES_REV" ]; then
	echo -e "\e[32mUpdate the kernel tree to support HID IMU sensors\e[0m"
	sed -i '/CONFIG_HID_SENSOR_ACCEL_3D/c\CONFIG_HID_SENSOR_ACCEL_3D=m' .config
	sed -i '/CONFIG_HID_SENSOR_GYRO_3D/c\CONFIG_HID_SENSOR_GYRO_3D=m' .config
	sed -i '/CONFIG_HID_SENSOR_IIO_COMMON/c\CONFIG_HID_SENSOR_IIO_COMMON=m\nCONFIG_HID_SENSOR_IIO_TRIGGER=m' .config
fi

make ARCH=arm64 prepare modules_prepare LOCALVERSION='' -j$(($(nproc)-1))

#Remove previously applied patches
git reset --hard
echo -e "\e[32mApply Librealsense Kernel Patches\e[0m"
if [ "6.0" != "$PATCHES_REV" ]; then
	patch -p1 < ./LRS_Patches/01-realsense-camera-formats-L4T-${PATCHES_REV}.patch
	patch -p1 < ./LRS_Patches/02-realsense-metadata-L4T-${PATCHES_REV}.patch
	if [ "4.4" = "$PATCHES_REV" ]; then # for Jetpack 4.4 and older
		patch -p1 < ./LRS_Patches/03-realsense-hid-L4T-4.9.patch
	fi
	if [ "5.0.2" != "$PATCHES_REV" ]; then
		patch -p1 < ./LRS_Patches/04-media-uvcvideo-mark-buffer-error-where-overflow.patch
	fi
	patch -p1 < ./LRS_Patches/05-realsense-powerlinefrequency-control-fix.patch
fi

if [ "6.0" = "$PATCHES_REV" ]; then
	patch -p1 < ${sdk_dir}/scripts/realsense-camera-formats-jammy-master.patch
	patch -p1 < ${sdk_dir}/scripts/realsense-metadata-jammy-master.patch
	patch -p1 < ${sdk_dir}/scripts/realsense-powerlinefrequency-control-fix-jammy.patch
	sed -i s'/1.1.1/1.1.1-realsense/'g ./drivers/media/usb/uvc/uvcvideo.h
fi

echo -e "\e[32mCompiling uvcvideo kernel module\e[0m"
#sudo -s make -j -C $KBASE M=$KBASE/drivers/media/usb/uvc/ modules
make -j$(($(nproc)-1)) ARCH=arm64 M=drivers/media/usb/uvc/ modules
echo -e "\e[32mCompiling v4l2-core modules\e[0m"
#sudo -s make -j -C $KBASE M=$KBASE/drivers/media/v4l2-core modules
make -j$(($(nproc)-1)) ARCH=arm64  M=drivers/media/v4l2-core modules
if [ "4.4" = "$PATCHES_REV" ]; then # for Jetpack 4.4 and older
	echo -e "\e[32mCompiling accelerometer and gyro modules\e[0m"
	make -j$(($(nproc)-1)) ARCH=arm64  M=drivers/iio modules
fi
if [ "6.0" = "$PATCHES_REV" ]; then # for Jetpack 6.0
	echo -e "\e[32mCompiling hid support, accelerometer and gyro modules\e[0m"
	make -j$(($(nproc)-1)) ARCH=arm64  M=drivers/hid modules
	make -j$(($(nproc)-1)) ARCH=arm64  M=drivers/iio modules
fi
if [ "6.0" != "$PATCHES_REV" ]; then # for Jetpack 4-5
	echo -e "\e[32mCopying the patched modules to (~/) \e[0m"
	sudo cp drivers/media/usb/uvc/uvcvideo.ko ~/${TEGRA_TAG}-uvcvideo.ko
	sudo cp drivers/media/v4l2-core/videobuf-vmalloc.ko ~/${TEGRA_TAG}-videobuf-vmalloc.ko
	sudo cp drivers/media/v4l2-core/videobuf-core.ko ~/${TEGRA_TAG}-videobuf-core.ko
else
	echo -e "\e[32mCopying the patched modules to destination \e[0m"
fi
if [ "4.4" = "$PATCHES_REV" ]; then # for Jetpack 4.4 and older
	sudo cp drivers/iio/common/hid-sensors/hid-sensor-iio-common.ko ~/${TEGRA_TAG}-hid-sensor-iio-common.ko
	sudo cp drivers/iio/common/hid-sensors/hid-sensor-trigger.ko ~/${TEGRA_TAG}-hid-sensor-trigger.ko
	sudo cp drivers/iio/accel/hid-sensor-accel-3d.ko ~/${TEGRA_TAG}-hid-sensor-accel-3d.ko
	sudo cp drivers/iio/gyro/hid-sensor-gyro-3d.ko ~/${TEGRA_TAG}-hid-sensor-gyro-3d.ko
fi

if [ "6.0" = "$PATCHES_REV" ]; then # for Jetpack 6
	sudo mkdir -p /lib/modules/$(uname -r)/extra/
	# uvc modules with formats/sku support
	sudo cp drivers/media/usb/uvc/uvcvideo.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/media/v4l2-core/videodev.ko /lib/modules/$(uname -r)/extra/
	# iio modules for iio-hid support
	sudo cp drivers/iio/buffer/kfifo_buf.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/iio/buffer/industrialio-triggered-buffer.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/iio/common/hid-sensors/hid-sensor-iio-common.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/hid/hid-sensor-hub.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/iio/accel/hid-sensor-accel-3d.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/iio/gyro/hid-sensor-gyro-3d.ko /lib/modules/$(uname -r)/extra/
	sudo cp drivers/iio/common/hid-sensors/hid-sensor-trigger.ko /lib/modules/$(uname -r)/extra/
	# set depmod search path to include "extra" modules
	sudo sed -i 's/search updates/search extra updates/g' /etc/depmod.d/ubuntu.conf
fi
popd
echo -e "\e[32mMove the modified modules into the modules tree\e[0m"
if [ "4.4" = "$PATCHES_REV" ]; then # for Jetpack 4.4 and older
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
if [ "6.0" != "$PATCHES_REV" ]; then
	try_module_insert uvcvideo              ~/${TEGRA_TAG}-uvcvideo.ko                /lib/modules/`uname -r`/kernel/drivers/media/usb/uvc/uvcvideo.ko
	try_load_module  uvcvideo
	try_load_module  hid-sensor-gyro-3d
	try_load_module  hid-sensor-accel-3d
else
	# for JP6.0 we will try to remove old modules and then load updated ones
	echo -e "\e[32mUnload kernel modules\e[0m"
	try_unload_module uvcvideo
	try_unload_module hid_sensor_accel_3d
	try_unload_module hid_sensor_gyro_3d
	try_unload_module hid_sensor_trigger
	try_unload_module industrialio_triggered_buffer
	try_unload_module kfifo_buf
	echo -e "\e[32mLoad modified kernel modules\e[0m"
	try_load_module kfifo_buf
	try_load_module industrialio_triggered_buffer
	try_load_module hid_sensor_trigger
	try_load_module hid_sensor_gyro_3d
	try_load_module hid_sensor_accel_3d
	try_load_module uvcvideo
	echo -e "\e[32mDone\e[0m"
fi
if [ "4.4" = "$PATCHES_REV" ]; then # for Jetpack 4.4 and older
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
