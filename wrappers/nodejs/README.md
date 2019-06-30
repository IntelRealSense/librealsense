# librealsense2 Node.js Wrapper
This is the Node.js wrapper for the C++ `librealsense2` for Intel® RealSense™ depth cameras (D400 series and the SR300).

## Notice: Before You Go Further ##

**To librealsense contributors**: this document is for developers who want to fork [librealsense](https://github.com/IntelRealSense/librealsense) and make changes to source code. (pull requests are welcome)

**To Node.js users**: if you're using Node.js and want to use [Node.js librealsense API](https://www.npmjs.com/package/node-librealsense), please use the following command to install it directly:
```
npm install --save node-librealsense
```
If it fails to install, please refer to [install prerequisites](https://www.npmjs.com/package/node-librealsense#1-install-prerequisites). Usage examples can be found in `node_modules/node-librealsense/examples` dir.

# 1. Build from Source #

## 1.1. Install Build Prerequisites

### Install Node.js

#### Ubuntu 16.04
In Ubuntu16.04, the default apt-get installed version is lower than v6.x. The following command could install the latest v6.x release:

```
curl -sL https://deb.nodesource.com/setup_6.x | sudo -E bash -
sudo apt-get install -y nodejs
```
#### Windows 10
Please download the latest .msi from [here](https://nodejs.org/en/download/) and install it.

#### Mac OS
**Note:** OSX support for the full range of functionality offered by the SDK is not yet complete.

Install the [Homebrew package manager](http://brew.sh/) via terminal if not installed, then run the following command to install node:
```
brew install node
```

#### Verfication
The version can be checked through this command:

```
node -v
```
#### Install required modules
After Node.js is installed, run the following command to install required modules.

```
npm install -g jsdoc     # Required for document generation
npm install -g node-gyp  # This is optional
```

You will probably need to setup [proxy of npm](https://docs.npmjs.com/misc/config#proxy) or [https proxy of npm](https://docs.npmjs.com/misc/config#https-proxy), if you don't have direct internet connection.

### Setup Build Environment

Environment is ready if you're using Ubuntu 16.04.

If you're using Windows 10, please do the following steps:

 1. Install Python 2.7.xx, make sure "`Add python.exe to Path`" is checked during the installation.

 1. Install Visual Studio 2015 or 2017. The `Community` version also works.

 1. Install CMake, make sure `CMake` is in system PATH (Choose "`Add CMake to the system PATH for all users`" or "`Add CMake to the system PATH for the current user`" during the installation). This step is for `npm install` of Node.js GLFW module that is used in `wrappers/nodejs/examples`.

Note#1: The npm module `windows-build-tools` also works for `npm install` Node.js bindings, but it's not suffcient to build the native C++ librealsense2.

Note#2: for Node.js GLFW module, add `msbuild` to system PATH, e.g "`C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin`"

Note#3: When running `Node.js` 6.x, you might need to [upgrade npm-bundled `node-gyp`](https://github.com/nodejs/node-gyp/wiki/Updating-npm%27s-bundled-node-gyp) to support Visual Studio 2017 (if you're using it)

## 1.2. Build Native C++ `librealsense` ##

Please refer to [Linux installation doc](../../doc/installation.md) or [Windows installation doc](../../doc/installation_windows.md) or [Mac OS installation doc](../../doc/installation_osx.md) to build native C++ librealsense2.

## 1.3. Build Node.js Module/Addon ##

There are two options to do it: "manually bulid" or "build with CMake".
The former one is for `Node.js` language binding developers who frequently change module/addon source code; the latter one is for all-in-one build scenarios.

### Manually Build

After C++ `librealsense` library is Built, run the following commands:

```
cd wrappers/nodejs
npm install
```
Note: on Windows only, the default libraries are taken from the librealsense Debug build output. You can specify from which build configuration to obtain the sources by installing with the vs_configuration flag. e.g: ``` npm install --vs_configuration=Release ``` or alternatively ``` npm install --vs_configuration=Debug # this is the default for windows ```
### Build with CMake

Before building C++ `librealsense` library, enable the following option when calling `cmake`.
```
cmake -DBUILD_NODEJS_BINDINGS=1 <other options...>
make  # Will build both C++ library & Node.js binding.
```


Note: when doing "Build with CMake" on Windows, `node-gyp` module of 'npm install' command requires one or many of the following Visual Studio 2017 components (if you're using it):
 - `.NET Framework 4.7 development tools`
 - `.NET Framework 4.6.2 development tools`
 - `.NET Framework 4-4.6 development tools`

If it still doens't work, try pass an environment variable to `node-gyp`: `set GYP_MSVS_VERSION=2015`

# 2. Run Examples

When Node.js wrapper is built, you can run examples to see if it works. Plug in your Intel® RealSense™ camera and run the following commands

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

# 4. Contribution
## Coding style guideline
We're following [Chromium coding style](https://chromium.googlesource.com/chromium/src/+/master/styleguide/styleguide.md) for different languages: [C++](https://chromium.googlesource.com/chromium/src/+/master/styleguide/c++/c++.md), [Python](https://google.github.io/styleguide/pyguide.html) and [JavaScript](https://google.github.io/styleguide/javascriptguide.xml).

### Run linter
 1. Install [depot_tools](https://www.chromium.org/developers/how-tos/install-depot-tools) and added to `PATH` env.
 1. Install Required npm modules, `cd src/tools && npm install`.
 1. Run `./tools/linter.js` before submitting your code.

## Commit message guideline
We use same [Chromium commit log guideline](https://www.chromium.org/developers/contributing-code) and [Github closing isses via commit messages](https://help.github.com/articles/closing-issues-via-commit-messages/). Use the following form:

```
Summary of change.

Longer description of change addressing as appropriate: why the change is made,
context if it is part of many changes, description of previous behavior and
newly introduced differences, etc.

Long lines should be wrapped to 80 columns for easier log message viewing in
terminals.

Fixes #123456
```
