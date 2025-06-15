<p align="center">
<!-- Light mode -->
<img src="doc/img/realsense-logo-light-mode.png#gh-light-mode-only" alt="Logo for light mode" width="70%"/>

<!-- Dark mode -->
<img src="doc/img/realsense-logo-dark-mode.png#gh-dark-mode-only" alt="Logo for dark mode" width="70%"/>
<br><br>
</p>

<p align="center">RealSense SDK 2.0 is a cross-platform library for RealSense depth cameras.
The SDK allows depth and color streaming, and provides intrinsic and extrinsic calibration information.</p>


<p align="center">
  <a href="https://www.apache.org/licenses/LICENSE-2.0"><img src="https://img.shields.io/github/license/IntelRealSense/librealsense.svg" alt="License"></a>
  <a href="https://github.com/IntelRealSense/librealsense/releases/latest"><img src="https://img.shields.io/github/v/release/IntelRealSense/librealsense?sort=semver" alt="Latest release"></a>
  <a href="https://github.com/IntelRealSense/librealsense/compare/master...development"><img src="https://img.shields.io/github/commits-since/IntelRealSense/librealsense/master/development?label=commits%20since" alt="Commits since"></a>
  <a href="https://github.com/IntelRealSense/librealsense/issues"><img src="https://img.shields.io/github/issues/IntelRealSense/librealsense.svg" alt="Issues"></a>
  <a href="https://github.com/IntelRealSense/librealsense/actions/workflows/buildsCI.yaml?query=branch%3Adevelopment"><img src="https://github.com/IntelRealSense/librealsense/actions/workflows/buildsCI.yaml/badge.svg?branch=development" alt="GitHub CI"></a>
  <a href="https://github.com/IntelRealSense/librealsense/network/members"><img src="https://img.shields.io/github/forks/IntelRealSense/librealsense.svg" alt="Forks"></a>
</p>

## Use Cases

Below are some of the many real-world applications powered by RealSense technology:

Depth Sensing | Robotics | 3D Scanning |
:------------: | :----------: | :-------------: |
<a href="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/align-expected.gif"><img src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/align-expected.gif" width="240"/></a> | <a href="https://www.intelrealsense.com/use-cases/#robotics"><img src="https://media.githubusercontent.com/media/NVIDIA-ISAAC-ROS/.github/main/resources/isaac_ros_docs/repositories_and_packages/isaac_ros_nvblox/realsense_example.gif/" width="240"/></a> | <a href="https://www.intelrealsense.com/use-cases/#3d-scanning"><img src="https://media.githubusercontent.com/media/NVIDIA-ISAAC-ROS/.github/main/resources/isaac_ros_docs/repositories_and_packages/isaac_ros_nvblox/realsense_dynamic_example.gif/" width="240"/></a>

Skeletal and people tracking | Drones | Facial authentication |
:--------------------------: | :-----: | :----------------------: |
<a href="https://realsenseai.com/case-studies/"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/GIF/SkeletalTracking.gif?raw=true" width="240"/></a> | <a href="https://realsenseai.com/case-studies/"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/GIF/drone-demo.gif?raw=true" width="240"/></a> | <a href="https://realsenseai.com/case-studies/"><img src="https://librealsense.intel.com/readme-media/face-demo.gif" width="240"/></a>



## Why RealSense?

