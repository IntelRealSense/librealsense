![](doc/planes.png)

[![image](https://img.shields.io/pypi/v/scikit-spatial.svg)](https://pypi.python.org/pypi/scikit-spatial)


# Introduction

This code provides examples of the using RealSense camera for detection of RGB and Depth objects.
The following objects are supported:

-   Planes : detected multiple planes in the depth image
-   Edges  : detecting edges / intersection of planes 
-   Corners: three plane intersection/ junctions

These objects could be integrated in your robotics and video processing pipe line.

Plane | Multi Plane | Fast Plane |
:------------: |  :----------: | :-------------:  |
[![Planes](planes/doc/planes_2.gif)](planes/README.md)  | [![Edges](planes/doc/planes_3.gif)](planes/README.md)  | [![Corners](hplanes/doc/planes_4.gif)](planes/README.md)  |



## Installation Windows

1. Install python 3.10 from Python Release Python 3.10.0 | <https://www.python.org>

2. Create virtual environment. In Windows PowerShell:

    python -m venv <your path>\Envs\planes

3. Activate virtual environment. In Windows CMD shell:

    <your path>\Envs\planes\Scripts\activate.bat

4. Installing realsense driver. For example, download pyrealsense2-2.55.10.6089-cp310-cp310-win_amd64.whl:

    pip install pyrealsense2-2.55.10.6089-cp310-cp310-win_amd64.whl

5. Install opencv and numpy:

    pip install opencv-contrib-python



## Usage

Importing the code:

```py
>>> from plane_detector import PlaneDetector

>>> pd = PlaneDetector()

```

Running tests:

```py
>>> python test_plane_detector.py

```