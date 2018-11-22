#!/bin/bash

function pause() {
    read -p "$*"
}

SRC_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

LIBREALSENSE_DIR=$(dirname $(dirname ${SRC_DIR}))

if [ -z ${ANDROID_SDK} ]
then
    ANDROID_SDK=~/Android/Sdk
    echo "Please make sure Android SDK/CMake/NDK are installed in Android Studio!"
    echo "And Android SDK is stored in ${ANDROID_SDK}"

    pause 'Press [ENTER] key to continue ...'
fi

ANDROID_SDK_CMAKE=${ANDROID_SDK}/cmake/3.6.4111459/bin/cmake
ANDROID_SDK_NINJA=${ANDROID_SDK}/cmake/3.6.4111459/bin/ninja

function build() {
    ANDROID_ABI=$1
    echo "Building ${ANDROID_ABI}"
    if [ ! -d build-${ANDROID_ABI} ]; then
        mkdir build-${ANDROID_ABI}
    fi

    pushd build-${ANDROID_ABI}
    ${ANDROID_SDK_CMAKE} -GNinja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${ANDROID_SDK}/ndk-bundle/build/cmake/android.toolchain.cmake -DCMAKE_MAKE_PROGRAM:FILEPATH=${ANDROID_SDK_NINJA} -DANDROID_ABI:STRING=${ANDROID_ABI} -DANDROID_STL:STRING=c++_static -DUSE_SYSTEM_LIBUSB:BOOL=OFF -DFORCE_LIBUVC:BOOL=OFF -DBUILD_GRAPHICAL_EXAMPLES:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release ..
    ${ANDROID_SDK_NINJA}

    mkdir -p ${SRC_DIR}/libs/${ANDROID_ABI}
    cp librealsense2.a ${SRC_DIR}/libs/${ANDROID_ABI}/
    cp libusb_install/lib/libusb.a ${SRC_DIR}/libs/${ANDROID_ABI}/
    cp third-party/realsense-file/librealsense-file.a ${SRC_DIR}/libs/${ANDROID_ABI}/
    popd
}



pushd ${LIBREALSENSE_DIR}

build armeabi-v7a
build arm64-v8a
build x86
build x86_64

echo "Done."

popd

