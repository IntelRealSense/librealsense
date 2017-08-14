# Intel® RealSense™ Viewer

<p align="center"><img src="res/realsense-viewer-backup.gif" /></p>

## Overview

RealSense Viewer is the flagship tool providing access to most camera functionality through simple, cross-platform UI. 
The tool offers:
* Streaming from multiple RealSense devices at the same time
* Exploring pointcloud data in realtime or by exporting to file
* Recording RealSense data as well as playback of recorded files
* Access to most camera specific controls, including 3D-generation ASIC registers when available

## Implementation Notes

You can get RealSense Viewer in form of a binary package on Windows and Linux, or build it from source alongside the rest of the library. The viewer is designed to be lightweight, requiring only a handful of embeded dependencies. Cross-platform UI is a combination of raw OpenGL calls, [GLFW](http://www.glfw.org/) for cross-platform window and event management, and [IMGUI](https://github.com/ocornut/imgui) for the interface elements. Please see [COPYING](../../COPYING) for full list of attributions. 
