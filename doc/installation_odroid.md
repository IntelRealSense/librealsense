# Odroid 4.14 Installation
NOTE: Intel does not officially support the Odroid line of devices. Furthermore, there are several known known [issues](../../../issues?utf8=%E2%9C%93&q=is%3Aissue%20is%3Aopen%20odroid) with running librealsense on Odroid.

To install librealsense on the Odroid, use the 4.14 image and follow the [regular instructions](installation.md) for Ubuntu 16.04 with the following exceptions:
1. Run `patch-realsense-odroid.sh` instead of `patch-realsense-lts.sh`
2. After `sudo udevadm control --reload-rules && udevadm trigger` do `sudo modprobe uvcvideo`
