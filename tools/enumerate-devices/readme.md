# rs-enumerate-devices Tool

## Goal
`rs-enumerate-devices` tool is a console application providing information about the connected devices.

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
|`-s`|Provide a one-line summary, one line per device|
|`-S`|Extends `-s` by providing a short summary for each device|
|`-o`|List supported device controls, options and streaming modes|
|`-c`|Provide calibration (Intrinsic/Extrinsic) and steraming modes information|
|`-p`|Enumerate streams contained in ROSBag record file. Usage `-p <rosbag_full_path>`|
| None| The default mode. Equivalent to `-S` plus the list of all the supported streaming profiles|

The options `-o`, `-c` `-p` are additive and can concatenated:
`rs-enumerate-devices -o -c -p rosbag.rec` - will print the camera info, streaming modes,
supported option and the calibration info both for the live cameras and the prerecorded `rosbag.rec` file.

The options `-S`, `-s` are restrictive and therefore incompatible with `-o` and `-c`,
but still can be used with `-p <file_name>`.
