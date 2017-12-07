# rs-fw-logger Tool

## Goal
`rs-fw-logger` tool is a console application for collecting internal camera logs.
If you are suspecting that you have an issue that is related to the camera’s firmware, please use this tool and provide the output together with the FW version number when you are creating an issue.
In order to run this, ensure that your camera is streaming. This can be done using the [realsense-viewer](https://github.com/IntelRealSense/librealsense/tree/development/tools/realsense-viewer) or [rs-capture Sample](https://github.com/IntelRealSense/librealsense/tree/development/examples/capture)

## Usage
After installing `librealsense` run `rs-fw-logger` to launch the tool. 
rs-fw-logger  >  filename – will save the FW logs to the filename.

