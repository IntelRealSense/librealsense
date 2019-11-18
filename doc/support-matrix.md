# Software Support Matrix

## Release Process

RealSense devices pass validation and stability cycles. Every firmware + librealsense release is going through several levels of validation, from automated unit-tests to longer validation cycles.  

After passing unit-tests and basic functional testing release is made available under [librealsense/releases](https://github.com/IntelRealSense/librealsense/releases) and marked as a ![pre-release](./img/prerelease.png).

Once full validation report is internally reviewed and approved, release is promoted to ![latest](./img/latest_release.png) and published at [www.intelrealsense.com/developers/](https://www.intelrealsense.com/developers/).

We recommend certain combinations (see **RECOMMENDED CONFIGURATIONS** at the link above) of firmware and software we have tested, but there is significant effort to keep maximum backward and forward compatibility between firmware and software versions.

Validation is done on x86 NUC machines running Windows 10 and Ubuntu 16.04 LTS.

Additional platforms are tested and supported on demand, driven by community requests and feedback.

## D400 Stereoscopic Depth Cameras

|                                | Win 7                  | Win 10 & 8.1                      | Ubuntu 16/18 LTS x64                       | Ubuntu Arm (Raspberry Pi, RK3399…) | NVidia Jetson Series | Android (7+) & JAVA | Mac OS            | Viewer         | C/C++       | Python      | C#  | ROS                | Node.js             | LabView        | Unity  | Matlab  | OpenNi2| Unreal|
|--------------------------------|------------------------|-----------------------------------|--------------------------------------------|-----------------------------|-------------------------|-------------------------|-------------------|----------------|-------------|-------------|-----|--------------------|---------------------|----------------|--------|---------|--------|------|
| **Precompiled Binaries**       | Yes                    | Yes                               | Yes                                        | No                          | Yes (2.24+)             | Yes                     | No                | Yes            | Yes         | Yes         | Yes | Yes                | Yes (outdated 2.16) | Yes            | Yes    | Yes     | No     | No   |
| **Depth Data Access**          | Yes                    | Yes                               | Yes                                        | Yes (Kernel 4.4+ or LIBUVC) | Yes                     | Yes                     | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **Infrared Streaming**         | Yes                    | Yes                               | Yes                                        | Yes (Kernel 4.4+ or LIBUVC) | Yes (via LibUVC)        | Yes                     | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **Advanced Mode API**          | Yes                    | Yes                               | Yes                                        | Yes                         | Yes                     | No                      | Yes               | Yes            | Yes         | Yes         | No  | No                 | No                  | No             | No     | No      | No     | No   |
| **Advanced Mode JSON Loading** | Yes                    | Yes                               | Yes                                        | Yes                         | Yes                     | Yes                     | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | No     | No      | No     | No   |
| **Preset Control**             | Yes                    | Yes                               | Yes                                        | Yes                         | Yes                     | Yes                     | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **Per-frame Frame Metadata**   | Yes                    | Yes (requires [script](./installation_windows.md#use-automation-script))| Yes (via DKMS or patch)| Yes (via LIBUVC) | Yes              | Yes                     | Yes               | Yes (2.26+)    | Yes         | Yes         | Yes | N/A                | Yes                 | N/A            | No     | Yes     | No     | No   |
| **Depth Post-Processing**      | Yes                    | Yes                               | Yes                                        | Unoptimised                 | Unoptimised             | Unoptimised             | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | No                  | N/A            | Yes    | N/A     | No     | No   |
| **Pointcloud Calculation**     | Yes                    | Yes                               | Yes                                        | Unoptimised                 | Yes (2.13+ with CUDA)   | Unoptimised             | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | N/A            | Yes    | Yes     | N/A    | N/A  |
| **Stream Alignment**           | Yes                    | Yes                               | Yes                                        | Unoptimised                 | Yes (2.13+ with CUDA)   | Yes                     | Yes               | No             | Yes         | Yes         | Yes | Yes                | Yes                 | N/A            | Yes    | N/A     | N/A    | N/A  |
| **RGB Streaming**              | Yes                    | Yes                               | Yes                                        | Unoptimised                 | Yes (2.13+ with CUDA)   | Unoptimised             | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **MPEG Decoding**              | Unoptimised            | Unoptimised                       | Unoptimised                                | No                          | No                      | No                      | No                | Yes (2.26.1?+) | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **Accelerometer & Gyro**       | No                     | Yes                               | Yes                                        | Yes (via LIBUVC, 2.26+)     | Yes (via LIBUVC, 2.26+) | Yes (via LIBUVC, 2.26+) | No (WIP)          | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | N/A            | N/A    | Yes     | No     | No   |
| **IMU Hardware Timestamps**    | No                     | Yes                               | Yes (via DKMS or patch)                    | Yes (via LIBUVC, 2.26+)     | Yes (via LIBUVC, 2.26+) | Yes (via LIBUVC, 2.26+) | No (WIP)          | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | N/A            | N/A    | Yes     | No     | No   |
| **Hardware Timestamps**        | Yes                    | Yes (requires script or DMFT INF) | Yes (via DKMS or patch or Kernel >= 4.18)  | Yes (via LIBUVC)            | Yes (via LibUVC)        | Yes                     | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **Global Timestamps**          | Yes                    | Yes (requires script or DMFT INF) | Yes (via DKMS or patch or Kernel >= 4.18)  | Yes (via LIBUVC)            | Yes (via LIBUVC)        | Yes                     | Yes               | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | Yes            | Yes    | Yes     | Yes    | Yes  |
| **On-Chip Calibration**           | Yes (2.26+)            | Yes (2.26+)                       | Yes (2.26+)                                | Yes (2.26+)                 | Yes (2.26+)             | No                      | Yes (2.26+)       | Yes (2.26+)    | No          | No          | No  | No                 | No                  | No             | No     | No      | No     | No   |
| **Firmware Update**            | Yes (2.24+)            | Yes (2.24+)                       | Yes (2.24+)                                | Yes (2.24+)                 | Yes (2.24+)             | Yes (2.24+)             | Yes (2.24+)       | Yes (2.24+)    | Yes (2.24+) | Yes (2.24+) | No  | No                 | No                  | No             | No     | No      | No     | No   |
| **Multicamera Support**        | Not tested             | Yes                               | Yes                                        | Not tested                  | Not tested              | Not tested              | Not tested        | Yes            | Yes         | Yes         | Yes | Yes                | Yes                 | No             | No     | Yes     | N/A    | N/A  |

### SR300 Notes:

* Global Timestamp mechanism is not yet available for the SR300
* D400 JSON presets are not applicable to SR300 devices, since they rely on completely different technology
* Multicamera is technically supported similar to D400 but SR300 devices suffer from destructive interference, hence should not share field of view
* On-Chip Calibration is not applicable to the SR300 since the structured light module does not lose depth calibration over time

### T265 Tracking Module Notes:

* Tracking module is currently not available on Android
