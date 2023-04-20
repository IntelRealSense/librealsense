<a name="readme-top"></a>

# Linux Ubuntu Installation

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#prerequisites">Prerequisites</a>
    </li>
    <li>
        <a href="#install-dependencies">Install Dependencies</a>
    </li>
    <li>
        <a href="#install-librealsense2">Install librealsense2</a>
    </li>
    <li>
        <a href="#building-librealsense2-sdk">Building librealsense2 SDK</a>
    </li>
    <li>
        <a href="#troubleshooting-installation-and-patch-related-issues">Troubleshooting Installation and Patch-related Issues</a>
    </li>
  </ol>
</details>

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the *librealsense* team does not support installation in a VM. <br/>
If you do choose to try it, we recommend using VMware Workstation Player, and not Oracle VirtualBox for proper emulation of the USB3 controller.
<br/><br/> Please ensure to work with the supported Kernel versions listed [here](https://github.com/IntelRealSense/librealsense/releases/) and verify that the kernel is updated properly according to the instructions.

## Prerequisites

**Note:** The scripts and commands below invoke `wget, git, add-apt-repository` which may be blocked by router settings or a firewall. <br/>
Infrequently, apt-get mirrors or repositories may also cause timeout. For _librealsense_ users behind an enterprise firewall, <br/>
configuring the system-wide Ubuntu proxy generally resolves most timeout issues.

**Important:** Running RealSense Depth Cameras on Linux requires patching and inserting modified kernel drivers. <br/>
Some OEM/Vendors choose to lock the kernel for modifications. Unlocking this capability may require modification of BIOS settings

## Install Dependencies

1. Make Ubuntu up-to-date including the latest stable kernel:
   ```sh
   sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade
   ```
2. Install the core packages required to build _librealsense_ binaries and the affected kernel modules:
   ```sh
   sudo apt-get install libssl-dev libusb-1.0-0-dev libudev-dev pkg-config libgtk-3-dev cmake
   ```
   **Cmake Note:** certain _librealsense_ [CMAKE](https://cmake.org/download/) flags (e.g. CUDA) require version 3.8+ which is currently not made available via apt manager for Ubuntu LTS.
3. Install tools
   ```sh
   sudo apt-get install git wget cmake
   ```
4. Prepare Linux Backend and the Dev. Environment
   <br/>
   Unplug any connected Intel RealSense camera and run:
   <br/>
   ```sh
   sudo apt-get install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev at
   ```
5. Install IDE (Optional):
   <br/>
   We use [QtCreator](https://wiki.qt.io/Install_Qt_5_on_Ubuntu) as an IDE for Linux development on Ubuntu.

## Install librealsense2

1. Clone/Download the latest stable version of _librealsense2_ in one of the following ways:
   * Clone the _librealsense2_ repo
     ```sh
     git clone https://github.com/IntelRealSense/librealsense.git
     ```
   * Download and unzip the latest stable _librealsense2_ version from `master` branch <br/>
     [IntelRealSense.zip](https://github.com/IntelRealSense/librealsense/archive/master.zip)

   **Note**: <br/>
      * on graphic sub-system utilization: <br/>
      *glfw3*, *mesa* and *gtk* packages are required if you plan to build the SDK's OpenGL-enabled examples. <br/>
       The *librealsens2e* core library and a range of demos/tools are designed for headless environment deployment.
      * `libudev-dev` installation is optional but recommended, <br/>
        when the `libudev-dev` is installed the SDK will use an event-driven approach for triggering USB detection and enumeration, <br/>
        if not the SDK will use a timer polling approach which is less sensitive for device detection.

2. Run Intel Realsense permissions script from _librealsense2_ root directory:
   ```sh
   ./scripts/setup_udev_rules.sh
   ```
   Notice: You can always remove permissions by running: `./scripts/setup_udev_rules.sh --uninstall`

3. Build and apply patched kernel modules for:
   * Ubuntu with Kernel 4.16 <br/>
     `./scripts/patch-ubuntu-kernel-4.16.sh`
   * Ubuntu 18/20 with LTS kernel (< 5.13) <br/>
     `./scripts/patch-realsense-ubuntu-lts.sh`
   * Ubuntu 20/22 (focal/jammy) with LTS kernel 5.13, 5.15 <br/>
     `./scripts/patch-realsense-ubuntu-lts-hwe.sh`
   * Intel® Joule™ with Ubuntu based on the custom kernel provided by Canonical Ltd.  
     `./scripts/patch-realsense-ubuntu-xenial-joule.sh`
   * Arch-based distributions
     1. Install the [base-devel](https://www.archlinux.org/groups/x86_64/base-devel/) package group.
     2. Install the matching linux-headers as well (i.e.: linux-lts-headers for the linux-lts kernel).
     3. Patch the uvc module
        ```sh
        ./scripts/patch-arch.sh
        ```
   * Odroid XU4 with Ubuntu 16.04 4.14 image based on the custom kernel provided by Hardkernel
     <br/>
     `./scripts/patch-realsense-ubuntu-odroid.sh`
   
   <br/>
   <details>
   <summary>What the *.sh script perform?</summary>
      The script above will download, patch and build realsense-affected kernel modules (drivers).<br/>
      Then it will attempt to insert the patched module instead of the active one. If failed
      the original uvc modules will be restored.
   </details>

   >  Check the patched modules installation by examining the generated log as well as inspecting the latest entries in kernel log:<br />
       `sudo dmesg | tail -n 50`<br />
       The log should indicate that a new _uvcvideo_ driver has been registered.  
       Refer to [Troubleshooting](#troubleshooting-installation-and-patch-related-issues) in case of errors/warning reports.2
   
## Building librealsense2 SDK

  * Navigate to _librealsense2_ root directory and run:
    ```sh
    mkdir build && cd build
    ```
  * Run **ONE** of the following CMake commands:
    <br/>
    The default build is set to produce the core shared object and unit-tests binaries in Debug mode. <br/>
    Use `-DCMAKE_BUILD_TYPE=Release` to build with optimizations.
    ```sh
    cmake ../
    ```
    Builds _librealsense2_ along with the demos and tutorials:
    ```sh
    cmake ../ -DBUILD_EXAMPLES=true
    ```
    For systems without OpenGL or X11 build only textual examples:
    ```sh
    cmake ../ -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=false
    ```
  * Recompile and install _librealsense2_ binaries:
    <br/>
    ```sh
    sudo make uninstall && make clean && make && sudo make -j$(($(nproc)-1)) install
    ```
    <br/>

    **FYI:** The shared object will be installed in `/usr/local/lib`, header files in `/usr/local/include`. <br/>
    The binary demos, tutorials and test files will be copied into `/usr/local/bin` <br/><br/>

    **Note:** Linux build configuration is presently configured to use the V4L2 backend by default. <br/><br/>
    **Note:** If you encounter the following error during compilation `gcc: internal compiler error` <br/>
    it might indicate that you do not have enough memory or swap space on your machine. <br/>
    Try closing memory consuming applications, and if you are running inside a VM, increase available RAM to at least 2 GB. <br/><br/>
    **Note:** You can find more information about the available configuration options on [this wiki page](https://github.com/IntelRealSense/librealsense/wiki/Build-Configuration).

## Troubleshooting Installation and Patch-related Issues

| Error                                                                                                      | Cause                                                                      | Correction Steps                                                                                                                    |
|------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| `git.launchpad... access timeout`                                                                          | Behind Firewall                                                            | Configure Proxy Server                                                                                                              |
| `dmesg:... uvcvideo: module verification failed: signature and/or required key missing - tainting kernel`  | A standard warning issued since Kernel 4.4-30+                             | Notification only - does not affect module's functionality                                                                          |
| `sudo modprobe uvcvideo` produces `dmesg: uvc kernel module is not loaded`                                 | The patched module kernel version is incompatible with the resident kernel | Verify the actual kernel version with `uname -r`.<br />Revert and proceed from [Make Ubuntu Up-to-date](#install-dependencies) step |
| Execution of `./scripts/patch-video-formats-ubuntu-xenial.sh` fails with `fatal error: openssl/opensslv.h` | Missing Dependency                                                         | Install _openssl_ package                                                                                                           |

  <p align="right">(<a href="#readme-top">back to top</a>)</p>