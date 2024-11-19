# dds-config tool

## Goal
This tool is used to define a device as a DDS-compatible device and ensure it operates correctly within a DDS environment.  
It allows users to manage connection priorities, enabling the selection of either DDS or USB as the preferred connection method when both options are available.

## Description
Typically, the PC which the device needs to be connected to will have only one ethernet port.  
However, two ethernet ports are required for the setup:  
* One Ethernet port is needed to connect to the network - for remote access.
* The second Ethernet port is needed to connect to the device - for DDS communication.

To enable this configuration, a USB to ethernet adapter can be used, allowing the network connection to be routed through a USB port.  
The dds-config tool helps manage this setup by enabling users to prioritize either the USB or DDS connection, based on the specified parameters. It also provides information about the connected devices when executed.  

## Command Line Parameters
| Flag | Description |
|---|---|
|'-h --help'|Show command line help menu|
|'--usb-first'|Prioritize USB before Ethernet|
|'--eth-first'|Prioritize Ethernet and fall back to USB after link timeout|
|'--dynamic-priority'|Dynamically prioritize the last-working connection method (the default)|

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