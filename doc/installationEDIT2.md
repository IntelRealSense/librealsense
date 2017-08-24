# Linux Installation

Intel® RealSense™ Linux Installation comprises various phases:
1. Verification of 3rd Party Dependencies  
2. Updating Ubuntu, Linux OS
3. Installing Intel® RealSense™ Packages  
4. Preparing Video4Linux Backend
5. Troubleshooting Installation Errors

**Note:** Due to the USB 3.0 translation layer between native hardware and a virtual machine, the *librealsense* team does not recommend nor support installation in a VM.

## 3rd-Party Dependencies

The RealSense™ installation requires two external dependencies: *glfw3* and *libusb-1.0*   
In addition, the *Cmake* build environment requires *pkg-config*
* **Note:** *glfw3* is required only if you plan to build the example code, not for the *librealsense* core library.

**Important:** Several scripts below invoke `wget, git, add-apt-repository` which may be blocked by router settings or a firewall. Infrequently, *apt-get* mirrors or repositories may also timeout.  
For *librealsense* users behind an enterprise firewall, configuring the system-wide Ubuntu proxy generally resolves most timeout issues.

**Note:** Linux build configuration is currently configured to use the **V4L2 Backend** by default. See *Video4Linux(V4L2) Backend Preparation* below.

## Update Ubuntu to the latest stable version
* Update Ubuntu distribution, including getting the latest stable kernel  
    1. `sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade`<br />

    **Note:** On stock Ubuntu 14 LTS systems, with Kernel prior version to 4.4.0-04, the basic *apt-get upgrade* command is not sufficient to bring the distribution to the latest recommended baseline. On these systems use:  
    `sudo apt-get install --install-recommends linux-generic-lts-xenial xserver-xorg-core-lts-xenial xserver-xorg-lts-xenial xserver-xorg-video-all-lts-xenial xserver-xorg-input-all-lts-xenial libwayland-egl1-mesa-lts-xenial `<br />

    2. Check the kernel version using `uname -r` and note the exact Kernel version being installed (4.4.0-XX or 4.8.0-XX).<br />

    3. Update OS Boot Menu and reboot to enforce the correct kernel selection with:  
     `sudo update-grub && sudo reboot`<br />

    4. When rebooting, interrupt the boot process at:  
     Grub2 Boot Menu -> "**Advanced Options for Ubuntu**",  
      and select the kernel version installed in the previous step.<br />

    5. Complete the boot, login, and verify that the required kernel version (4.4.0-79 or 4.8.0-54 as of June 17th 2017) is in place with `uname -r`

## Install librealsense
1. Install the packages required for the **librealsense** build:  

  1.1 **libusb-1.0** and **pkg-config**:  
   `sudo apt-get install libusb-1.0-0-dev pkg-config libgtk-3-dev`

  1.2 **glfw3**:
    * For Ubuntu 16.04:   
    Install **glfw3** via `sudo apt-get install libglfw3-dev`  

    * For Ubuntu 14.04 use:  
     `./scripts/install_glfw3.sh` <br />      
2. Library Build Process:<br />
  **librealsense** employs **CMake** as a cross-platform build and project management system.  

  2.1 Navigate to *librealsense* root directory and run `mkdir build && cd build`<br />  

  2.2 Run **CMake**:
    * `cmake ../` - The default build is set to produce the core shared object and unit-tests binaries<br /><br />
    * `cmake ../ -DBUILD_EXAMPLES=true` - Builds *librealsense* along with the demos and tutorials<br /><br />
    * `cmake ../ -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=false` - For systems without OpenGL or X11 build only textual examples <br /> <br />  

   2.3 Recompile and Install **librealsense** binaries:<br />
  * `sudo make uninstall && make clean && make && sudo make install`<br />
  The shared object will be installed in `/usr/local/lib`, header files in `/usr/local/include`<br />
  The demos, tutorials and tests will be located in `/usr/local/bin`<br />   

  *  **Tip:** Use *`make -jX`* for parallel compilation, where *`X`* stands for the number of CPU cores available:<br />  `sudo make uninstall && make clean && make -j8 && sudo make install`<br />
  This enhancement will significantly improve build time. The side-effect, however, is that it may cause a low-end platform to hang randomly<br />  
  **Note:** Linux build configuration is currently configured to use the V4L2 backend by default.
  <br /> <br />      

3. Install IDE (Optional):
    We use **QtCreator** as an IDE for Linux development on Ubuntu    
    * Follow the  [link](https://wiki.qt.io/Install_Qt_5_on_Ubuntu) for QtCreator5 installation

## Video4Linux V4L2 Backend Preparation
Running RealSense™ Depth Cameras on Linux requires applying patches to kernel modules.<br />
**Note:** Ensure that there are no Intel RealSense cameras plugged into the system before beginning.<br />
1. Install udev rules located in librealsense source directory:<br />
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`<br />
  * `sudo udevadm control --reload-rules && udevadm trigger`<br /><br />

2. Install the *openssl* package required for kernel module build:<br />
  * `sudo apt-get install libssl-dev`<br /><br />

3. Build the patched module for the desired machine configuration.<br />
  * **Ubuntu 14/16 LTS**  
    The script will download.  
    Patch and build the uvc kernel module from source.
    * `./scripts/patch-realsense-ubuntu-xenial.sh`<br />
    There will be an attempt to insert the patched module instead of the active one. If this fails, the original uvc module will be preserved.<br /><br />
  * **Intel® Joule™ with Ubuntu**
    Based on the custom kernel provided by Canonical Ltd.
      * `./scripts/patch-realsense-ubuntu-xenial-joule.sh`<br /><br />
  * **Arch-based distributions**
    * You need to install the [base-devel](https://www.archlinux.org/groups/x86_64/base-devel/) package group.
    * You also need to install the matching linux-headers (i.e.: linux-lts-headers for the linux-lts kernel).<br />
      * Navigate to the scripts folder: `cd ./scripts/`<br />
      * Then run the following script to patch the uvc module: `./patch-arch.sh`<br /><br />

4. Check the installation by examining the latest entries in the kernel log:
  * `sudo dmesg | tail -n 50`<br />
  The log should indicate that a new uvcvideo driver has been registered. If any errors have been noted, first attempt the patching process again. If patching is not successful on the second attempt, file an issue (and make sure to copy the specific error in dmesg).

## Appendix: Troubleshooting Installation and Patch-related Issues

Error    |      Cause   | Correction Steps |
-------- | ------------ | ---------------- |
`git.launchpad... access timeout` | Behind Firewall | Configure Proxy Server |
`dmesg:... uvcvideo: module verification failed: signature and/or required key missing - tainting kernel` | A standard warning issued since Kernel 4.4-30+ | Notification only - does not affect module's functionality |
`sudo modprobe uvcvideo` produces `dmesg: uvc kernel module is not loaded` | The patched module kernel version is incompatible with the resident kernel | Verify the actual kernel version with `uname -r`.<br />Re-run the installation from the stage: [Update Ubuntu](#update-ubuntu-to-the-latest-stable-version)|
Execution of `./scripts/patch-video-formats-ubuntu-xenial.sh`  fails with `fatal error: openssl/opensslv.h` | Missing Dependency | Install the **openssl** package as instructed in: [Video4Linux Backend Preparation](#video4linux-v4l2-backend-preparation) |
