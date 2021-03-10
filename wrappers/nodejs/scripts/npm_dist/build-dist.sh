#!/bin/bash

# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

#sudo apt install -y libusb-1.0-0-dev pkg-config libgtk-3-dev libglfw3-dev cmake

echo 'Buliding librealsense C++ API...'
pushd .  > /dev/null
pwd

cd librealsense
mkdir -p build
cd build

# Step 1
cmake -DBUILD_UNIT_TESTS=0 -DBUILD_EXAMPLES=0 -DBUILD_GRAPHICAL_EXAMPLES=0 ..
if [ "$?" -gt 0 ]
then
	echo 'Failed to configure librealsense'
	exit 1
fi

# Step 2
CPU_NUM=`grep -c ^processor /proc/cpuinfo`
make -j $CPU_NUM
if [ "$?" -gt 0 ]
then
	echo 'Failed to build librealsense'
	exit 2
fi

cd ../..

popd > /dev/null
