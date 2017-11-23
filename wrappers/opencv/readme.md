# OpenCV Samples for Intel® RealSense™ cameras
**Code Examples to start prototyping quickly:** These simple examples demonstrate how to easily use the SDK to include code snippets that access the camera into your applications.  

For a detailed explanations and API documentation see our [Documentation](../doc) section 
 
## List of Samples:
1. [ImShow](./imshow) - Minimal OpenCV application for visualizing depth data

## Getting Started:
This page is certainly **not** a comprehensive guide to getting started with OpenCV and CMake, but it can help get on the right track. 

### Windows
1. Download and install `CMake` from [cmake.org/download](https://cmake.org/download/)
2. Clone or download OpenCV sources from [github.com/opencv/opencv](https://github.com/opencv/opencv) into a local directory (`C:/git/opencv-master`)
3. Run `cmake-gui`, input source code and binaries locations: 

<p align="center"><img src="res/1.PNG" /></p>

4. Click `Configure`
> When working behind a firewall, you might want to consider unchecking `WITH_FFMPEG` and `WITH_IPP` to avoid additional downloads
5. Uncheck `BUILD_SHARED_LIBS`: 

<p align="center"><img src="res/2.PNG" /></p>

6. Click `Generate`
7. Click `Open Project` to open Visual Studio
8. Press `Ctrl+Shift+B` to build solution
9. Clone or download librealsense sources from [github.com/IntelRealSense/librealsense](https://github.com/IntelRealSense/librealsense) into a local directory (`C:/git/librealsense`)
10. Run `cmake-gui` and fill source code and binaries locations and press `Configure`
11. Make sure you check the `BUILD_CV_EXAMPLES` flag and click `Configure` again:

<p align="center"><img src="res/3.PNG" /></p>

12. Specify CMake binaries folder for OpenCV as `OpenCV_DIR` (`c:/git/opencv-master`)

<p align="center"><img src="res/4.PNG" /></p>

13. Click `Generate` and `Open Project`
14. Locate CV solution-folder under Examples

<img src="res/5.PNG" />

15. Right-click on one of the examples to `Set as StartUp Project`
16. Press `F5` to compile and run the example
