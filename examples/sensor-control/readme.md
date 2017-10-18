# Sensor Control Sample

## Overview

The sensor control samples aims to provide the entry point and a tutorial for using the more advanced API that the RealSense SDK provides.
In addition to the `r2::pipeline` API, which simplifies streaming flow, the SDK provides the `rs2::context`, `rs2::device`, and `rs2::sensor` APIs that provide an abstraction for RealSense devices.

In this sample we show how to use this API for querying all connected devices, their sensors, and their streams.
Using a `rs2::sensor` one can control a camera's options, stream from it, and get the frames directly without having them synchronize (as the `rs2::pipeline` does).

## Expected Output

Running this example with a connected Intel RealSense device, will output the device name and some of its details.
For each one of the device's sensors, the output will contain a list of supported stream profiles, and if all goes well a number of frames should arrive and print their number and stream type to theo utput.

## Code Overview

Please refer to the actual code for details documentation and explanations
