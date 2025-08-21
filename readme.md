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

Robotics | Depth Sensing | 3D Scanning |
:------------: | :----------: | :-------------: |
<a href="https://www.intelrealsense.com/use-cases/#robotics"><img src="https://media.githubusercontent.com/media/NVIDIA-ISAAC-ROS/.github/main/resources/isaac_ros_docs/repositories_and_packages/isaac_ros_nvblox/realsense_example.gif/" width="240"/></a> |<a href="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/align-expected.gif"><img src="https://raw.githubusercontent.com/wiki/IntelRealSense/librealsense/res/align-expected.gif" width="240"/></a> | <a href="https://www.intelrealsense.com/use-cases/#3d-scanning"><img src="https://media.githubusercontent.com/media/NVIDIA-ISAAC-ROS/.github/main/resources/isaac_ros_docs/repositories_and_packages/isaac_ros_nvblox/realsense_dynamic_example.gif/" width="240"/></a>

Drones | Skeletal and people tracking | Facial authentication |
:--------------------------: | :-----: | :----------------------: |
<a href="https://realsenseai.com/case-studies/"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/GIF/drone-demo.gif?raw=true" width="240"/></a> |<a href="https://realsenseai.com/case-studies/"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/GIF/SkeletalTracking.gif?raw=true" width="240"/></a> | <a href="https://realsenseai.com/case-studies/"><img src="https://librealsense.intel.com/readme-media/face-demo.gif" width="240"/></a>



## Why RealSense?

