REM Copyright (c) 2017 Intel Corporation. All rights reserved.
REM Use of this source code is governed by an Apache 2.0 license
REM that can be found in the LICENSE file.

@echo off
pushd .

cd librealsense
md build
cd build

REM Step 1
IF /I "%1"=="x64" (
	cmake -DBUILD_UNIT_TESTS=0 -DBUILD_EXAMPLES=0 -DBUILD_GRAPHICAL_EXAMPLES=0 -DCMAKE_GENERATOR_PLATFORM=x64 .. > NUL
) else (
	cmake -DBUILD_UNIT_TESTS=0 -DBUILD_EXAMPLES=0 -DBUILD_GRAPHICAL_EXAMPLES=0 .. > NUL
)

if errorlevel 1 (
	echo 'Failed to configure librealsense'
	exit 1
)

REM Step 2
msbuild librealsense2.sln /p:Configuration=Release /p:Platform=%2 /m:4
if errorlevel 1 (
	echo 'Failed to build librealsense'
	exit 2
)

cd ../..

popd
