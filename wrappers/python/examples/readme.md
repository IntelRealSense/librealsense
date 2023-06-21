# Sample Code for Intel® RealSense™ Python Wrapper

These Examples demonstrate how to use the python wrapper of the SDK.

## List of Examples:

1. [Tutorial 1](./python-tutorial-1-depth.py) - Demonstrates how to start streaming depth frames from the camera and display the image in the console as an ASCII art.
2. [NumPy and OpenCV](./opencv_viewer_example.py) - Example of rendering depth and color images using the help of OpenCV and Numpy
3. [Stream Alignment](./align-depth2color.py) - Demonstrate a way of performing background removal by aligning depth images to color images and performing simple calculation to strip the background.
4. [RS400 Advanced Mode](./python-rs400-advanced-mode-example.py) - Example of the advanced mode interface for controlling different options of the D400 cameras
5. [Realsense Backend](./pybackend_example_1_general.py) - Example of controlling devices using the backend interface
6. [Read bag file](./read_bag_example.py) - Example on how to read bag file and use colorizer to show recorded depth stream in jet colormap.
7. [Box Dimensioner Multicam](./box_dimensioner_multicam/box_dimensioner_multicam_demo.py) - Simple demonstration for calculating the length, width and height of an object using multiple cameras.
8. [T265 Basic](./t265_example.py) - Demonstrates how to retrieve pose data from a T265
9. [T265 Coordinates](./t265_rpy.py) - This example shows how to change coordinate systems of a T265 pose
10. [T265 Stereo](./t265_stereo.py) - This example shows how to use T265 intrinsics and extrinsics in OpenCV to asynchronously compute depth maps from T265 fisheye images on the host.
11. [Realsense over Ethernet](./ethernet_client_server/README.md) - This example shows how to stream depth data from RealSense depth cameras over ethernet.
12. [Realsense Network Device Viewer](./net_viewer.py) - Shows how to connect to rs-server over network.

## Pointcloud Visualization

1. [OpenCV software renderer](https://github.com/IntelRealSense/librealsense/blob/development/wrappers/python/examples/opencv_pointcloud_viewer.py)
2. [PyGlet pointcloud renderer](https://github.com/IntelRealSense/librealsense/blob/development/wrappers/python/examples/pyglet_pointcloud_viewer.py) - requires `pip install pyglet`

## Interactive Examples:

1. [Distance to Object](https://github.com/IntelRealSense/librealsense/blob/jupyter/notebooks/distance_to_object.ipynb) [![Binder](https://mybinder.org/badge.svg)](https://mybinder.org/v2/gh/IntelRealSense/librealsense/jupyter?filepath=notebooks/distance_to_object.ipynb)
2. [Depth Filters](https://github.com/IntelRealSense/librealsense/blob/jupyter/notebooks/depth_filters.ipynb) [![Binder](https://mybinder.org/badge.svg)](https://mybinder.org/v2/gh/IntelRealSense/librealsense/jupyter?filepath=notebooks/depth_filters.ipynb)
