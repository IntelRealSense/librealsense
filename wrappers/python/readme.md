# Python Wrapper

## Table of Contents
* [Installation](#installation)
* [Building from source](#building-from-source)
* [Examples](#examples)

## Installation

> **Note:**
>
>[`pyrealsense`](https://github.com/toinsson/pyrealsense) AKA [`pyrealsense/2.0`](https://pypi.python.org/pypi/pyrealsense/2.0) is a community supported Python wrapper for the legacy **librealsense v1.12.1**. This wrapper does not support newer versions and **does not work with the RealSense SDK 2.0**.
>
> HOWEVER: The [`pyrealsense2`](https://pypi.org/project/pyrealsense2/) package is our official wrapper which **does** support SDK 2.0

We provide a PyPI distribution which is created from this folder by running `python setup.py bdist_wheel`.

Package is available at https://pypi.python.org/pypi/pyrealsense2

To install the package, run:
> `pip install pyrealsense2`

Windows users can install the RealSense SDK 2.0 from the release tab to get pre-compiled binaries of the wrapper, for both x86 and x64 architectures. (Both Python 2.7 and Python 3 (3.6, 3.7, 3.8, 3.9) are supported).


## Building From Source

#### Ubuntu 14.04/16.04 LTS
1. Ensure apt-get is up to date
  * `sudo apt-get update && sudo apt-get upgrade`
  * **Note:** Use `sudo apt-get dist-upgrade`, instead of `sudo apt-get upgrade`, in case you have an older Ubuntu 14.04 version
2. Install Python and its development files via apt-get (Python 2 and 3 both work)
  * `sudo apt-get install python python-dev` or `sudo apt-get install python3 python3-dev`
  * **Note:** The project will only use Python 2 if it can't use Python 3
3. Run the top level CMake command with the following additional flag `-DBUILD_PYTHON_BINDINGS:bool=true`:
  * `mkdir build`
  * `cd build`
  * `cmake ../ -DBUILD_PYTHON_BINDINGS:bool=true`
> **Note**: To force compilation with a specific version on a system with both Python 2 and Python 3 installed, add the following flag to CMake command:
`-DPYTHON_EXECUTABLE=[full path to the exact python executable]`
  * `make -j4`
  * `sudo make install`
4. update your PYTHONPATH environment variable to add the path to the pyrealsense library
  * `export PYTHONPATH=$PYTHONPATH:/usr/local/lib`
5. Alternatively, copy the build output (`librealsense2.so` and `pyrealsense2.so`) next to your script.
  * **Note:** Python 3 module filenames may contain additional information, e.g. `pyrealsense2.cpython-35m-arm-linux-gnueabihf.so`)



#### Windows
1. Install Python 2 or 3 for windows. You can find the downloads on the official Python website [here](https://www.python.org/downloads/windows/)
2. When running `cmake-gui`, select the `BUILD_PYTHON_BINDINGS` option
3. If you have multiple python installations on your machine you can use: `-DPYTHON_EXECUTABLE=<path to python executable>`
For example: `-DPYTHON_EXECUTABLE=C:/Python27/python.exe`
> The precompiled binaries shipped with [the installer](https://github.com/IntelRealSense/librealsense/releases) assume **Python 2.7**.
>The error `ImportError: DLL load failed: The specified module could not be found` might indicate versions mismatch or architecture (x86 vs x64) mismatch.

4. Open `librealsense2.sln` that was created in the previous step, and build the `pyrealsense2` project
5. Open the output folder of the project (e.g `build\x64-Release\Release\`) and copy `pyrealsense2.pyd` into your python's `site-packages` (e.g `C:\Python27\Lib\site-packages`)
6. Alternatively, copy the build output (`realsense2.dll` and `pyrealsense2.pyd`) next to your script.

## Examples

> For a list of full code examples see the [examples](./examples) folder

#### Streaming using _rs.pipeline_
```python
# First import the library
import pyrealsense2 as rs

# Create a context object. This object owns the handles to all connected realsense devices
pipeline = rs.pipeline()
pipeline.start()

try:
    while True:
        # Create a pipeline object. This object configures the streaming camera and owns it's handle
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

finally:
    pipeline.stop()
```

#### NumPy Integration
Librealsense frames support the buffer protocol. A numpy array can be constructed using this protocol with no data marshalling overhead:
```python
import numpy as np
depth = frames.get_depth_frame()
depth_data = depth.as_frame().get_data()
np_image = np.asanyarray(depth_data)
```
