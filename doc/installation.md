# Linux Installation

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM. 

## 3rd-party dependencies

The project requires two external dependencies, GLFW3 and libusb-1.0. The Cmake build environment requires pkg-config.
* **Note:** glfw3 is only required if you plan to build the example code, not for librealsense directly.

**Note:** Several scripts below invoke `wget, git, add-apt-repository` which may be blocked by router settings or a firewall. Infrequently, apt-get mirrors or repositories may also timeout. For librealsense users behind an enterprise firewall, configuring the systemwide Ubuntu proxy generally resolves most timeout issues.

1. Ensure apt-get is up to date
  * `sudo apt-get update && sudo apt-get upgrade`
  * **Note:** Use `sudo apt-get dist-upgrade`, instead of `sudo apt-get upgrade`, in case you have an older Ubuntu 14.04 version (with deprecated `nvidia-331*` packages installed), as this prevents the linux 4.4* kernel to compile properly.
2. Install libusb-1.0 and pkg-config via apt-get
  * `sudo apt-get install libusb-1.0-0-dev pkg-config`
3. glfw3 is not available in apt-get on Ubuntu 14.04. Use included installer script:
  * `scripts/install_glfw3.sh`
  * For 16.04 you can install glfw3 via `sudo apt-get install libglfw3-dev`
4. We use QtCreator as an IDE for Linux development on Ubuntu
  * **Note:** QtCreator is presently configured to use the V4L2 backend by default
  * `sudo apt-get install qtcreator`
  * `sudo scripts/install_qt.sh` (we also need qmake from the full qt5 distribution)
  * `all.pro` contains librealsense and all example applications
  * From the QtCreator top menu: Clean => Run QMake => Build
  * Built QtCreator projects will be placed into `./bin/debug` or `./bin/release`
5. We also provide a cmake file if you'd prefer to use your own favourite text editor
  * `mkdir build`
  * `cd build`
  * `cmake ..`
  * `make && sudo make install`
  * The library will be installed in `/usr/local/lib` and header files in `/usr/local/include`.
  * **Note:** If you would also like to compile the code examples use:
  * `cmake .. -DBUILD_EXAMPLES:BOOL=true`
  * `make && sudo make install`
  * The example executables will build into `./examples` and install into `/usr/local/bin`.

## Video4Linux backend

1. Ensure no cameras are presently plugged into the system.
2. Install udev rules
  * `sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/`
  * `sudo udevadm control --reload-rules && udevadm trigger`
3. Next, choose one of the following subheadings based on desired machine configuration / kernel version (and remember to complete step 4 after). **Note: ** Multi-camera support is currently NOT supported on 3.19.xx kernels. Please update to 4.4 stable. 
  * **Ubuntu 14.04.[0-4]**
    * **Update to 4.4 Stable Kernel**
      * Run the following script to install necessary dependencies (GCC 4.9 compiler and openssl) and update kernel to v4.4-wily
        * `./scripts/install_dependencies-4.4.sh`
      * Run the following script to patch uvcvideo.ko
        * `./scripts/patch-uvcvideo-4.4.sh v4.4-wily` (note the argument provided to this version of the script)
        * This script involves shallow cloning the Linux source repository (~100mb), and may take a while
    * **(OR) Keep 3.19.xx Kernel**
      * Run the following script to patch uvcvideo.ko
        * `./scripts/patch-uvcvideo-ubuntu-mainline.sh`
      * (R200 Only with 3.19.xx Kernel) Install connectivity workaround. Note this script needs to be run from within the scripts folder:
        * `cd scripts` 
        * `./install-r200-udev-fix.sh`
        * `cd ..`
        * This udev fix is not necessary for kernels >= 4.2
        * Use of 3.19.xx Kernel is not recommended. 
  * **(OR) Ubuntu 14.04.5 or Ubuntu 16.04.xx (Kernel 4.4)**
    * `./scripts/patch-uvcvideo-16.04.simple.sh`
  * **(OR) Arch based distributions**
    * You need to install the [base-devel](https://www.archlinux.org/groups/x86_64/base-devel/) and [linux-headers](https://www.archlinux.org/packages/core/x86_64/linux-headers/) package group.
	* Then run the following script to patch the uvc module:
    * `./scripts/patch-arch.sh`
4. Reload the uvcvideo driver
  * `sudo modprobe uvcvideo`
5. Check installation by examining the last 50 lines of the dmesg log:
  * `sudo dmesg | tail -n 50`
  * The log should indicate that a new uvcvideo driver has been registered. If any errors have been noted, first attempt the patching process again, and then file an issue if not successful on the second attempt (and make sure to copy the specific error in dmesg). 
