# rs-enumerate-devices Tool

## Goal
`rs-enumerate-devices` tool is a console application providing information about connected devices.

## Usage
After installing `librealsense` run ` rs-enumerate-devices` to launch the tool.
An example for output for a D415 camera is:

```
Name                          :     Intel RealSense 415    
Serial Number                 :     725112060346     
Firmware Version              :     05.08.14.00
Physical Port                 :     /sys/devices/pci0000:00/0000:00:14.0/usb2/2-2/2-2:1.0/video4linux/video1
Debug Op Code                 :     15
Advanced Mode                 :     YES
Product Id                    :     0AD3
```

## Command Line Parameters

|Flag   |Description   |
|---|---|
|`-s`|Provide short summary, one line per device|
|`-o`|List supported device controls and options|
|`-c`|Provide calibration (Intrinsic/Extrinsic) information|
| None| List supported streaming modes|
