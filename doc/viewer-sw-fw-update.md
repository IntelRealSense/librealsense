# Viewer tool SW/FW updates

`librealsense2 Viewer` supports several ways to notify the user on software/firmware updates.
The update alerts logic "under the hood" When clicking on the `Check for updates`  button or connecting a new device button is described below.

## Online updates

The Viewer will try to download and query a versions database from the Internet and will create a version update notification based on the connected device tailored recommended version.

*The versions database online may be behind [GitHub SW releases webpage](https://github.com/IntelRealSense/librealsense/releases) or [RealSense FW releases webpage](https://dev.intelrealsense.com/docs/firmware-updates) ,
we recommend checking those links for getting latest released versions.

## Offline updates

`librealsense2` also contain a fallback recommended firmware version inside the compiled library.

When the device is in recovery mode and no suitable version will be found online (or no Internet access detected) the Viewer will notify the user to update the device firmware to a recommended version.

## Updates notifications logic flow

### Sequence Diagram
![LRS Updates Flow](./img/updates/updates.png)

*Created using  [DrawIO](https://app.diagrams.net/)





