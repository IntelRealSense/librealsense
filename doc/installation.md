# Linux Installation

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the *librealsense* team does not recommend or support installation in a VM.

## 3rd-party dependencies

The project requires two external dependencies, *glfw* and *libusb-1.0*. The Cmake build environment additionally requires *pkg-config*.
* **Note**  glfw3 is only required if you plan to build the example code, not for *librealsense* core library.

**Important** Several scripts below invoke `wget, git, add-apt-repository` which may be blocked by router settings or a firewall. Infrequently, apt-get mirrors or repositories may also timeout. For *librealsense* users behind an enterprise firewall, configuring the system-wide Ubuntu proxy generally resolves most timeout issues.

## Make Ubuntu Up-to-date
1. Update Ubuntu distribution, including getting the latest stable kernel
    * `sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade`<br />

    Check the kernel version - > `uname -r`<br />
    In case of stack Ubuntu 14 LTS with Kernel prior to 4.4.0-04 (e.g. 3.19..) the basic *apt-get upgrade* rule is not sufficient to bring the distribution to the latest baseline recommended.<br />
    Therefore, perform the following command to promote both Kernel and FrontEnd <br />
    * `sudo apt-get install --install-recommends linux-generic-lts-xenial xserver-xorg-core-lts-xenial xserver-xorg-lts-xenial xserver-xorg-video-all-lts-xenial xserver-xorg-input-all-lts-xenial libwayland-egl1-mesa-lts-xenial `<br />

    Note the exact Kernel version being installed (4.4.0-XX) for the next step.<br />

    At the end update OS Boot Menu and reboot to enforce the correct kernel selection<br />
    * `sudo update-grub && sudo reboot`<br />

    Interrupt the boot process at  Grub2 Boot Menu -> "Advanced Options for Ubuntu" -> Select the kernel version installed in the previous step.<br />
    Complete the boot, login and verify that the required kernel version in place
    * `uname -r`  >=  4.4.0-50

2. Install the packages required for *librealsense* build: <br />
  *libusb-1.0* and *pkg-config*:<br />
  * `sudo apt-get install libusb-1.0-0-dev pkg-config`.

  *glfw3*:<br />
  For Ubuntu 14.04: use
    * `./scripts/install_glfw3.sh`<br />

  For Ubuntu 16.04 install glfw3 via
    * `sudo apt-get install libglfw3-dev`

3. Library Build Process<br />
  *librealsense* employs CMake as a cross-platform build and project management system.
  Navigate to *librealsense* root directory and run<br />
  * `mkdir build && cd build`<br />
  The default build is set to produce the core shared object and unit-tests binaries
  * `cmake ../`<br />
  In order to build *librealsense* along with the demos and tutorials use<br />
  * `cmake ../ -DBUILD_EXAMPLES=true`

  Generate and install binaries:<br />
  * `make && sudo make install`<br />
  The library will be installed in `/usr/local/lib`, header files in `/usr/local/include`<br />
  The demos, tutorials and tests will located in `/usr/local/bin`.<br />
  **Note:** Linux build configuration is presently configured to use the V4L2 backend by default

4. Install IDE (Optional):
    We use QtCreator as an IDE for Linux development on Ubuntu
    * Follow the  [link](https://wiki.qt.io/Install_Qt_5_on_Ubuntu) for QtCreator5 installation

## Video4Linux backend preparation
**Note:** Running RealSense Depth Cameras on Linux requires applying patches to kernel modules.<br />
Ensure no Intel RealSense cameras are presently plugged into the system.<br />
Install udev rules located in librealsense source directory:<br />
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
  * `sudo udevadm control --reload-rules && udevadm trigger`

Install *openssl* package required for kernel module build:<br />
  * `sudo apt-get install libssl-dev`<br />

Next, build the patched module for the desired machine configuration.<br />
  * **Ubuntu 14/16 LTS**
    The script will download, patch and build uvc kernel module from sources.<br />
    Then it will attempt to insert the patched module instead of the active one. If failed
    the original uvc module will be preserved.
    * `./scripts/patch-realsense-ubuntu-xenial.sh`<br />

  * **Arch-based distributions**
    * You need to install the [base-devel](https://www.archlinux.org/groups/x86_64/base-devel/) package group.
Then run the following script to patch the uvc module:
    * `./scripts/patch-arch.sh`<br />

Check installation by examining the latest entries in kernel log:
  * `sudo dmesg | tail -n 50`<br />
  The log should indicate that a new uvcvideo driver has been registered. If any errors have been noted, first attempt the patching process again, and then file an issue if not successful on the second attempt (and make sure to copy the specific error in dmesg).

## Troubleshooting Installation and Patch-related Issues

Error    |      Cause   | Correction Steps |
-------- | ------------ | ---------------- |
`git.launchpad... access timeout` | Behind Firewall | Configure Proxy Server |
`dmesg:... uvcvideo: module verification failed: signature and/or required key missing - tainting kernel` | A standard warning issued since Kernel 4.4-30+ | Notification only - does not affect module's functionality |
`sudo modprobe uvcvideo` produces `dmesg: uvc kernel module is not loaded` | The patched module kernel version is incompatible with the resident kernel | Verify the actual kernel version with `uname -r`.<br />Revert and proceed on **Make Ubuntu Up-to-date** step |
Execution of `./scripts/patch-video-formats-ubuntu-xenial.sh`  fails with `fatal error: openssl/opensslv.h` | Missing Dependency | Install *openssl* package from **Video4Linux backend preparation** step |
