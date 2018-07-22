# macOS Installation  

**Note:** macOS support for the full range of functionality offered by the SDK is not yet complete. If you need support for R200 or the ZR300, [legacy librealsense](https://github.com/IntelRealSense/librealsense/tree/legacy) offers a subset of SDK functionality.

## Building from Source

1. Install XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install the following packages via brew:
  * `brew install libusb pkg-config`
  * `brew install homebrew/core/glfw3`
  * `brew install cmake`

**Note** *librealsense* requires CMake version 3.8+ that can be obtained via the [official CMake site](https://cmake.org/download/).  


4. Generate XCode project:
  * `mkdir build && cd build`
  * `sudo xcode-select --reset`
  * `cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode`
5. Open and build the XCode project

## What works?
* SR300, D415 and D435 will stream depth, infrared and color at all supported resolutions
* The Viewer, Depth-Quality Tool and most of the examples should work

## What are the known issues?
* Changing configurations will often result in a crash or the new configuration not being applied (WIP)
