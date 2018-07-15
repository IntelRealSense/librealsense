# Linux Distribution

**Intel® RealSense™ SDK 2.0** provides installation packages in [`dpkg`](https://en.wikipedia.org/wiki/Dpkg) format for Ubuntu 16 LTS\*.    
\* The Realsense [DKMS](https://en.wikipedia.org/wiki/Dynamic_Kernel_Module_Support) kernel drivers package (`librealsense2-dkms`) supports Ubuntu LTS kernels 4.4, 4.10 and 4.13.


**Note** Kernel 4.16 introduces some non-compatible modification to uvcvideo and media subsystem. We work to support it in future releases, but at this time the support is scoped up to kernel 4.15 and the affected users are requested to downgrade the kernel version.


> To build the project from source, or in case of Ubuntu 18 (Bionic Beaver) with kernel 4.15 please follow steps described [here](./installation.md) to manually install the kernel patches



## Installing the packages:
- Add Intel server  to the list of repositories :  
`echo 'deb http://realsense-hw-public.s3.amazonaws.com/Debian/apt-repo xenial main' | sudo tee /etc/apt/sources.list.d/realsense-public.list`  
It is recommended to backup `/etc/apt/sources.list.d/realsense-public.list` file in case of an upgrade.

- Register the server's public key :  
`sudo apt-key adv --keyserver keys.gnupg.net --recv-key C8B3A55A6F3EFCDE`  
- Refresh the list of repositories and packages available :  
`sudo apt-get update`  

- In order to run demos install:  
  `sudo apt-get install librealsense2-dkms`  
  `sudo apt-get install librealsense2-utils`  
  The above two lines will deploy librealsense2 udev rules, kernel drivers, runtime library and executable demos and tools.
  Reconnect the Intel RealSense depth camera and run: `realsense-viewer`  

- Developers shall install additional packages:  
  `sudo apt-get install librealsense2-dev`  
  `sudo apt-get install librealsense2-dbg`  
  With `dev` package installed, you can compile an application with **librealsense** using `g++ -std=c++11 filename.cpp -lrealsense2` or an IDE of your choice.


  Verify that the kernel is updated :    
  `modinfo uvcvideo | grep "version:"` should include `realsense` string

## Uninstalling the Packages:
**Important** Removing Debian package is allowed only when no other installed packages directly refer to it. For example removing `librealsense2-udev-rules` requires `librealsense2` to be removed first.

Remove a single package with:   
  `sudo apt-get --purge <package-name>`  

Remove all RealSense™ SDK-related packages with:   
  `dpkg -l | grep "realsense" | cut -d " " -f 3 | xargs sudo dpkg --purge`  

## Package Details:
The packages and their respective content are listed below:  

Name    |      Content   | Depends on |
-------- | ------------ | ---------------- |
librealsense2-udev-rules | Configures RealSense device permissions on kernel level  | -
librealsense2-dkms | DKMS package for Depth cameras-specific kernel extensions | librealsense2-udev-rules
librealsense2 | RealSense™ SDK runtime (.so) and configuration files | librealsense2-udev-rules
librealsense2-utils | Demos and tools available as a part of RealSense™ SDK | librealsense2
librealsense2-dev | Header files and symbolic link for developers | librealsense2
librealsense2-dbg | Debug symbols for developers  | librealsense2

**Note** The packages include binaries and configuration files only.
Use the github repository to obtain the source code.
