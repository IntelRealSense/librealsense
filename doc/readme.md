## Useful Links
* [Intel RealSense Community](https://communities.intel.com/community/tech/realsense) - Official support, Q&A and other useful content
* [Support Site](http://www.intel.com/content/www/us/en/support/emerging-technologies/intel-realsense-technology.html) - Contains content and web ticket capability for 1:1 interaction
* [SDK Design Guidelines](http://www.mouser.com/pdfdocs/intel-realsense-sdk-design-r200.pdf) - Guidelines and tips for designing applications using RealSense cameras
* [R200 Datasheet](http://www.mouser.com/pdfdocs/intel_realsense_camera_r200.pdf) - In-depth information about the R200 camera hardware.
* [Intel RealSense Stereoscopic Depth Cameras](https://arxiv.org/abs/1705.05548) - A technical paper describing the R200, LR200, SR300 and RS400 in detail. Includes theoretical background, performance expectations, post-processing suggestions, etc.  

## Documentation

* [API Architecture](api_arch.md) - Overview of the high-level concepts
* [C API](../include/librealsense/rs.h) - With doxygen-style API comments
* To build documentation locally from sources, on Ubuntu run the following commands:
  * `sudo apt-get install doxygen`
  * `doxygen doc/doxygen/doxyfile`
* [Frame Management](frame_lifetime.md) - Frame Memory Management, Threading Model and Synchronization
* [Frame Metadata](frame_metadata.md) - support for frame-metadata attributes
* [Getting Started](stepbystep/getting_started_with_openCV.md) - Getting started with OpenCV
* [Troubleshooting](troubleshooting.md) - Useful tips when debugging issues related to the camera
* [Error Handling](error_handling.md) - Documents librealsense error handling policy
* Device specific topics:
  * [RS400 and External Devices](rs400/external_devices.md) - Notes on integrating RS400 with external devices
  * [RS400 Advanced Mode](rs400/rs400_advanced_mode.md) - Overview of the Advanced Mode APIs
* [Record and Playback](../src/media/readme.md) - SDK Record and Playback functionality using ROS-bag file format