# Sample Code for Intel® RealSense™ cameras
**Code Examples to start prototyping quickly:** These simple examples demonstrate how to easily use the SDK to include code snippets that access the camera into your applications.  

For mode advanced usages please review the list of [Tools](../tools) we provide.

For a detailed explanations and API documentation see our [Documentation](../doc) section

## List of Samples:
### C++ Examples:
#### Basic
1. [Capture](./capture) - Show how to syncronize and render multiple streams: left & right imagers, depth and RGB streams.
2. [Save To Disk](./save-to-disk) - Demonstrate how to render and save video streams on headless systems without graphical user interface (GUI).
3. [Multicam](./multicam) - Present multiple cameras depth streams simultaneously, in separate windows.
4. [Pointcloud](./pointcloud) - Showcase Projection API while generating and rendering 3D pointcloud.
#### Intermediate
5. [Streams Alignment](./align) - Show a simple method for dynamic background removal from video.
6. [Depth Post Processing](./post-processing) - Demonstrating usage of post processing filters for depth images.
7. [Record and Playback](./record-playback) - Demonstrating usage of the recorder and playback devices.
#### Advanced
8. [Software Device](./software-device) - Shows how to create a custom `rs2::device`.

6. [Sensor Control](./sensor-control) -- A tutorial for using the `rs2::sensor` API
7. [Measure](./measure) - Lets the user measure the dimentions of 3D objects in a stream.

### C Examples:
1. [Depth](./C/depth) - Demonstrates how to stream depth data and prints a simple text-based representation of the depth image.
2. [Distance](./C/distance) - Print the distance from the camera to the object in the center of the image.
3. [Color](./C/color) - Demonstrate how to stream color data and prints some frame information.

### CV Examples:

> See [getting started with OpenCV and RealSense](https://github.com/IntelRealSense/librealsense/tree/master/wrappers/opencv)

1. [ImShow](../wrappers/opencv/imshow) - Minimal OpenCV application for visualizing depth data
2. [GrabCuts](../wrappers/opencv/grabcuts) - Simple background removal using the GrabCut algorithm
3. [Latency-Tool](../wrappers/opencv/latency-tool) - Basic latency estimation using computer vision
3. [DNN](../wrappers/opencv/dnn) - Intel RealSense camera used for real-time object-detection

### Community Projects:

1. [OpenCV DNN object detection with RealSense camera](https://github.com/twMr7/rscvdnn)
2. [minimal_realsense2](https://github.com/SirDifferential/minimal_realsense2) - Streaming and Presets in C
3. [ANDREASJAKL.COM](https://www.andreasjakl.com/capturing-3d-point-cloud-intel-realsense-converting-mesh-meshlab/) - Capturing a 3D Point Cloud with Intel RealSense and Converting to a Mesh with MeshLab
4. [FluentRealSense](https://www.codeproject.com/Articles/1233892/FluentRealSense-The-First-Steps-to-a-Simpler-RealS) - The First Steps to a Simpler RealSense
5. [RealSense ROS-bag parser](https://github.com/IntelRealSense/librealsense/issues/2215) - code sample for parsing ROS-bag files by [@marcovs](https://github.com/marcovs)
6. [OpenCV threaded depth cleaner](https://github.com/juniorxsound/ThreadedDepthCleaner) - RealSense depth-map cleaning and inpainting using OpenCV
7. [Sample of how to use the IMU of D435i as well as doing PCL rotations based on this](https://github.com/GruffyPuffy/imutest)