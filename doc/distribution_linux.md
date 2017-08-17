# Linux Distribution

For Linux, RealSense™ cross-platform SDK provides installation packages in [`dpkg`](https://en.wikipedia.org/wiki/Dpkg) format for Debian OS and its derivatives.  
The packages and their respective content are listed below:  

Name    |      Content   | Depends on |
-------- | ------------ | ---------------- |
realsense-sdk-udev-rules | Manages the devices permissions configuration for | -
realsense-uvcvideo | DKMS package for Depth cameras-specific kernel extensions | realsense-sdk-udev-rules
realsense-sdk | RealSense™ SDK runtime (.so) and configuration files | realsense-sdk-udev-rules
realsense-utils | Demos and tools available as a part of RealSense™ SDK | realsense-sdk
realsense-dev | Header files and symbolic link for developers | realsense-sdk
realsense-dbg | Debug symbols for developers  | realsense-sdk

**Note** The packages include binaries only.
Use the github repository to obtain the source code.

## Installing the packages:
- Add Intel server  to the list of repositories :  
`echo 'deb http://realsense-hw-public.s3.amazonaws.com/Debian/apt-repo xenial main' | sudo tee /etc/apt/sources.list.d/realsense-public.list`  
It is recommended to backup `/etc/apt/sources.list.d/realsense-public.list` file in case of an upgrade.

- Register the server's public key :  
`sudo apt-key adv --keyserver keys.gnupg.net --recv-key 7DCC6F0F`  
- Refresh the list of repositories and packages available :  
`sudo apt-get update`  

- In order to run demos install :  
`sudo apt-get install realsense-utils`  
`sudo apt-get install realsense-uvcvideo`  

- Developers shall install additional packages:  
`sudo apt-get install realsense-dev`  
`sudo apt-get install realsense-dbg`  

  Complete the installation by performing  
  `reboot`   
  to reload the kernel with modules provided by DKMS.

  Verify that the kernel is updated :    
  `modinfo uvcvideo | grep "version:"` should include `realsense` string

## Uninstalling the Packages:
**Important** Removing Debian package is allowed only when no other installed packages directly refer to it. For example removing `realsense-sdk-udev-rules` requires `realsense-sdk` to be removed first.

Remove a single package with:   
  `sudo apt-get --purge <package-name>`  

Remove all RealSense™ SDK-related packages with:   
  `dpkg -l | grep "realsense" | cut -d " " -f 3 | xargs sudo dpkg --purge`  
