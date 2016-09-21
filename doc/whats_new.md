# What's New

## DS5 B0 Support

DS5 is the next generation in the family of Intel® RealSense™ cameras.
This branch will provide continuous support for everyone for all new features as they become available.

![ds5-placeholder](./ds5-placeholder.png)


## Enabled functionality
Demos:
- cpp-tutorial-2-streaming
- cpp-config-ui

Supported stream types:
- Linux:
    - Depth
    - Infrared (Left)
    - Depth + Infrared
    - Calibration mode (Infrared Left + Right )
    - Depth + Calibration

- Windows
    - Depth only

Extrinsic and Intrinsic API
Control:
- Laser Power
- Manual Exposure
- Manual Gain

## Known Issues
- Exposure Control set value updates with delay
- Camera often fails when changing stream configuration on Linux. Requires physical reconnection
- No Infrared streaming on Windows
