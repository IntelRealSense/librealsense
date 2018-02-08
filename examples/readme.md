# Sample Code for Intel® RealSense™ cameras
**Code Examples to start prototyping quickly:** These simple examples demonstrate how to easily use the SDK to include code snippets that access the camera into your applications.  

For mode advanced usages please review the list of [Tools](../tools) we provide.

For a detailed explanations and API documentation see our [Documentation](../doc) section

## List of Samples:
### C++ Examples
1. [Capture](./capture) - Show how to syncronize and render multiple streams: left & right imagers, depth and RGB streams.
2. [Save To Disk](./save-to-disk) - Demonstrate how to render and save video streams on headless systems without graphical user interface (GUI).
3. [Multicam](./multicam) - Present multiple cameras depth streams simultaneously, in separate windows.
4. [Pointcloud](./pointcloud) - Showcase Projection API while generating and rendering 3D pointcloud.
5. [Streams Alignment](./align) - Show a simple method for dynamic background removal from video.
6. [Sensor Control](./sensor-control) -- A tutorial for using the `rs2::sensor` API
7. [Measure](./measure) - Lets the user measure the dimentions of 3D objects in a stream.
8. [Software Device](./software-device) - Shows how to create a custom `rs2::device`.
9. [Depth Post Processing](./post-processing) - Demonstrating usage of post processing filters for depth images.

### C Examples
1. [Depth](./C/depth) - Demonstrates how to stream depth data and prints a simple text-based representation of the depth image.
2. [Distance](./C/distance) - Print the distance from the camera to the object in the center of the image.
3. [Color](./C/color) - Demonstrate how to stream color data and prints some frame information.
