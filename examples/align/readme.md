# rs-align Sample

## Overview

This sample demonstrates usage of the `rs2::align` object, which allows users to align 2 streams (projection wise).

In this example, we align depth frames to their corresponding color frames. Then, we use the two frames to determine the depth of each color pixel.

Using this information, we "remove" the background of the color frame that is further (away from the camera) than some user-define distance.

The example display a GUI for controlling the max distance to show from the original color image.

## Expected Output

The application should open a window and display a video stream from the camera. 

The window should have the following elements:
- On the left side of the window is a verticl silder for controlling the depth clipping distance.
- A color image with grayed out background
- A corresponding (colorized) depth image.