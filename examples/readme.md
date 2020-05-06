
# Sample Code for Intel® RealSense™ cameras
**Code Examples to start prototyping quickly:** These simple examples demonstrate how to easily use the SDK to include code snippets that access the camera into your applications.  

For mode advanced usages please review the list of [Tools](../tools) we provide.

For a detailed explanations and API documentation see our [Documentation](../doc) section

## List of Examples:

|Name | Language | Description | Experience Level | Technology |
|---|---|---|---|---|
|[Hello-RealSense](./hello-realsense)| C++ | Demonstrates the basics of connecting to a RealSense device and using depth data | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Distance](./C/distance) | C | Equivalent to `hello-realsense` but rewritten for C users | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Color](./C/color) | C | Demonstrate how to stream color data and prints some frame information | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Capture](./capture)| C++ | Shows how to synchronize and render multiple streams: left, right, depth and RGB streams | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[Save To Disk](./save-to-disk)| C++ | Demonstrate how to render and save video streams on headless systems without graphical user interface (GUI) | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools) |
|[Pointcloud](./pointcloud)| C++ | Showcase Projection API while generating and rendering 3D pointcloud | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Pose](./pose)|C++|Demonstrates how to obtain data from pose frames| :star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[ImShow](../wrappers/opencv/imshow) | C++ & [OpenCV](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/opencv#getting-started) | Minimal OpenCV application for visualizing depth data | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Multicam](./multicam)| C++ | Present multiple cameras depth streams simultaneously, in separate windows | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools) |
|[Face](../wrappers/dlib/face)| C++ & [Dlib](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/dlib) | Facial recognition with simple anti-spoofing | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)  |
|[Face](../wrappers/openvino/face)| C++ & [OpenVINO](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/openvino) | Facial recognition with OpenVINO™ | :star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)  |
|[Depth](./C/depth) | C | Demonstrates how to stream depth data and prints a simple text-based representation of the depth image | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)|
|[Spatial Alignment](./align)| C++ | Introduces the concept of spatial stream alignment, using depth-color mapping | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Advanced Alignment](./align-advanced)| C++ | Show a simple method for dynamic background removal from video | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Measure](./measure)| C++ | Lets the user measure the dimensions of 3D objects in a stream | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Post Processing](./post-processing)| C++ | Demonstrating usage of post processing filters for depth images | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Record & Playback](./record-playback)| C++ | Demonstrating usage of the recorder and playback devices | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Motion](./motion)| C++ | Demonstrates how to use data from gyroscope and accelerometer to compute the rotation of the camera | :star::star: | [![Depth with IMU](https://img.shields.io/badge/-D435i-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools) |
|[Pose Prediction](./pose-predict)|C++|Demonstrates how to use tracking camera asynchroniously to implement simple pose prediction using librealsense global time | :star::star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[Pose and Image](./pose-and-image)|C++|Demonstrates how to use tracking camera asynchroniously to obtain 200Hz poses and 30Hz images | :star::star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[Apriltag Pose](./pose-apriltag)|C++|Demonstrates how to compute [Apriltag](https://github.com/AprilRobotics/apriltag/tree/3.1.1) pose from T265 fisheye image stream. | :star::star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[AR-Basic](./ar-basic)|C++|Shows how to use pose and fisheye frames to display a simple virtual object on the fisheye image | :star::star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[AR-Advanced](./ar-advanced)|C++|Shows how to extend the [AR-Basic](./ar-basic) example with the relocalization API | :star::star::star: |[![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)|
|[DNN](../wrappers/opencv/dnn)| C++ & [OpenCV](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/opencv#getting-started) | Intel RealSense camera used for real-time object-detection | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[DNN](../wrappers/openvino/dnn)| C++ & [OpenVINO](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/openvino) | Intel RealSense camera used for real-time object-detection | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Tracking and Depth](./tracking-and-depth)| C++ | Shows how to use the tracking camera T265 together with a depth camera to display a 3D pointcloud with respect to a static reference frame | :star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)
|[Trajectory](./trajectory)| C++ | Shows how to calculate and render 3D trajectory based on pose data from a tracking camera | :star::star::star: | [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools)
|[Software Device](./software-device)| C++ | Shows how to create a custom `rs2::device` | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools) |
|[GL](./gl)| C++ | Shows how to perform parts of frame processing using the GPU | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) |
|[Sensor Control](./sensor-control)| C++ | A tutorial for using the `rs2::sensor` API | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md) [![Motion Tracking - T260 and SLAM](https://img.shields.io/badge/-Tracking-0e2356.svg)](../doc/t265.md#examples-and-tools) |
|[GrabCuts](../wrappers/opencv/grabcuts)| C++ & [OpenCV](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/opencv#getting-started) | Simple background removal using the GrabCut algorithm | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)  |
|[Latency](../wrappers/opencv/latency-tool)| C++ & [OpenCV](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/opencv#getting-started) | Basic latency estimation using computer vision | :star::star::star: | [![Depth Sensing - Structured Light, Stereo and L500](https://img.shields.io/badge/-Depth-5bc3ff.svg)](./depth.md)  |

### Community Projects:

1. [OpenCV DNN object detection with RealSense camera](https://github.com/twMr7/rscvdnn)
2. [minimal_realsense2](https://github.com/SirDifferential/minimal_realsense2) - Streaming and Presets in C
3. [ANDREASJAKL.COM](https://www.andreasjakl.com/capturing-3d-point-cloud-intel-realsense-converting-mesh-meshlab/) - Capturing a 3D Point Cloud with Intel RealSense and Converting to a Mesh with MeshLab
4. [FluentRealSense](https://www.codeproject.com/Articles/1233892/FluentRealSense-The-First-Steps-to-a-Simpler-RealS) - The First Steps to a Simpler RealSense
5. [RealSense ROS-bag parser](https://github.com/IntelRealSense/librealsense/issues/2215) - code sample for parsing ROS-bag files by [@marcovs](https://github.com/marcovs)
6. [OpenCV threaded depth cleaner](https://github.com/juniorxsound/ThreadedDepthCleaner) - RealSense depth-map cleaning and inpainting using OpenCV
7. [Sample of how to use the IMU of D435i as well as doing PCL rotations based on this](https://github.com/GruffyPuffy/imutest)
8. [realsense-ir-to-vaapi-h264](https://github.com/bmegli/realsense-ir-to-vaapi-h264) - hardware encode infrared stream to H.264 with Intel VAAPI
9. [realsense-depth-to-vaapi-hevc10](https://github.com/bmegli/realsense-depth-to-vaapi-hevc10) - hardware encode depth stream to 10 bit HEVC with Intel VAAPI
10. [EtherSense](https://github.com/krejov100/EtherSense) - Ethernet client and server for RealSense using python's Asyncore
11. [Unofficial OpenVino example + D400](https://github.com/gbr1/ros_openvino) - example of using OpenVino with RealSense and ROS for object detection
12. [keijiro/Rsvfx](https://github.com/keijiro/Rsvfx) - An example that shows how to connect RealSense depth camera to Unity VFX Graph
13. [kougaku/RealSenseOSC](https://github.com/kougaku/RealSenseOSC) - Client-Server project with Processing example for T265
14. [Vimeo Depth Viewer](https://github.com/vimeo/vimeo-depth-viewer) - A RealSense depth viewer using [nanogui](https://github.com/wjakob/nanogui)
15. [PINTO0309/MobileNet-SSD-RealSense](https://github.com/PINTO0309/MobileNet-SSD-RealSense) - RaspberryPi3 or Ubuntu + Multi Neural Compute Stick(NCS/NCS2) + RealSense D435 + MobileNet-SSD(MobileNetSSD) + Background Multi-transparent(Simple multi-class segmentation) + FaceDetection + MultiGraph + MultiProcessing + MultiClustering
16. [realsenseOnCyber](https://github.com/mickeyouyou/realsenseOnCyber) Realsense module Based on Apollo Cyber RT
17. [Raspberry Pi Handheld 3D Scanner](https://eleccelerator.com/pi-handheld-3d-scanner/) by [Frank Zhao](https://github.com/frank26080115)
18. [Erwhi Hedgehog](https://gbr1.github.io/erwhi_hedgehog.html) by [Giovanni Bruno](https://github.com/gbr1) - Open-Source Hardware and Software design featuring Intel RealSense, UP-Squared and Intel Movidius technologies
