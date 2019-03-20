
# Sample Code for Intel® RealSense™ cameras
**Code Examples to start prototyping quickly:** These simple examples demonstrate how to easily use the SDK to include code snippets that access the camera into your applications.  

For mode advanced usages please review the list of [Tools](../tools) we provide.

For a detailed explanations and API documentation see our [Documentation](../doc) section

## List of Examples:

|Name | Language | Description | Experience Level | Product Families |
|---|---|---|---|---|
|[Hello-RealSense](./hello-realsense) | C++ | Demonstrates the basics of connecting to a RealSense device and using depth data | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg)|
|[Distance](./C/distance) | C | Equivalent to `hello-realsense` but rewritten for C users | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg)|
|[Color](./C/color) | C | Demonstrate how to stream color data and prints some frame information | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg)|
|[Capture](./capture)| C++ | Shows how to synchronize and render multiple streams: left, right, depth and RGB streams | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) ![T260](https://img.shields.io/badge/-T260-0e2356.svg) |
|[Save To Disk](./save-to-disk)| C++ | Demonstrate how to render and save video streams on headless systems without graphical user interface (GUI) | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) ![T260](https://img.shields.io/badge/-T260-0e2356.svg) |
|[Pointcloud](./pointcloud)| C++ | Showcase Projection API while generating and rendering 3D pointcloud | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Pose](./pose)|C++|Demonstarates how to obtain data from pose frames| :star: |![T260](https://img.shields.io/badge/-T260-0e2356.svg)|
|[ImShow](../wrappers/opencv/imshow) | C++ & OpenCV | Minimal OpenCV application for visualizing depth data | :star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg)|
|[Depth](./C/depth) | C | Demonstrates how to stream depth data and prints a simple text-based representation of the depth image | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg)|
|[Multicam](./multicam)| C++ | Present multiple cameras depth streams simultaneously, in separate windows | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) ![T260](https://img.shields.io/badge/-T260-0e2356.svg) |
|[Basic Alignment](./align)| C++ | Introduces the concept of spatial stream alignment, using depth-color mapping | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Advanced Alignment](./align-advanced)| C++ | Show a simple method for dynamic background removal from video | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Measure](./measure)| C++ | Lets the user measure the dimensions of 3D objects in a stream | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Post Processing](./post-processing)| C++ | Demonstrating usage of post processing filters for depth images | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Record & Playback](./record-playback)| C++ | Demonstrating usage of the recorder and playback devices | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Motion](./motion)| C++ | Demonstrates how to use data from gyroscope and accelerometer to compute the rotation of the camera | :star::star: | ![Stereo](https://img.shields.io/badge/-D435i-5bc3ff.svg) ![T260](https://img.shields.io/badge/-T260-0e2356.svg) |
|[Pose Prediction](./pose-predict)|C++|Demonstarates how to use tracking camera asynchroniously to implement simple pose prediction | :star::star: |![T260](https://img.shields.io/badge/-T260-0e2356.svg)|
|[DNN](../wrappers/opencv/dnn)| C++ & OpenCV | Intel RealSense camera used for real-time object-detection | :star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Software Device](./software-device)| C++ | Shows how to create a custom `rs2::device` | :star::star::star: | |
|[Sensor Control](./sensor-control)| C++ | A tutorial for using the `rs2::sensor` API | :star::star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) ![T260](https://img.shields.io/badge/-T260-0e2356.svg) |
|[GrabCuts](../wrappers/opencv/grabcuts)| C++ & OpenCV | Simple background removal using the GrabCut algorithm | :star::star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |
|[Latency](../wrappers/opencv/latency-tool)| C++ & OpenCV | Basic latency estimation using computer vision | :star::star::star: | ![Structured Light](https://img.shields.io/badge/-SR300-7f2fbc.svg) ![Stereo](https://img.shields.io/badge/-D400-5bc3ff.svg) ![L500](https://img.shields.io/badge/-L500-ff2845.svg) |

### Community Projects:

1. [OpenCV DNN object detection with RealSense camera](https://github.com/twMr7/rscvdnn)
2. [minimal_realsense2](https://github.com/SirDifferential/minimal_realsense2) - Streaming and Presets in C
3. [ANDREASJAKL.COM](https://www.andreasjakl.com/capturing-3d-point-cloud-intel-realsense-converting-mesh-meshlab/) - Capturing a 3D Point Cloud with Intel RealSense and Converting to a Mesh with MeshLab
4. [FluentRealSense](https://www.codeproject.com/Articles/1233892/FluentRealSense-The-First-Steps-to-a-Simpler-RealS) - The First Steps to a Simpler RealSense
5. [RealSense ROS-bag parser](https://github.com/IntelRealSense/librealsense/issues/2215) - code sample for parsing ROS-bag files by [@marcovs](https://github.com/marcovs)
6. [OpenCV threaded depth cleaner](https://github.com/juniorxsound/ThreadedDepthCleaner) - RealSense depth-map cleaning and inpainting using OpenCV
7. [Sample of how to use the IMU of D435i as well as doing PCL rotations based on this](https://github.com/GruffyPuffy/imutest)
8. [realsense-ir-to-vaapi-h264](https://github.com/bmegli/realsense-ir-to-vaapi-h264) - hardware encode infrared stream to H.264 with Intel VAAPI
9. [EtherSense](https://github.com/krejov100/EtherSense) - Ethernet client and server for RealSense using python's Asyncore
