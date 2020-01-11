## Useful Links
* White papers and further documents are available at: [intel-realsense-downloads](https://realsense.intel.com/intel-realsense-downloads/)
* [Intel RealSense Community](https://communities.intel.com/community/tech/realsense) - Official support, Q&A and other useful content
* [Support Site](http://www.intel.com/content/www/us/en/support/emerging-technologies/intel-realsense-technology.html) - Contains content and web ticket capability for 1:1 interaction
* [Intel RealSense Stereoscopic Depth Cameras](https://arxiv.org/abs/1705.05548) - A technical paper describing the R200, LR200, SR300 and RS400 in detail. Includes theoretical background, performance expectations, post-processing suggestions, etc.  
* [Build an Autonomous Mobile Robot with the Intel® RealSense™ D400 Series Camera, ROS*, and SAWR](https://software.intel.com/en-us/articles/build-an-autonomous-mobile-robot-with-the-intel-realsense-camera-ros-and-sawr)

## librealsense Wiki
* [API How-To](https://github.com/IntelRealSense/librealsense/wiki/API-How-To) - List ot questions and answers related to the camera API
* [Troubleshooting Q&A](https://github.com/IntelRealSense/librealsense/wiki/Troubleshooting-Q&A) - List of questions and answers related to camera setup
* [Build Configuration](https://github.com/IntelRealSense/librealsense/wiki/Build-Configuration) - List of supported CMake flags
* [Projection In RealSense SDK 2.0](https://github.com/IntelRealSense/librealsense/wiki/Projection-in-RealSense-SDK-2.0) - A comprehensive document on Projection, Deprojection, and related helper processing-blocks the SDK provides

## Documentation

* [Depth from Stereo](depth-from-stereo.md) - Tutorial introducing users to depth from stereo technology 
* [API Architecture](api_arch.md) - Overview of the high-level concepts
* [C API](../include/librealsense2) - With doxygen-style API comments
  * [Latest Doxygen Output](http://intelrealsense.github.io/librealsense/doxygen/annotated.html)
  * To build documentation locally from sources, on Ubuntu run the following commands:
    * `sudo apt-get install doxygen`
    * `cd doc/doxygen`
    * `doxygen doxyfile`
  * Please also refer to [API How-To](https://github.com/IntelRealSense/librealsense/wiki/API-How-To) for further doc how to use the APIs
* [Frame Management](frame_lifetime.md) - Frame Memory Management, Threading Model and Synchronization
* [Frame Metadata](frame_metadata.md) - Support for frame-metadata attributes
* [Post-Processing Filters](post-processing-filters.md) - Post-acquisition Depth enhancement techniques
* [Getting Started](stepbystep/getting_started_with_openCV.md) - Getting started with OpenCV
* [Error Handling](error_handling.md) - Documents librealsense error handling policy
* Device specific topics:
  * [D400 at realsense.intel.com/](https://realsense.intel.com/stereo) - Camera specifications
  * [D400 Advanced Mode](rs400/rs400_advanced_mode.md) - Overview of the Advanced Mode APIs
  * [D400 cameras with Raspberry Pi](./RaspberryPi3.md) - Example of low-end system without USB3 interface
  * [D400 cameras on **rooted** Android devices](./android.md) - Instructions of how to build the RealSense SDK for Android OS.
  * [D435i depth camera with IMU](./d435i.md) - An introduction to the D435i depth camera and it's IMU
  * [T265 tracking module](./t265.md) - An introduction to the T265 tracking module
* [Record and Playback](../src/media/readme.md) - SDK Record and Playback functionality using ROS-bag file format
