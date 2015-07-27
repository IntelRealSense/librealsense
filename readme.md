# librealsense

A libusb based driver for the RealSense F200 (IVCAM) and RealSense R200 (DS4). It will work across OSX and Linux. Not sure about Windows...

# Setup

1. Install XCode
2. Install XCode command line tools
3. Install Utilities: brew, git, sublime, chrome
4. Install libusb via brew

# ToDo
1. [X] Calibration Info - z to world?
2. [X] Triple Buffering + proper mutexing
3. [X] Better frame data handling (for streams, uint8_t etc)
4. [X] Stream Handles
5. [X] Proper support for streaming intent.
6. [X] Frame count into triple buffer
7. [X] Device Handles per device! D'oh
8. [X] Context should let me know if it's R200 or F200
9. [x] openStreamWithHardwareIndex should be internal func
10. [x] Latest camera header info 
11. [x] Latest calib version info
12. [ ] F200 Device Abstraction
12. [ ] Properly namespace all the things. Half done. Maybe another rename pass...? 
13. [ ] Shield user app from libusb/libuvc headers entirely
14. [ ] Error handling for everything
15. [ ] Header Documentation
16. [ ] Cleanup @tofix declarations
17. [ ] Fix stream width/height 
18. [ ] Pointcloud lighting/camera

# Longer-Term ToDo
1. [ ] Support IR frames + decoding
2. [ ] Support autogain, etc.
3. [ ] Fix rectification / image width/height, etc. 
4. [ ] Shutdown bug
  
 ---
 
  ivcam = 0x0a66
  UVC_FRAME_FORMAT_INVI, 640, 480, 60
  
void UVCCamera::DumpInfo()
{
    //if (!isInitialized) throw std::runtime_error("Camera not initialized. Must call Start() first");
    
    //uvc_print_stream_ctrl(&ctrl, stderr);
    //uvc_print_diag(deviceHandle, stderr);
}

