#!/bin/bash

rm -rf librealsense-dylib

mkdir librealsense-dylib
mkdir librealsense-dylib/bin
mkdir librealsense-dylib/include
mkdir librealsense-dylib/bindings
mkdir librealsense-dylib/examples
mkdir librealsense-dylib/librealsense.xc
mkdir librealsense-dylib/unit-tests

cp -r include/ librealsense-dylib/include
cp -r bindings/ librealsense-dylib/bindings
cp -r examples/ librealsense-dylib/examples
cp -r librealsense.xc/ librealsense-dylib/librealsense.xc
cp -r unit-tests/ librealsense-dylib/unit-tests

cp COPYING librealsense-dylib/
cp readme.md librealsense-dylib/

rm -rf librealsense-dylib/librealsense.xc/librealsense.xcodeproj
rm -rf librealsense-dylib/librealsense.xc/librealsense.xcworkspace
rm -rf librealsense-dylib/librealsense.xc/build

rm librealsense-dylib/examples/*.cs
rm librealsense-dylib/bindings/*.cs

cd librealsense.xc

xcodebuild -project librealsense.xcodeproj clean
xcodebuild -project librealsense.xcodeproj -configuration Release

cp build/Release/librealsense.dylib ../librealsense-dylib/bin

cd ..

zip -r librealsense-dylib.zip librealsense-dylib

rm -rf librealsense-dylib/