- **High-resolution color and depth** at close and long ranges
- **Open source SDK** with rich examples and wrappers (Python, ROS, C#, Unity and [more...](https://github.com/IntelRealSense/librealsense/tree/master/wrappers))
- **Active developer community and defacto-standard 3D stereo camera for robotics**
- **Cross-platform** support: Windows, Linux, macOS, Android, and Docker

## Product Line

RealSense stereo depth products use stereo vision to calculate depth, providing high-quality performance in various lighting and environmental conditions.

Here are some examples of the supported models:

| Product | Image | Description |
|---------|-------|-------------|
| [**D555 PoE**](https://realsenseai.com/ruggedized-industrial-stereo-depth/d555-poe/) | <img src="https://realsenseai.com/wp-content/uploads/2025/07/D555.png" width="1000"> | The RealSense™ Depth Camera D555 introduces Power over Ethernet (PoE) interface on chip, expanding our portfolio of USB and GMSL/FAKRA products. |
| [**D457 GMSL/FAKRA**](https://realsenseai.com/ruggedized-industrial-stereo-depth/d457-gmsl-fakra/) | <img src="https://realsenseai.com/wp-content/uploads/2025/07/D457.png" width="1000"> | The RealSense™ Depth Camera D457 is our first GMSL/FAKRA high bandwidth stereo camera. The D457 has an IP65 grade enclosure protecting it from dust ingress and projected water. |
| [**D455**](https://realsenseai.com/stereo-depth-cameras/real-sense-depth-camera-d455/) | <img src="https://www.realsenseai.com/wp-content/uploads/2021/11/455.png" width="1000"> | The D455 is one of the D400 series, designed from feedback and knowledge gained from over 10 years of stereo camera development. |
| [**D435i**](https://realsenseai.com/stereo-depth-cameras/depth-camera-d435i/) | <img src="https://realsenseai.com/wp-content/uploads/2025/07/D435i.png" width="1000"> | RealSense™ depth camera D435i combines the robust depth sensing capabilities of the D435 with the addition of an inertial measurement unit (IMU). |
| [**D405**](https://realsenseai.com/stereo-depth-cameras/stereo-depth-camera-d405/) | <img src="https://realsenseai.com/wp-content/uploads/2025/07/D-405.png" width="1000"> | The RealSense™ Depth Camera D405 is a short-range stereo camera providing sub-millimeter accuracy for your close-range computer vision needs. |


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

### Setup Guides - precompiled SDK
<div align="center" style="margin: 20px 0;">
<a href="./doc/distribution_linux.md"><img src="https://img.shields.io/badge/Ubuntu_Guide-333?style=flat&logo=ubuntu&logoColor=white" style="margin: 5px;" alt="Linux\Jetson Guide"/></a>
<a href="./doc/distribution_windows.md"><img src="https://custom-icon-badges.demolab.com/badge/Windows_Guide-333?logo=windows11&logoColor=white" style="margin: 5px;" alt="Windows Guide"/></a>
</div>

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

### Setup Guides - build from source
<div align="center" style="margin: 20px 0;">
<a href="./doc/installation.md"><img src="https://img.shields.io/badge/Ubuntu_Guide-333?style=flat&logo=ubuntu&logoColor=white" style="margin: 5px;" alt="Linux Guide"/></a>
<a href="./doc/installation_jetson.md"><img src="https://img.shields.io/badge/Jetson_Guide-333?style=flat&logo=nvidia&logoColor=white" style="margin: 5px;" alt="Jetson Guide"/></a>
<a href="./doc/installation_windows.md"><img src="https://custom-icon-badges.demolab.com/badge/Windows_Guide-333?logo=windows11&logoColor=white" style="margin: 5px;" alt="Windows Guide"/></a>
<a href="./doc/installation_osx.md"><img src="https://img.shields.io/badge/macOS_Guide-333?style=flat&logo=apple&logoColor=white" style="margin: 5px;" alt="macOS Guide"/></a>

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
#include <librealsense2/rs.hpp>
#include <iostream>

int main() {
    rs2::pipeline p;                 // Top-level API for streaming & processing frames
    p.start();                       // Configure and start the pipeline

    while (true) {
        rs2::frameset frames = p.wait_for_frames();        // Block until frames arrive
        rs2::depth_frame depth = frames.get_depth_frame(); // Get depth frame
        if (!depth) continue;

        int w = depth.get_width(), h = depth.get_height();
        float dist = depth.get_distance(w/2, h/2);         // Distance to center pixel
        std::cout << "The camera is facing an object " << dist << " meters away\r";
    }
}
```

For more information on the library, please follow our [examples](./examples) or [tools](./tools/), and read the [documentation](./doc) to learn more.

## Supported Platforms

### Operating Systems and Platforms

| Ubuntu | Windows | macOS High Sierra | Jetson | Raspberry Pi |
|--------|---------|-------|--------|----------------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/compiling-librealsense-for-linux-ubuntu-guide"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ubuntu.png?raw=true" width="40%" alt="Ubuntu" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/compiling-librealsense-for-windows-guide"><img src="https://librealsense.intel.com/readme-media/Windows_logo.svg.png" width="40%" alt="Windows" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/macos-installation-for-intel-realsense-sdk"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/apple-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/apple-light.png?raw=true" width="40%" alt="macOS" /></picture></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/nvidia-jetson-tx2-installation"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/nvidia.png?raw=true" width="40%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/using-depth-camera-with-raspberry-pi-3"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/raspberry-pi.png?raw=true" width="40%" alt="" /></a></div> 



### Programming Languages and Wrappers

| C++ | C | C# | Python | ROS 2 |Rest API |
|-----|---|----|--------|-------|---------|
| <div align="center"><a href="https://dev.realsenseai.com/docs/code-samples"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/cpp.png?raw=true" width="50%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/code-samples"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/c.png?raw=true" width="60%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/csharp-wrapper"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/c-sharp.png?raw=true" width="50%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/python2"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/python.png?raw=true" width="30%" alt="" /></a></div> | <div align="center"><a href="https://dev.realsenseai.com/docs/ros2-wrapper"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ros2-dark.png?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/ROS2-light.png?raw=true" width="80%" alt="ROS 2" /></picture></a></div> | <div align="center"><a href="https://github.com/IntelRealSense/librealsense/blob/development/wrappers/rest-api/README.md"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/REST_API.png?raw=true" width="50%" alt="Rest API" /></a></div>|

For more platforms and wrappers look over [here](https://dev.realsenseai.com/docs/docs-get-started).
> Full feature support varies by platform – refer to the [release notes](https://github.com/IntelRealSense/librealsense/wiki/Release-Notes) for details.

## Community & Support

- [📚 Wiki & Docs](https://github.com/IntelRealSense/librealsense/wiki)
- [🐞 Report Issues](https://github.com/IntelRealSense/librealsense/issues)- Found a bug or want to contribute? Read our [contribution guidelines](./CONTRIBUTING.md).

> 🔎 Looking for legacy devices (F200, R200, LR200, ZR300)? Visit the [legacy release](https://github.com/IntelRealSense/librealsense/tree/v1.12.1).

---
<p align="center">
You can find us at
</p>
<p align="center">
  <a href="https://github.com/realsenseai" target="_blank" aria-label="GitHub"><picture><source media="(prefers-color-scheme: dark)" srcset="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/github_light.PNG?raw=true"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/github.png?raw=true" width="32" alt="GitHub"></picture></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://x.com/RealSenseai" target="_blank" aria-label="X (Twitter)"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/twitter.png?raw=true" width="32" alt="X (Twitter)" /></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.youtube.com/@RealSenseai" target="_blank" aria-label="YouTube"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/social.png?raw=true" width="32" alt="YouTube" /></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.linkedin.com/company/realsenseai?trk=similar-pages" target="_blank" aria-label="LinkedIn"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/linkedin.png?raw=true" width="32" alt="LinkedIn" /></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://realsenseai.com/" target="_blank" aria-label="Community"><img src="https://github.com/Noy-Zini/librealsense/blob/media-files/doc/img/logos/Real-sense-badge-rgb-c.png?raw=true" width="32" alt="Community" /></a>
</p>



