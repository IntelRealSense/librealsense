# Node.js API for Intel® RealSense™ Depth Cameras

This module works with Intel® RealSense™ D400 series camera (and SR300 camera). It's part of the Intel® [librealsense](https://github.com/IntelRealSense/librealsense) open source project.

# Sample Usage #

```
const rs2 = require('node-librealsense');

const colorizer = new rs2.Colorizer();  // This will make depth image pretty
const pipeline = new rs2.Pipeline();  // Main work pipeline of RealSense camera
pipeline.start();  // Start camera

const frameset = pipeline.waitForFrames();  // Get a set of frames
const depth = frameset.depthFrame;  // Get depth data
const depthRGB = colorizer.colorize(depth);  // Make depth image pretty
const color = frameset.colorFrame;  // Get RGB image

// TODO: use frame buffer data
depthRGB.getData();
color.getData();

// Before exiting, do cleanup.
rs2.cleanup();
```

More examples can be found in [examples directory](https://github.com/IntelRealSense/librealsense/tree/development/wrappers/nodejs/examples) of the module.

## 1. Install Prerequisites ##

### Setup Windows 10 Build Environment ###

 1. Install Python 2.7.xx, make sure "`Add python.exe to Path`" is checked during the installation.

 1. Install Visual Studio 2015 or 2017. The `Visual Studio 2017 Community` version also works.
    After installation, make sure `msbuild.exe` is in PATH, e.g "`C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin`"

 1. Install CMake, make sure `CMake` is in system PATH (Choose "`Add CMake to the system PATH for all users`" or "`Add CMake to the system PATH for the current user`" during the installation).

Note: The npm module `windows-build-tools` is not suffcient to build the native C++ librealsense.

### Setup Ubuntu Linux 16.04 Build Environment ###

```
sudo apt install -y libusb-1.0-0-dev pkg-config libgtk-3-dev libglfw3-dev cmake
```

Please refer to [Linux installation doc](https://github.com/IntelRealSense/librealsense/blob/development/doc/installation.md) or [Windows installation doc](https://github.com/IntelRealSense/librealsense/blob/development/doc/installation_windows.md) for full document of C++ librealsense build environment setup.

### Setup Mac OS Build Environment ###

**Note:** OSX support for the full range of functionality offered by the SDK is not yet complete.

 1. Install XCode 6.0+ via the AppStore.

 2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)

 3. Install the following packages via brew:
   * `brew install libusb pkg-config`
   * `brew install homebrew/versions/glfw3`
   * `brew install cmake`

### Setup Necessary Global NPM Packages ###

```
npm install -g jsdoc
```

## 2. Instal node-librealsense Module ##

```
npm install --save node-librealsense
```
It will take a while to build C++ librealsense library, and then the Node.js addon will be built. If both of them succeed, the node-librealsense module is ready to use.

## 3. Run Examples ##

When it's installed, you can run examples to see if it works fine. Plug in your Intel® RealSense™ camera and do the following:

```
cd node-librealsense/examples
npm install
node nodejs-capture.js
```
### List of Examples ##

1. `nodejs-align.js`: capture and then align RGB image frames to depth image frames, using depth info to remove background by a distance threshold.
1. `nodejs-capture.js`: display RGB image frames and colorized depth image frames that are captured in real time
1. `nodejs-save-to-disk.js`: capture a RGB image frame and a depth image frame, then save both of them to disk file (*.png)
1. `nodejs-pointcloud.js`: capture RGB image frames and depth image frames, and then use them to generate and visualize textured 3D pointcloud

## 4. API Reference Document ##
Open `node-librealsense/doc/index.html` for full reference document. If it isn't there, run the following commands to generate it:

```
cd node-librealsense/
npm run doc
```

## 5. Compatibility ##

List of supported platforms
 - Windows 10 + Node.js x64 & ia32 - supported
 - Windows 8.1 + Node.js x64 & ia32 - theoretically supported. Not verified yet.
 - Ubuntu 16.04 + Node.js x64 & ia32 - supported
 - Node.js v8 - supported
 - Node.js v6 LTS - You might need to [upgrade npm-bundled `node-gyp`](https://github.com/nodejs/node-gyp/wiki/Updating-npm%27s-bundled-node-gyp) to support Visual Studio 2017 (if you're using it)