- **High-resolution depth** at close and long ranges
- **Open source SDK** with rich examples and wrappers (Python, ROS, C#, Unity and [more...](https://github.com/IntelRealSense/librealsense/tree/master/wrappers))
- **Active developer community**
- **Cross-platform** support: Windows, Linux, macOS, Android, and Docker

## Product Line

RealSense stereo depth products use stereo vision to calculate depth, providing high-quality performance in various lighting and environmental conditions.

Here are some examples of the supported models:

| Product | Image | Description |
|---------|-------|-------------|
| [**D455**](https://realsenseai.com/stereo-depth-cameras/real-sense-depth-camera-d455/) | <img src="https://www.realsenseai.com/wp-content/uploads/2021/11/455.png" width="1000"> | The D455 is one of the D400 series, designed from feedback and knowledge gained from over 10 years of stereo camera development. |
| [**D435f**](https://realsenseai.com/stereo-depth-with-ir-pass-filter/d435f/) | <img src="https://realsenseai.com/wp-content/uploads/2025/07/D435if-a.png" width="1000"> | The RealSense Depth Camera D435f expands our portfolio targeting the growing market of autonomous mobile robots. The D435f utilizes an IR pass filter to enhance depth noise quality and performance range in many robotic environments. |
| [**D555 PoE**](https://realsenseai.com/ruggedized-industrial-stereo-depth/d555-poe/) | <img src="https://realsenseai.com/wp-content/uploads/2025/07/D555.png" width="1000"> | The RealSense™ Depth Camera D555 introduces Power over Ethernet (PoE) interface on chip, expanding our portfolio of USB and GMSL/FAKRA products. |

> 🛍️ [Explore more stereo products](https://store.realsenseai.com/)

## Getting Started

Start developing with RealSense in minutes using either method below.

### 1️. Precompiled SDK

This is the best option if you want to plug in your camera and get started right away.
1. Download the latest SDK bundle from the [Releases page](https://github.com/IntelRealSense/librealsense/releases).
2. Connect your RealSense camera.
3. Run the included tools:
    - RealSense Viewer: View streams, tune settings, record and playback.
    - Depth Quality Tool: Measure accuracy and fill rate.

### 2️. Build from Source
For a more custom installation, follow these steps to build the SDK from source.
1. Clone the repository and create a build directory:
   ```bash
   git clone https://github.com/IntelRealSense/librealsense.git
   mkdir build && cd build
   ```
2. Run CMake to configure the build:
    ```bash
    cmake .. -DBUILD_EXAMPLES=true
    ```
3. Build the project:
    ```bash
    make -j$(nproc)
    ```

### Setup Guides
<div align="center" style="margin: 20px 0;">
<a href="./doc/distribution_linux.md"><img src="https://img.shields.io/badge/Ubuntu_Guide-333?style=flat&logo=ubuntu&logoColor=white" style="margin: 5px;" alt="Ubuntu Guide"/></a>
<a href="./doc/distribution_windows.md"><img src="https://custom-icon-badges.demolab.com/badge/Windows_Guide-333?logo=windows11&logoColor=white" style="margin: 5px;" alt="Windows Guide"/></a>
<a href="./doc/installation_osx.md"><img src="https://img.shields.io/badge/macOS_Guide-333?style=flat&logo=apple&logoColor=white" style="margin: 5px;" alt="macOS Guide"/></a>
<a href="./doc/android.md"><img src="https://img.shields.io/badge/Android_Guide-333?style=flat&logo=android&logoColor=white" style="margin: 5px;" alt="Android Guide"/></a>
<a href="./scripts/Docker/readme.md"><img src="https://img.shields.io/badge/Docker_Guide-333?style=flat&logo=docker&logoColor=white" style="margin: 5px;" alt="Docker Guide"/></a>
</div>


## Python Packages
[![pyrealsense2](https://img.shields.io/pypi/v/pyrealsense2.svg?label=pyrealsense2&logo=pypi)](https://pypi.org/project/pyrealsense2/)
[![PyPI - pyrealsense2-beta](https://img.shields.io/pypi/v/pyrealsense2-beta.svg?label=pyrealsense2-beta&logo=pypi)](https://pypi.org/project/pyrealsense2-beta/)

**Which should I use?**
- **Stable:** `pyrealsense2` — validated releases aligned with SDK tags (Recommended for most users).  
- **Beta:** `pyrealsense2-beta` — fresher builds for early access and testing. Expect faster updates.  

> Both packages import as `pyrealsense2`. Install **only one** at a time.

### Install
```bash
pip install pyrealsense2 # Stable
pip install pyrealsense2-beta # Beta
```

## Ready to Hack!

Our library offers a high level API for using RealSense depth cameras (in addition to lower level ones).
The following snippet shows how to start streaming frames and extracting the depth value of a pixel:

```cpp

rs2::pipeline p; // Create a Pipeline - this serves as a top-level API for streaming and processing frames
p.start();       // Configure and start the pipeline

while (true)
{
    rs2::frameset frames = p.wait_for_frames();        							// Block program until frames arrive
    rs2::depth_frame depth = frames.get_depth_frame(); 							// Try to get a frame of a depth image
    float dist = depth.get_distance(depth.get_width()/2, depth.get_height()/2); // Query the distance from the camera to the object in the center of the image
    std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";
}
```
For more information on the library, please follow our [examples](./examples), and read the [documentation](./doc) to learn more.

## Supported Platforms

### Operating Systems

| Ubuntu | Windows | Android | macOS |
|--------|---------|---------|-------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/compiling-librealsense-for-linux-ubuntu-guide"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ubuntu.png?raw=true" width="40%" alt="Ubuntu" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/compiling-librealsense-for-windows-guide"><img src="https://librealsense.intel.com/readme-media/Windows_logo.svg.png" width="40%" alt="Windows" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/android-build-of-the-intel-realsense-sdk-20"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/android.png?raw=true" width="40%" alt="Android" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/macos-installation-for-intel-realsense-sdk"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/apple-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/apple-light.png?raw=true" width="40%" alt="macOS" /></picture></a></div> |

### Platforms

| Jetson | Raspberry Pi 3 | Rockchip |
|--------|----------------|----------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/nvidia-jetson-tx2-installation"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/nvidia.png?raw=true" width="30%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/using-depth-camera-with-raspberry-pi-3"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/raspberry-pi.png?raw=true" width="25%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/firefly-rk3399-installation"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/rockchip.png?raw=true" width="60%" alt="" /></a></div>

### Programming Languages

| C++ | C | C# | Python | Matlab |
|-----|---|----|--------|--------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/code-samples"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/cpp.png?raw=true" width="50%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/code-samples"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/c.png?raw=true" width="60%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/csharp-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/c-sharp.png?raw=true" width="50%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/python2"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/python.png?raw=true" width="30%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/matlab-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/matlab.png?raw=true" width="40%" alt="" /></a></div>

### Frameworks and Wrappers

| Unity | Unreal Engine | OpenCV | ROS | ROS 2 |
|-------|---------------|--------|-----|-------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/unity-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/unity.png?raw=true" width="30%" alt="Unity" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/unrealengine-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/unreal-engine.png?raw=true" width="35%" alt="Unreal Engine" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/opencv-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/OpenCV_logo.png?raw=true" width="30%" alt="OpenCV" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/ros-wrapper"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ros-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ros-light.png?raw=true" width="60%" alt="ROS" /></picture></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/ros2-wrapper"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ros2-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ROS2-light.png?raw=true" width="40%" alt="ROS 2" /></picture></a></div> |

| PCL | OpenVINO | Open3D | LabVIEW |
|-----|----------|--------|---------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/pcl-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/pcl_logo.png?raw=true" width="30%" alt="PCL" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/openvino"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/openvino-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/openvino-light.png?raw=true" width="25%" alt="OpenVINO" /></picture></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/open3d"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/OPEN3D.png?raw=true" width="30%" alt="Open3D" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/labview-wrapper"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/labview-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/labview-light.png?raw=true" width="40%" alt="LabVIEW" /></picture></a></div> |


> Full feature support varies by platform – refer to the [release notes](https://github.com/IntelRealSense/librealsense/wiki/Release-Notes) for details.

## Community & Support

- [📚 Wiki & Docs](https://github.com/IntelRealSense/librealsense/wiki)
- [🐞 Report Issues](https://github.com/IntelRealSense/librealsense/issues)- Found a bug or want to contribute? Read our [contribution guidelines](./CONTRIBUTING.md).

> 🔎 Looking for legacy devices (F200, R200, LR200, ZR300)? Visit the [legacy release](https://github.com/IntelRealSense/librealsense/tree/v1.12.1).


<p align="center">
  <a href="https://github.com/IntelRealSense/librealsense/tree/master" target="_blank" aria-label="GitHub"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/github_light.PNG?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/github.png?raw=true" width="32" alt="GitHub"></picture></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://x.com/RealSenseai" target="_blank" aria-label="X (Twitter)"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/twitter.png?raw=true" width="32" alt="X (Twitter)" /></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.youtube.com/c/IntelRealSense" target="_blank" aria-label="YouTube"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/social.png?raw=true" width="32" alt="YouTube" /></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.linkedin.com/company/realsenseai?trk=similar-pages" target="_blank" aria-label="LinkedIn"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/linkedin.png?raw=true" width="32" alt="LinkedIn" /></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://realsenseai.com/" target="_blank" aria-label="Community"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/Real-sense-badge-rgb-c.png?raw=true" width="32" alt="Community" /></a>
</p>



