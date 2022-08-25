# Overview

LRS-Net 3.0 is the LRS subsystem which enables access to RealSense cameras over the network. It including following features:

* Server-side software upgrade support.
* Firmware update for the remote camera.
* Set of LRS-Net 2.0 features, with the following majors to highlight:
  * Support for networks with high packet loss (WLAN/WAN).
  * Better compression for depth streams.
  * Optimized JPEG compression for color streams.
  * Motion module support.
  * Full set of device profiles support.
  * Full set of device options support.
  * Full metadata support.

# Deliverables

The image for RP4 containing preinstalled LRS-Net 3.0 server located [here](https://demo.filestash.app/s/ApbHfQi).
The precompiled client-side package containing viewer and upgrade utility located [here](https://demo.filestash.app/s/BniNVFD).
The Pull Request updated with the latest source located [here](https://github.com/IntelRealSense/librealsense/pull/8343).

# Installation

## Client
### Linux
Checkout the source specified above. Configure it with the following commands:
`mkdir build && cd build && cmake .. -DBUILD_NETWORK_DEVICE=true -DBUILD_UNIT_TESTS=false -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=true -DBUILD_WITH_TM2=false -DFORCE_RSUSB_BACKEND=true -DBUILD_SHARED_LIBS=true`

After the build you can find the viewer, server and upgrade utility in *build/tools/realsense-viewer* and *build/tools/rs-server* folders.

### Windows
You can use the precompiled tools downloaded from the link specified in the deliverables section.
To build from the source the following configuration command might be used:
`cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_NETWORK_DEVICE=true -DBUILD_UNIT_TESTS=false -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=true -DBUILD_WITH_TM2=false -DBUILD_SHARED_LIBS=true`
	
## Server
### Linux
The same steps as building the client.

### Windows
No server supported now for on Windows.

### RP4
You can use the preconfigured image downloaded from the link specified in the deliverables section. Flash it into the microSD card using balenaEtcher and follow instruction from next section to configure it. If you have RP4 installed you might use the DEB package from the client tools package specified in the deliverables section.

Also, you might want to built the server from the sources directly on RP4. To configure the source the following command might be used:
`mkdir build && cd build && cmake .. -DBUILD_NETWORK_DEVICE=true -DBUILD_UNIT_TESTS=false -DBUILD_EXAMPLES=false -DBUILD_GRAPHICAL_EXAMPLES=false -DBUILD_WITH_TM2=false -DFORCE_RSUSB_BACKEND=true -DBUILD_SHARED_LIBS=true`

After the build you can find the server and upgrade utility in *build/tools/rs-server* folder.

TODO: cross-compilation explanation.

# Board access.

There are two scenarios to access the board. One assumes the user can connect monitor and keyboard to the board. The second scenario allows to access the board using USB connection only.

You will need Bonjour as explained [here](https://learn.adafruit.com/bonjour-zeroconf-networking-for-windows-and-linux/). Most Linux systems already have it installed. 

## Linux
1. After connecting to USB make the new wired connection shared using nm-connection-editor (install it if needed).
1. Make SSH connection to pi@raspberrypi.local.
1. Login using *pi* username and *raspberry* for the password.
1. Adjust */etc/dhcpcd.conf* as whitepaper explains.
1. Start *realsense-viewer* on the other side.

## Windows
1. Use Putty to open SSH connection to pi@raspberrypi.local.
1. Login using *pi* username and *raspberry* for the password.
1. Adjust */etc/dhcpcd.conf* as whitepaper explains.
1. Start realsense-viewer on the other side.
[Source](https://learn.adafruit.com/turning-your-raspberry-pi-zero-into-a-usb-gadget/ethernet-gadget)

Windows might misdetect RP4 device and might be manually enforced to use Microsoft RNDIS driver.
1. Download and install the 'Acer Incorporated. - Other hardware - USB Ethernet/RNDIS Gadget' for 'Windows 7,Windows 8,Windows 8.1 and later drivers' from [here](https://www.catalog.update.microsoft.com/Search.aspx?q=USB%20RNDIS%20Gadget).
1. Double-click the downloaded .CAB file.
1. Select both files ( RNDIS.inf and rndis.cat).
1. Right-click then select Extract.
1. Select RNDIS.inf.
1. Right-click then select Install.
1. Click Open.
1. After a successful operation, please try again to use Putty to open SSH connection to pi@raspberrypi.local.

Alternatively,
From PC, go to Device Manager > Network Adapters > Look for unknown or problematic driver related to USB Ethernet.
Open up the problem/unknown driver, choose "Update driver" and select "USB Ethernet/RNDIS Gadget". 
Once install successfully, PC will be able to communicate with RFID Reader via USB connection.

[Source](https://supportcommunity.zebra.com/s/article/Install-RNDIS-Driver-to-connect-RFID-reader-via-USB)

# Configuration.

## DHCP over Ethernet.
Just flash the image to micro SD and boot Raspberry Pi. Ensure it has the ethernet connection to the LAN with an active DHCP server. Connect a monitor and a keyboard to check the assigned IP address. Another solution is to check logs of the DHCP server.

## DHCP over Wi-Fi.
Wireless networking requires additional configuration. Connect to the board via SSH and run *raspi-config* utility to enable and configure Wi-Fi. Again, connect a monitor and a keyboard or check DHCP server logs to check assigned IP address.

## Static over Ethernet.
Connect to the board via SSH and adjust */etc/dhcpcd.conf* as described in the white paper.

## Static over Wi-Fi.
Connect to the board via SSH and adjust */etc/dhcpcd.conf* as described in the white paper.

## Multihomed configuration.
In some setups the board could be connected to Ethernet and Wi-Fi networks simultaneously. To tell the server which network to use put the interface name into the *IFACE_DEFAULT* variable at the beginning of */usr/bin/start-rs-server.sh* script.

# Remote FW/SW update.

To upgrade the firmware of the remote camera, you need to run the *fw-server-upgrade* tool in the following way (substitute 172.16.1.111 with the IP of your RP4 board and the name of firmware image with the downloaded one):
`rs-server-upgrade -f ./Signed_Image_UVC_5_12_7_100.bin -a 172.16.1.111 --fw`

To upgrade the server itself, you need to run the *fw-server-upgrade* tool in the following way (substitute 172.16.1.111 with the IP of your RP4 board):
`rs-server-upgrade -f librealsense-2.41.0-1.deb -a 172.16.1.111 --sw`

The latest Debian package is included in the archive referenced above.

