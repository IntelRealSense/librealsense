# rs-dds-server tool

## Goal

RS DDS server tool is expected to run with a matching running  `librealsense` application on the client side demonstrates how `librealsense` SDK can detect a RS device being connected / disconnected on the server side while Client <> Server is connected throw Ethernet.

> librealsense on client side needs to be built with `BUILD_WITH_DDS=ON` CMake flag.

## Description

This tool will run and monitor librealsense2 `devices_changed` callback.

If the tool detects a RS device connected/disconnected, it will notify the `librealsense ` client through the DDS protocol.

## Command Line Parameters
| Flag | Description | Default|
|---|---|---|
|'-h --help'|Show command line help menu||
|'-d --domain < ID >'|DDS-Server will publish devices at domain < ID >|0|

## Expected Output
Assuming a running `librealsense` is found on the client side and network connection is stable.

We expect to see prints like:

 `Participant discovered`  --> The server found a valid client

`Device <device_name> - added` --> The device was added and a matching DDS data writer was created and publish the device information.

`Device <device_name> - removed`  --> The device was removes and a matching DDS data writer was destructed.