# EtherSense
Ethernet client and server for RealSense using python's Asyncore.

## Prerequisites
Installation and Setup of Server:
These steps assume a fresh install of Ubuntu 18.04 on an UpBoard but has also been tested on an Intel NUC.

```
sudo apt-get update; sudo apt-get upgrade; 

sudo apt-get install python

sudo apt-get install python-pip  

sudo apt-get install git
```

Clone the repo then run:

```
sudo python setup.py
```

This will first install the pip dependencies, followed by the creation of cronjobs in the /etc/crontab file that maintains an instance of the Server running whenever the device is powered. 

## Overview
Mulicast broadcast is used to establish connections to servers that are present on the network. 
Once a server receives a request for connection from a client, Asyncore is used to establish a TCP connection for each server. 
Frames are collected from the camera using librealsense pipeline. It is then resized and send in smaller chucks as to conform with TCP.

### UpBoard PoE 
Below shows use of a PoE switch and PoE breakout devices(avalible from online retailers) powering each dedicated UpBoard: 
This configuration should allow for a number of RealSense cameras to be connected over distances greater then 30m 
![Example Image](https://github.com/krejov100/EtherSense/blob/master/UpBoardSwitch.JPG)
The 5 RealSense cameras are connected to each UpBoard using the provided USB3 cables.

### Client Window
Below shows the result of having connected to five cameras over the local network: 
![Example Image](https://github.com/krejov100/EtherSense/blob/master/MultiCameraEthernet.png)
The window titles indicate the port which the frames are being received over.

## Error Logging
Errors are piped to a log file stored in /tmp/error.log as part of the command that is setup in /etc/crontab

## NOTES

### Power Considerations
The UpBoards require a 5v 4Amp power supply. When using PoE breakout adaptors I have found some stability issues, for example the device kernel can crash when the HDMI port is connected. As such I recommend running the UpBoard as a headless server when using PoE. 

### Network bandwidth
It is currently very easy to saturate the bandwidth of the Ethernet connection I have tested 5 servers connected to the same client without issue beyond limited framerate:

cfg.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)

self.decimate_filter.set_option(rs.option.filter_magnitude, 2)

There are a number of strategies that can be used to increase this bandwidth but are left to the user for brevity and the specific tradeoff for your application, these include:

Transmitting frames using UDP and allowing for frame drop, this requires implementation of packet ordering.

Reducing the depth channel to 8bit.

Reducing the resolution further.

The addition of compression, either frame wise or better still temporal.

Local recording of the depth data into a buffer, with asynchronous frame transfer.
 
## TroubleShooting Tips

I first of all suggest installing and configuring openssh-server on each of the UpBoards allowing remote connection from the client machine.

Check that the UpBoards are avalible on the local network using "nmap -sP 192.168.2.*"

Check that the server is running on the UpBoard using "ps -eaf | grep "python EtherSenseServer.py"

Finally check the log file at /tmp/error.log

There might still be some conditions where the Server is running but not in a state to transmit, help in narrowing these cases would be much appreciated. 

