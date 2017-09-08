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
3. run the toplevel cmake command with the following additional flag: `-DBUILD_PYTHON_BINDINGS=bool:true`

**Note**: To force compilation with a specific version on a system with both Python 2 and Python 3 installed, add the following flag to CMake command:
`-DPYTHON_EXECUTABLE=[full path to the exact python executable]`

### Windows Installation
1. Install Python 2 or 3 for windows
  * You can find the downloads on the official Python website [here](https://www.python.org/downloads/windows/)
2. When running cmake, select the BUILD_PYTHON_BINDINGS option

## Examples
```python
# First import the library
import pyrealsense2 as rs

try:
    # Create a context object. This object owns the handles to all connected realsense devices
    pipeline = rs.pipeline()
    pipeline.start()

    while True:
        # This call waits until a new coherent set of frames is available on a device
        # Calls to get_frame_data(...) and get_frame_timestamp(...) on a device will return stable values until wait_for_frames(...) is called
        frames = pipeline.wait_for_frames()
        depth = frames.get_depth_frame()
        if not depth: continue

        # Print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and approximating the coverage of pixels within one meter
        coverage = [0]*64
        for y in xrange(480):
            for x in xrange(640):
                dist = depth.get_distance(x, y)
                if 0 < dist and dist < 1:
                    coverage[x/10] += 1
            
            if y%20 is 19:
                line = ""
                for c in coverage:
                    line += " .:nhBXWW"[c/25]
                coverage = [0]*64
                print(line)
```
A longer example can be found [here](./python-tutorial-1-depth.py)

### NumPy Integration
Librealsense frames support the buffer protocol. A numpy array can be constructed using this protocol with no data marshalling overhead:
```python
import numpy as np
depth = frames.get_depth_frame()
depth_data = depth.as_frame().get_data()
np_image = np.asanyarray(depth_data)
```