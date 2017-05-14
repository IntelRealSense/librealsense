# Python Wrapper

## Table of Contents
* [Installation Guide](#installation-guide)
* [Examples](#examples)

## Installation Guide

### Ubuntu 14.04/16.04 LTS Installation
1. Ensure apt-get is up to date
  * `sudo apt-get update && sudo apt-get upgrade`
  * **Note:** Use `sudo apt-get dist-upgrade`, instead of `sudo apt-get upgrade`, in case you have an older Ubuntu 14.04 version
2. Install Python and its development files via apt-get (Python 2 and 3 both work)
  * `sudo apt-get install python python-dev` or `sudo apt-get install python3 python3-dev`
  * **Note:** The project will only use Python2 if it can't use Python3
3. run the toplevel cmake command with the following additional flag: -DBUILD_PYTHON_BINDINGS=bool:true

### Windows Installation
1. Install Python 2 or 3 for windows
  * You can find the downloads on the official Python website [here](https://www.python.org/downloads/windows/)
2. When running cmake, select the BUILD_PYTHON_BINDINGS option

## Examples
```python
import time
import pyrealsense2 as rs

def callback(frame):
    print "got frame #", frame.get_frame_number()

ctx = rs.context()
devs = ctx.query_devices()
dev = devs[0]

profile = rs.stream_profile(rs.stream.depth, 640, 480, 30, rs.format.z16)
dev.open(profile)
dev.start(callback)

time.sleep(5) # Callback is running

dev.stop()
dev.close()
```

A longer example can be found [here](../examples/python-tutorial-1-depth.py)
