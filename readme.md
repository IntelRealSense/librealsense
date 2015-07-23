# librealsense

A libusb based driver for the RealSense F200 (IVCAM) and RealSense R200 (DS4). It will work across OSX and Linux. Not sure about Windows...

# Setup

1. Install XCode
2. Install XCode command line tools
3. Install Utilities: brew, git, sublime, chrome
4. Install libusb via brew

# ToDo
[X] Calibration Info - z to world?
[X] Triple Buffering + proper mutexing
[X] Better frame data handling (for streams, uint8_t etc)
[X] Stream Handles
[X] Proper support for streaming intent.
[X] Frame count into triple buffer
[] Device Handles per device! D'oh
[] StartStream should already know about which streams to start without additional idx

[] Sterling linalg from Melax sandbox
[] Pointcloud projection + shader
[] Orbit camera for pointcloud
[] Make drawing textures not suck

[] Properly namespace all the things
[] Shield user app from libusb/libuvc headers entirely
[] F200 Device Abstraction
[] Error handling for everything

# Longer-Term ToDo
[] Support IR frames + decoding
[] Support autogain, etc.
[] Fix rectification / image width/height, etc. 

ivcam = 0x0a66
UVC_FRAME_FORMAT_INVI, 640, 480, 60
