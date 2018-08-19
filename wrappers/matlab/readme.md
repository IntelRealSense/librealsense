# Getting Started with RealSense™ SDK2.0 for Matlab®

## Introduction:
**NOTE: This is an early version. Much of the RealSense SDK2.0 is exposed in Matlab, but it's still a work in progess.**<br>
The latest generation of RealSense Depth Cameras can be controlled via the RealSense SDK2.0, also referred to as libRealSense 2.x.x, or LibRS for short. This includes stereo depth cameras D415 and D435 as well as their corresponding modules, such as D410 and D430.

To get started controlling the RealSense Cameras with Matlab® in Windows 10, we have created a package that wraps some of the core functions of the realsense2.dll, and we have also created a simple “Hello World” function to get started capturing depth data. This uses Matlab  R2017a and requires a Windows 10 laptop with a USB3 port.

## Getting Started:
1. Download the Git repository and run CMake, specifying the x64 toolchain.
2. Open Visual Studio and import the project file located [here](./librealsense_mex.vcxproj).
3. Make sure that the project settings are consistent with the instructions listed on the [MathWorks website](https://www.mathworks.com/help/matlab/matlab_external/compiling-mex-files-with-the-microsoft-visual-c-ide.html) and your system.
4. After compiling the project, set [build/Debug](../../build/Debug) or [build/Release](../../build/Release) as your Matlab working directory.
5. Our simple example can be run by typing `realsense.depth_example` at the Matlab prompt.
