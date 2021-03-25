#!/bin/bash

# set the kernel version, like 5.0.21
KV1=5
KV2=0
KV3=21
# get the rt number, like rt10 on 4.x and rt16 on 5.x
RT=16

# use -c to continue existing downloads
wget -c https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/linux-${KV1}.${KV2}.${KV3}.tar.xz
wget -c https://mirrors.edge.kernel.org/pub/linux/kernel/projects/rt/${KV1}.${KV2}/patch-${KV1}.${KV2}.${KV3}-rt${RT}.patch.xz

# use -k, keep the input files so they don't have to be redownloaded if you restart the script
# use -f, overwrite existing in the case of restart
unxz -kf linux-${KV1}.${KV2}.${KV3}.tar.xz
unxz -kf patch-${KV1}.${KV2}.${KV3}-rt${RT}.patch.xz

tar xf linux-${KV1}.${KV2}.${KV3}.tar
cd linux-${KV1}.${KV2}.${KV3}

patch -p1 < ../patch-${KV1}.${KV2}.${KV3}-rt${RT}.patch

# this will apply all patches
patch -p1 < ../*.patch

cp /boot/config-$(uname -r) .config

echo "/boot/config* has been copied to .config"
echo "now run \`make oldconfig\` to finish configuring kernel options"

echo ""
echo ""

echo "Before building, edit .config"
echo -e "\tset CONFIG_SYSTEM_TRUSTED_KEYS = "" in .config otherwise the build will fail"
echo -e "\t\tThis is used to bake additional trusted X.509 keys directly into the kernel image, which can be used to verify kernel modules before loading them. Debian 10 comes with this configuration, but we won’t actually need this, so we will remove it."
echo -e "\tset CONFIG_DEBUG_INFO=n in .config"
echo -e "\t\tThis will not build dbg images, saving 20GB+"

echo ""
echo ""

echo "For Vanilla Linux:"
echo -e "\tmake"
echo -e "\tmake modules_install"
echo -e "\tmake install"
echo -e "\tupdate-initramfs -c -k ${KV1}.${KV2}.${KV3}"
echo -e "\tupdate-grub"
echo ""
echo ""
echo "COPY AND PASTE THE ABOVE OUTPUT TO ANOTHER CONSOLE SO YOU HAVE IT WHEN THIS ONE FILLS UP"
