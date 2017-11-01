# Sensor Control Sample

## Overview

The sensor control samples aims to provide the entry point and a tutorial for using the more advanced API that the RealSense SDK provides.
In addition to the `r2::pipeline` API, which simplifies streaming flow, the SDK provides the `rs2::context`, `rs2::device`, and `rs2::sensor` APIs that provide an abstraction for RealSense devices.

In this sample we show how to use this API for querying all connected devices, their sensors, and their streams.
Using a `rs2::sensor` one can control a camera's options, stream from it, and get the frames directly without having them processed or synchronized (as the `rs2::pipeline` does).

## Expected Output

Running this example with a connected Intel RealSense device, will output the connected device names, and requests the user to select a camera (or connect one).
After selecting a camera, the user will be prompt to select one of the device's sensor, and select which action to perform using that sensor.
This flow will repeat itself as long as the user desires.

## Code Overview

Please follow the [`api_how_to.h`](./api_how_to.h) for detailed documentation and explanations
This file contains detailed explanations and code on how to do the following:

- [get a realsense device](./api_how_to.h#22)
- [print device information](./api_how_to.h#71)
- [get device name](./api_how_to.h#94)
- [get sensor name](./api_how_to.h#109)
- [get a sensor from a device](./api_how_to.h#118)
- [get sensor option](./api_how_to.h#147)
- [get depth units](./api_how_to.h#193)
- [get field of view](./api_how_to.h#209)
- [get extrinsics](./api_how_to.h#243)
- [change sensor option](./api_how_to.h#262)
- [choose a streaming profile](./api_how_to.h#315)
- [start streaming a profile](./api_how_to.h#408)
