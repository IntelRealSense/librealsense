# What's New

## DS5 B0 Support

DS5 is the next generation in the family of Intel® RealSense™ cameras.
This branch will provide continuous support for everyone for all new features as they become available.

![ds5-placeholder](./ds5-placeholder.png)


## Enabled functionality
Demos:
- cpp-enumerate-devices
- cpp-tutorial-1-depth
- cpp-tutorial-2-streaming
- cpp-config-ui

Supported stream types:
- Linux and Windows:
    - Depth
    - Left imager 8-bit Infrared (Y8 monochrome)/UYVY color stream
    - Depth + Left (Y8/UYVY)
    - Calibration profiles (Advanced Mode):
      - L/R Infrared Full-HD 12bit; unrectified
      - L/R Infrared 8-bit monochrome; rectified

Controls:
- Project Mode
- Projector Power
- Manual Exposure
- Manual Gain

Extrinsic and Intrinsic API

Camera Advanced Mode API

## Known Issues
- Streaming may fail when changing stream configuration on Linux. Requires physical reconnection
- Stopping Depth+Infrared configuration on Windows takes about two seconds.
