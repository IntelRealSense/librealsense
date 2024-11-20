# dds-config tool

## Goal
This tool is used to configer DDS based device and ensure it operates correctly within a DDS environment.  
It allows users to manage connection priorities, enabling the selection of either DDS or USB as the preferred connection method when both options are available.
## Description
The dds-config tool allows users to prioritize either the DDS or USB connection and configure network parameters for the device.  
It also provides information about the connected devices when executed.

## Command Line Parameters
| Flag | Description |
|---|---|
|-h --help|Show command line help menu|
|--usb-first|Prioritize USB before Ethernet|
|--eth-first|Prioritize Ethernet and fall back to USB after link timeout|
|--dynamic-priority|Dynamically prioritize the last-working connection method (the default)|
|--version|Displays version information and exits|
|--serial-number <S/N>|Device serial-number to use, if more than one device is available|
|--reset|Hardware reset without making any changes|
|--link-timeout <milliseconds>|Milliseconds before --eth-first link times out and falls back to USB|
|--dhcp <on/off>|DHCP dynamic IP discovery 'on' or 'off'|
|--ip <address>|Device static IP address to use when DHCP is off|
|--domain-id <0-232>|DDS Domain ID to use (default is 0), note:this is the device domain ID not librealsense domain ID|
|--golden| Show R/O golden values vs. current; mutually exclusive with any changes|
|--factory-reset|Reset settings back to the --golden values|
|--dhcp-timeout <seconds>|Seconds before DHCP times out and falls back to a static IP|
|--mask <1.2.3.4>|Device static IP network mask to use when DHCP is off|
|--gateway <1.2.3.4>|Displays version information and exits|
|--no-reset|Do not hardware reset after changes are made|

## Usage example
Prioritize Ethernet connection by using `rs-dds-config.exe --eth-first`:  
```
     Device: [USB] Intel RealSense D555 s/n 338522301774
     MAC address: 98:4f:ee:19:cc:49
     configured: 192.168.11.55 & 255.255.255.0 / 192.168.11.1
     DDS:
         domain ID: 0
         link: 1000 Mbps
         MTU, bytes: 9000
         timeout, ms: 10000
         priority: usb-first --> eth-first
     DHCP: OFF
         timeout, sec: 30
     Successfully changed
     Resetting device...
```
After running `rs-dds-config.exe` we can now see the device changed it's connection to DDS:
```
     Device: [DDS] Intel RealSense D555 s/n 338522301774
     MAC address: 98:4f:ee:19:cc:49
     configured: 192.168.11.55 & 255.255.255.0 / 192.168.11.1
     DDS:
         domain ID: 0
         link: 1000 Mbps
         MTU, bytes: 9000
         timeout, ms: 10000
         priority: eth-first
     DHCP: OFF
         timeout, sec: 30
```