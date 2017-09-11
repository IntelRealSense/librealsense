<p align="center"><img src="doc/img/realsense.png" width="70%" /><br><br></p>

-----------------

Linux | Windows |
-------- | ------------ |
[![Build Status](https://travis-ci.org/IntelRealSense/librealsense.svg?branch=development)](https://travis-ci.org/IntelRealSense/librealsense) | [![Build status](https://ci.appveyor.com/api/projects/status/6u04bgmpwfejpgo8?svg=true)](https://ci.appveyor.com/project/dorodnic/librealsense-s4xnv) |

## Overview
**Intel® RealSense™ SDK 2.0** is a cross-platform library for working with Intel® RealSense™ depth cameras (D400 series and the SR300).

> For other Intel® RealSense™ devices, please refer to the [latest legacy release](https://github.com/IntelRealSense/librealsense/tree/v1.12.1).

The SDK allows depth, color and fisheye streaming, and provides intrinsic and extrinsic calibration information.
The library also offers synthetic streams (pointcloud, depth aligned to color and vise-versa), and a built-in support for [record and playback](./src/media/readme.md) of streaming sessions.

The library aims to provide an easy to use API and tools for computer vision professionals, game developers and other **Intel® RealSense™** technology enthusiasts.

We provide a C, C++, [Python](./wrappers/python) and [Node.js](./wrappers/nodejs) API.
Developer kits containing the necessary hardware to use this library are available for purchase at [click.intel.com](http://click.intel.com/realsense.html).

## Quick Start

#### Step 1: Download and Install
After installing the Intel RealSense SDK (on [Linux](./doc/distribution_linux.md) \ [Windows](./doc/distribution_windows.md) \ OSX), and connecting the depth camera, you are ready to start writing your first application.

> In case of any issue during initial setup, or later, please first see our [FAQ & Troubleshooting](https://github.com/IntelRealSense/librealsense/wiki/Troubleshooting-Q%26A) section.
> If you can't find an answer there, try searching our [Closed GitHub Issues](https://github.com/IntelRealSense/librealsense/issues?utf8=%E2%9C%93&q=is%3Aclosed) page,  [Community](https://communities.intel.com/community/tech/realsense) and [Support](https://www.intel.com/content/www/us/en/support/emerging-technologies/intel-realsense-technology.html) sites.
> If none of the above helped, [open a new issue](https://github.com/IntelRealSense/librealsense/issues/new).

#### Step 2: Ready to Hack!

Our library offers a high level API for using Intel RealSense depth cameras (in addition to lower level ones).
The following snippet shows how to start streaming frames and extracting the depth value of a pixel:

```cpp
// Create a Pipeline - this serves as a top-level API for streaming and processing frames
rs2::pipeline p;

// Configure and start the pipeline
p.start();

while (true)
{
    // Block program until frames arrive
    rs2::frameset frames = p.wait_for_frames(); 
    
    // Try to get a frame of a depth image
    rs2::depth_frame depth = frames.get_depth_frame(); 
    // The frameset might not contain a depth frame, if so continue until it does
    if (!depth) continue;

    // Get the depth frame's dimensions
    float width = depth.get_width();
    float height = depth.get_height();
    
    // Query the distance from the camera to the object in the center of the image
    float dist_to_center = depth.get_distance(width / 2, height / 2);
    
    // Print the distance 
    std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";
}
```
For more information on the library, please follow our [examples](./examples), and read the [documentation](./doc) to learn more.

## Contributing
In order to contribute to Intel RealSense SDK, please follow our [contribution guidelines](CONTRIBUTING.md).

## License
This project is licensed under the [Apache License, Version 2.0](LICENSE). 
Copyright 2017 Intel Corporation