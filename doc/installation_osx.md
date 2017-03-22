# Apple OSX Installation  

**Note:** OSX backend is temporarily unavailable. For OSX support for R200, F200, SR300 please use the [the master branch](https://github.com/IntelRealSense/librealsense/tree/master)

**Note:** Due to the USB 3.0 translation layer between native hardware and virtual machine, the librealsense team does not recommend or support installation in a VM.

1. Install XCode 6.0+ via the AppStore
2. Install the Homebrew package manager via terminal - [link](http://brew.sh/)
3. Install pkg-config and libusb via brew:
  * `brew install libusb pkg-config`
4. Install glfw3 via brew:
  * `brew install homebrew/versions/glfw3`
