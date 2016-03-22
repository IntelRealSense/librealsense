# Table of Contents
* [Ubuntu 14.04 LTS Installation](#ubuntu-1404-lts-installation)
* [Apple OSX Installation](#apple-osx-installation)
* [Windows 8.1/10 Installation](#windows-81-installation)

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM. 

# Ubuntu 14.04 LTS Installation

Installation of cameras on Linux is lengthy compared to other supported platforms. Several upstream fixes to the uvcvideo driver have been merged in recent kernel versions, greatly enhancing stability. Once an updated kernel has been installed, one more patch must be applied to the uvcvideo driver with support for several non-standard pixel formats provided by RealSense™ cameras.

**Note:** Several scripts below invoke `wget, git, add-apt-repository` which may be blocked by router settings or a firewall. Infrequently, apt-get mirrors or repositories may also timeout. For librealsense users behind an enterprise firewall, configuring the systemwide Ubuntu proxy generally resolves most timeout issues.

1. Ensure apt-get is up to date
  * `sudo apt-get update && sudo apt-get upgrade`
2. Install libusb-1.0 via apt-get
  * `sudo apt-get install libusb-1.0-0-dev`
3. glfw3 is not available in apt-get on Ubuntu 14.04. Use included installer script:
  * `scripts/install_glfw3.sh`
4. **Follow the installation instructions for your desired backend (see below)**
5. We use QtCreator as an IDE for Linux development on Ubuntu
  * **Note:** QtCreator is presently configured to use the V4L2 backend by default
  * `sudo apt-get install qtcreator`
  * `sudo scripts/install_qt.sh` (we also need qmake from the full qt5 distribution)
  * `all.pro` contains librealsense and all example applications
  * From the QtCreator top menu: Clean => Run QMake => Build
  * Built QtCreator projects will be placed into `./bin/debug` or `./bin/release`
6. We also provide a makefile if you'd prefer to use your own favorite text editor
  * `make && sudo make install`
  * The example executables will build into `./bin`

## Video4Linux backend

1. Ensure no cameras are presently plugged into the system.
2. Install udev rules 
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
  * Reboot or run `sudo udevadm control --reload-rules && udevadm trigger` to enforce the new udev rules
3. Next, choose one of the following subheadings based on desired machine configuration / kernel version (and remember to complete step 4 after). **Note: ** Multi-camera support is currently NOT supported on 3.19.xx kernels. Please update to 4.4 stable. 
  * **Updated 4.4 Stable Kernel** (recommended)
    * Run the following script to install necessary dependencies (GCC 4.9 compiler and openssl) and update kernel to v4.4-wily
      * `./scripts/install_dependencies-4.4.sh`
    * Run the following script to patch uvcvideo.ko
      * `./scripts/patch-uvcvideo-4.4.sh v4.4-wily` (note the argument provided to this version of the script)
      * This script involves shallow cloning the Linux source repository (~100mb), and may take a while
  * **(OR) Stock 3.19.xx Kernel in 14.04.xx** (not recommended)
    * Run the following script to patch uvcvideo.ko
      * `./scripts/patch-uvcvideo-3.19.sh`
    * (R200 Only) Install connectivity workaround
      * `./scripts/install-r200-udev-fix.sh`
      * This udev fix is not necessary for kernels >= 4.2.3
4. Reload the uvcvideo driver
  * `sudo modprobe uvcvideo`
5. Check installation by examining the last 50 lines of the dmesg log:
  * `sudo dmesg | tail -n 50`
  * The log should indicate that a new uvcvideo driver has been registered. If any errors have been noted, first attempt the patching process again, and then file an issue if not successful on the second attempt (and make sure to copy the specific error in dmesg). 

## LibUVC backend

**Note:** This backend has been deprecated on Linux.

The libuvc backend requires that the default linux uvcvideo.ko driver be unloaded before libusb can touch the device. This is because uvcvideo will 'own' a UVC device the moment is is plugged in (user-space applications do not have permission to access the devuce handle). Follow the instructions below to install the udev permissions script.

The libuvc backend has known incompatibilities with some versions of SR300 and R200 firmware (1.0.71.xx series of firmwares are problematic). 

1. Grant appropriate permissions to detach the kernel UVC driver when a device is plugged in:
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
  * `sudo cp config/uvc.conf /etc/modprobe.d/`
  * Either reboot or run `sudo udevadm control --reload-rules && udevadm trigger` to enforce the new udev rules
2. Use the makefile to build the LibUVC backend
  * `make BACKEND=LIBUVC`
  * `sudo make install`

---

# Apple OSX Installation  

1. Install XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install libusb via brew:
  * `brew install libusb`
4. Install glfw3 via brew:
  * `brew install homebrew/versions/glfw3`

---

# Windows 8.1 Installation

librealsense should compile out of the box with Visual Studio 2013 Release 5. Particular C++11 features are known to be incompatible with earlier VS2013 releases due to internal compiler errors. GLFW is provided in the solution as a NuGet package.

librealsense has not been tested with Visual Studio Community Edition.
