# rs-fw-update Tool

## Goal
`rs-fw-update` tool is a console application for updating the depth cameras firmware.

## Prerequisites
In order to update a depth camera firmware, a signed image file is required.
The latest firmware for D400 cameras is available [here.](https://downloadcenter.intel.com/download/28870/Latest-Firmware-for-Intel-RealSense-D400-Product-Family?product=128255)
The firmware is packed into zip file and contains a file with "bin" extension with the following naming convension: "Signed_Image_UVC_<firmware_version>.bin"

## Usage
After installing `librealsense` run ` rs-fw-update -l` to launch the tool and print a list of connected devices.
An example for output for a D415 camera is:

```
connected devices:
1) Name: Intel RealSense D415, serial number: 725112060411, ASIC serial number: 012345678901, firmware version: 05.11.01.100, USB type: 3.2
```

Then we will provid the serial number to identify the device together with the path to firmware file that we want to update ` rs-fw-update -s 725112060411 -f Signed_Image_UVC_5_11_6_250.bin`.
An example for the expected output is:

```
search for device with serial number: 725112060411

update to FW: Signed_Image_UVC_5_11_6_250.bin

updating device:
Name: Intel RealSense D415, serial number: 725112060411, ASIC serial number: 012345678901, firmware version: 05.11.01.100, USB type: 3.2

firmware update started

firmware update progress: 100[%]

firmware update done

device 725112060411 successfully updated to FW: 05.11.06.250
```

In case only one camera is connected you can simply run ` rs-fw-update -f Signed_Image_UVC_5_11_6_250.bin`.

A camera/s might be in a recovery state, in such case listing the devices will output the following:

```
connected devices:
1) Name: Intel RealSense D4xx Recovery, serial number: unknown, ASIC serial number: 012345678901, firmware version: unknown, USB type: unknown
```

In such case we can use the recovery flag and run ` rs-fw-update -r -f Signed_Image_UVC_5_11_6_250.bin`
An example for the expected output is:

```
update to FW: Signed_Image_UVC_5_11_6_250.bin

recovering device:
Name: Intel RealSense D4xx Recovery, serial number: unknown, ASIC serial number: 012345678901, firmware version: unknown, USB type: unknown

firmware update started

firmware update progress: 100[%]

firmware update done

recovery done

```

## Command Line Parameters

|Flag   |Description   |
|---|---|
|`-s`| The serial number of the device to be update, this is mandetory if more than one device is connected|
|`-f`|Path of the firmware image file|
|`-r`|Recover all connected devices which are in recovery mode|
|`-l`|List all available devices and exits|
|`-v`|Displays version information and exits|
|`-h`|Displays usage information and exits|
| None| List supported streaming modes|

## Limitation
* Do not run additional applications that use the RealSense camera, such as Viewer, together with the rs-fw-update tool
