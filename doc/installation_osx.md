# macOS Installation  

**Note:** macOS support for the full range of functionality offered by the SDK is not yet complete. If you need support for R200 or the ZR300, [legacy librealsense](https://github.com/IntelRealSense/librealsense/tree/legacy) offers a subset of SDK functionality.

**Note:** The T265 tracking module is not yet fully supported on macOS via librealsense. Support is planned to be added in a future release.

## Building from Source

1. Install CommantLineTools `sudo xcode-select --install` or download XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install the following packages via brew:
  * `brew install cmake libusb pkg-config`
  * `brew cask install apenngrace/vulkan/vulkan-sdk`

**Note** *librealsense* requires CMake version 3.8+ that can also be obtained via the [official CMake site](https://cmake.org/download/).  


4. Generate XCode project:
  * `mkdir build && cd build`
  * `sudo xcode-select --reset`
  * `cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false`
5. Build the Project
  * `make -j2`

> **Note:** On some Mac systems you might encounter `ld: library not found for -lusb-1.0` error (either in the terminal during make or in XCode) This can be worked-around by setting environment variable: `/bin/launchctl setenv LIBRARY_PATH /usr/local/lib`

  **Note:** You can find more information about the available configuration options on [this wiki page](https://github.com/IntelRealSense/librealsense/wiki/Build-Configuration).

## Packaging your application
1. librealsense requires libusb to be bundled in the application. To fix the real-time linking, use `install_name_tool`
```
install_name_tool -change /usr/local/opt/libusb/lib/libusb-1.0.0.dylib @rpath/libusb-1.0.0.dylib librealsense2.dylib
```
2. Copy `libusb-1.0.0.dylib` and `librealsense2.dylib` to your application's `Frameworks` folder
