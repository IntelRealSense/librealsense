# librealsense2 Node.js Wrapper
This is the Node.js wrapper for the C++ `librealsense2` for Intel® RealSense™ depth cameras (D400 series and the SR300).

# 1. Build from Source #

## 1.1. Install Build Prerequisites

### Install Node.js
Install [`Node.js`](https://nodejs.org/en/download/)

You will probably need to setup [proxy of npm](https://docs.npmjs.com/misc/config#proxy) or [https proxy of npm](https://docs.npmjs.com/misc/config#https-proxy), if you don't have direct internet connection.

### Setup Build Environment

#### Setup Windows 10 Build Environment

 1. Install Python 2.7.xx, make sure "`Add python.exe to Path`" is checked during the installation.

 1. Install Visual Studio 2015 or 2017. The `Community` version also works.

 1. Install CMake, make sure `CMake` is in system PATH (Choose "`Add CMake to the system PATH for all users`" or "`Add CMake to the system PATH for the current user`" during the installation). This step is for `npm install` of Node.js GLFW module that is used in `wrappers/nodejs/examples`.

Note#1: The npm module `windows-build-tools` also works for `npm install` Node.js bindings, but it's not suffcient to build the native C++ librealsense2.

Note#2: for Node.js GLFW module, add `msbuild` to system PATH, e.g "`C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin`"

#### Setup Ubuntu Linux 16.04 Build Environment

TODO:

## 1.2. Build Native C++ librealsense2 ##

Please refer to [Linux installation doc](../../doc/installation.md) or [Windows installation doc](../../doc/installation_windows.md) to build native C++ librealsense2.

## 1.3. Build Node.js Module/Addon ##

```
cd wrappers/nodejs
npm install
```

# 2. Run Examples

When Node.js wrapper is built, you can run examples to see if it works. Plug your Intel® RealSense™ camera and run the following commands

```
cd wrappers/nodejs/examples
npm install
node nodejs-capture.js
```

# 3. API Reference Document
Open `wrappers/nodejs/doc/index.html` for full reference document. If it isn't there, run the following commands to generate it:

```
cd wrappers/nodejs
node scripts/generate-doc.js
```
