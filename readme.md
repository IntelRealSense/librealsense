<p align="center"><img src="doc/img/realsense.png" width="80%" /><br><br></p>

-----------------

Linux | Windows |
-------- | ------------ |
[![Build Status](https://travis-ci.org/IntelRealSense/librealsense.svg?branch=master)](https://travis-ci.org/IntelRealSense/librealsense) | [![Build status](https://ci.appveyor.com/api/projects/status/y9f8qcebnb9v41y4?svg=true)](https://ci.appveyor.com/project/ddiakopoulos/librealsense) |

## Overview
**Intel® RealSense™ SDK 2.0** is a cross-platform library (Linux, Windows, Mac) for working with Intel® RealSense™ depth cameras (SR300 and the RS400 series).
It allows streaming of depth, color and fisheye video, and provides intrinsic and extrinsic calibration information. The library also offers synthetic streams (pointcloud, depth aligned to color and vise-versa) and support for motion tracking. 

In addition, the SDK has a built-in support for recording a streaming session to a [rosbag](http://wiki.ros.org/rosbag) file, and playback of these files. 

The library aims to provide an easy to use API and tools for computer vision professionals, game developers and other Intel RealSense technology enthusiasts.

We provide a C, C++, and [Python](./bindings/python) API.

Developer kits containing the necessary hardware to use this library are available for purchase at [click.intel.com](http://click.intel.com/realsense.html).




## Quick Start

#### Step 1: Download and Install
After installing the Intel RealSense SDK (on [Linux](./doc/distribution_linux.md) \ Windows \ OSX), and connecting the depth camera, you are ready to start writing your first application!

> In case of any issue during initial setup, or later, please first see our [FAQ & Troubleshooting](./doc) section. 
> If you can't find an answer there, try searching our [Closed GitHub Issues](https://github.com/IntelRealSense/librealsense/issues?utf8=%E2%9C%93&q=is%3Aclosed) page,  [Community](https://communities.intel.com/community/tech/realsense) and [Support](https://www.intel.com/content/www/us/en/support/emerging-technologies/intel-realsense-technology.html) sites.
> If none of the above helped, [open a new issue](https://github.com/IntelRealSense/librealsense/issues/new).

#### Step 2: Ready to hack!

Our library offers a high level API for using Intel RealSense depth cameras (in addition to lower level ones).
The following snippet shows how to start streaming frames and extracting the depth value of a pixel:

```cpp
// Create a Pipeline, which serves as a top-level API for streaming and processing frames
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
    float frame_width = depth.get_width();
    float frame_height = depth.get_height();
    
    // Query the distance from the camera to the object in the center of the image
    float dist_to_center = depth.get_distance(frame_width / 2, frame_height / 2);
    
    // Print the distance 
    std::cout << "The camera is facing an object " << std::fixed << dist_to_center << " meters away \r";
}
```
For more information on the library, please follow our [examples](./examples), and read the [documentation](./doc) to learn more.


## License

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details
