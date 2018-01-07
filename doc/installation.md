# Linux Installation

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the *librealsense* team does not support installation in a VM. If you do choose to try it, we recommend using VMware Workstation Player, and not Oracle VirtualBox for proper emulation of the USB3 controller. 

## 3rd-party dependencies

* **Note:** On Ubuntu 16.04 LTS, make sure you have git and cmake installed: `sudo apt-get install git cmake3`

**Important:** Several scripts below invoke `wget, git, add-apt-repository` which may be blocked by router settings or a firewall. Infrequently, apt-get mirrors or repositories may also timeout. For *librealsense* users behind an enterprise firewall, configuring the system-wide Ubuntu proxy generally resolves most timeout issues.

## Make Ubuntu Up-to-date
1. Update Ubuntu distribution, including getting the latest stable kernel
    * `sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade`<br />

    **Note:** On stock Ubuntu 14 LTS systems with Kernel prior to 4.4.0-04 the basic *apt-get upgrade* command is not sufficient to bring the distribution to the latest recommended baseline. On those systems use: `sudo apt-get install --install-recommends linux-generic-lts-xenial xserver-xorg-core-lts-xenial xserver-xorg-lts-xenial xserver-xorg-video-all-lts-xenial xserver-xorg-input-all-lts-xenial libwayland-egl1-mesa-lts-xenial `<br />

    * Check the kernel version using `uname -r` and note the exact Kernel version being installed (4.4.0-XX or 4.8.0-XX) for the next step.<br />

    * Update OS Boot Menu and reboot to enforce the correct kernel selection with `sudo update-grub && sudo reboot`<br />

    * When rebooting interrupt the boot process at Grub2 Boot Menu -> "Advanced Options for Ubuntu" and select the kernel version installed in the previous step.
    * Complete the boot, login and verify that the required kernel version (4.4.0-79 or 4.8.0-54 as of June 17th 2017) is in place with `uname -r`

2. Install the packages required for *librealsense* build:
  * *libudev-dev*, *pkg-config* and *libgtk-3*: `sudo apt-get install libudev-dev pkg-config libgtk-3-dev`.
  * **Note:** glfw3 and gtk is only required if you plan to build the example code, not for the *librealsense* core library.

  * *glfw3*:
    * On Ubuntu 16.04 install glfw3 via `sudo apt-get install libglfw3-dev`
    * On Ubuntu 14.04 or when running of Ubuntu 16.04 live-disk, please use `./scripts/install_glfw3.sh`

3. Library Build Process<br />
  *librealsense* employs CMake as a cross-platform build and project management system.
  * On Ubuntu 14.04, update your build toolchain to *gcc-5*:
    * `sudo add-apt-repository ppa:ubuntu-toolchain-r/test`
    * `sudo apt-get update`
    * `sudo apt-get install gcc-5 g++-5`
    * `sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/g++ g++ /usr/bin/g++-5`
    * `sudo update-alternatives --set gcc "/usr/bin/gcc-5"`

    > You can check the gcc version by typing: `gcc -v`
    > If everything went fine you should see gcc 5.0.0.


  * Navigate to *librealsense* root directory and run `mkdir build && cd build`<br />
  * Run CMake:
    * `cmake ../` - The default build is set to produce the core shared object and unit-tests binaries<br />
    * `cmake ../ -DBUILD_EXAMPLES=true` - Builds *librealsense* along with the demos and tutorials<br />
    * `cmake ../ -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=false` - For systems without OpenGL or X11 build only textual examples<br />

  Recompile and install *librealsense* binaries:<br />
  * `sudo make uninstall && make clean && make && sudo make install`<br />
  The shared object will be installed in `/usr/local/lib`, header files in `/usr/local/include`<br />
  The demos, tutorials and tests will be located in `/usr/local/bin`<br />
  **Tip:** Use *`make -jX`* for parallel compilation, where *`X`* stands for the number of CPU cores available:<br />  `sudo make uninstall && make clean && make -j8 && sudo make install`<br />
  This enhancement will significantly improve build time. The side-effect, however, is that it may cause a low-end platform to hang randomly<br />
  **Note:** Linux build configuration is presently configured to use the V4L2 backend by default.<br />
  **Note:** If you encounter the following error during compilation `gcc: internal compiler error` it might indicate that you do not have enough memory or swap space on your machine. Try closing memory consuming applications, and if you are running inside a VM increase available RAM to at least 2 GB.

4. Install IDE (Optional):
    We use QtCreator as an IDE for Linux development on Ubuntu
    * Follow the  [link](https://wiki.qt.io/Install_Qt_5_on_Ubuntu) for QtCreator5 installation

## Video4Linux backend preparation
**Note:** Running RealSense Depth Cameras on Linux requires applying patches to kernel modules.<br />
Ensure no Intel RealSense cameras are plugged into the system before beginning.<br />
1. Install udev rules located in librealsense source directory:<br />
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
  * `sudo udevadm control --reload-rules && udevadm trigger`

2. Install *openssl* package required for kernel module build:<br />
  * `sudo apt-get install libssl-dev`<br />

3. Next, build the patched module for the desired machine configuration.<br />
  * **Ubuntu 14/16 LTS**
    The script will download, patch and build the uvc kernel module from source.<br />
    Then it will attempt to insert the patched module instead of the active one. If failed
    the original uvc module will be preserved.
    * `./scripts/patch-realsense-ubuntu-xenial.sh`<br />
  * **Intel® Joule™ with Ubuntu**
    Based on the custom kernel provided by Canonical Ltd.
      * `./scripts/patch-realsense-ubuntu-xenial-joule.sh`<br />
  * **Arch-based distributions**
    * You need to install the [base-devel](https://www.archlinux.org/groups/x86_64/base-devel/) package group.
    * You also need to install the matching linux-headers as well (i.e.: linux-lts-headers for the linux-lts kernel).<br />
      * Navigate to the scripts folder: `cd ./scripts/`<br />
      * Then run the following script to patch the uvc module: `./patch-arch.sh`<br />

4. Check installation by examining the latest entries in kernel log:
  * `sudo dmesg | tail -n 50`<br />
  The log should indicate that a new uvcvideo driver has been registered. If any errors have been noted, first attempt the patching process again, and then file an issue if not successful on the second attempt (and make sure to copy the specific error in dmesg).

## Troubleshooting Installation and Patch-related Issues

Error    |      Cause   | Correction Steps |
-------- | ------------ | ---------------- |
`git.launchpad... access timeout` | Behind Firewall | Configure Proxy Server |
`dmesg:... uvcvideo: module verification failed: signature and/or required key missing - tainting kernel` | A standard warning issued since Kernel 4.4-30+ | Notification only - does not affect module's functionality |
`sudo modprobe uvcvideo` produces `dmesg: uvc kernel module is not loaded` | The patched module kernel version is incompatible with the resident kernel | Verify the actual kernel version with `uname -r`.<br />Revert and proceed from [Make Ubuntu Up-to-date](#make-ubuntu-up-to-date) step |
Execution of `./scripts/patch-video-formats-ubuntu-xenial.sh`  fails with `fatal error: openssl/opensslv.h` | Missing Dependency | Install *openssl* package from [Video4Linux backend preparation](#video4linux-backend-preparation) step |
