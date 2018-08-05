# Linux Distribution

**Intel® RealSense™ SDK 2.0** provides installation packages in [`dpkg`](https://en.wikipedia.org/wiki/Dpkg) format for Ubuntu 16/18 LTS.    
\* The Realsense [DKMS](https://en.wikipedia.org/wiki/Dynamic_Kernel_Module_Support) kernel drivers package (`librealsense2-dkms`) supports Ubuntu LTS kernels 4.4, 4.10, 4.13 and 4.15\*.

**Note** Kernel 4.16 introduced a major change to uvcvideo and media subsystem. While we work to add support v4.16 in future releases,  **librealsense** is verified to run correctly with kernels v4.4-v4.15; the affected users are requested to downgrade the kernel version.


>To build the project from sources and prepare/patch the OS manually please follow steps described [here](./installation.md).


## Installing the packages:
- Register the server's public key :  
`sudo apt-key adv --keyserver keys.gnupg.net --recv-key C8B3A55A6F3EFCDE || sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key C8B3A55A6F3EFCDE`  
In case the public key still cannot be retrieved, check and specify proxy settings: `export http_proxy="http://<proxy>:<port>"`  
, and rerun the command. See additional methods in the following [link](https://unix.stackexchange.com/questions/361213/unable-to-add-gpg-key-with-apt-key-behind-a-proxy).  

- Add the server to the list of repositories :  
  Ubuntu 16 LTS:  
`sudo add-apt-repository "deb http://realsense-hw-public.s3.amazonaws.com/Debian/apt-repo xenial main" -u`  
  Ubuntu 18 LTS:  
`sudo add-apt-repository "deb http://realsense-hw-public.s3.amazonaws.com/Debian/apt-repo bionic main" -u`

 When upgrading, remove the old records:  
  - `sudo rm -f /etc/apt/sources.list.d/realsense-public.list`.  
  - `sudo apt-get update`.  

- In order to run demos install:  
  `sudo apt-get install librealsense2-dkms`  
  `sudo apt-get install librealsense2-utils`  
  The above two lines will deploy librealsense2 udev rules, build and activate kernel modules, runtime library and executable demos and tools.  

 Reconnect the Intel RealSense depth camera and run: `realsense-viewer` to verify the installation.

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
