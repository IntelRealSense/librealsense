# Projection and Deprojection in librealsense

This document describes the projection mathematics relating the images provided by librealsense to their associated 3D coordinate systems, as well as the relationships between those coordinate systems.

## Pixel coordinates

librealsense defines a separate pixel coordinate space for each image, with the coordinate [0,0] referring to the center of the top left pixel in the image, and [w-1,h-1] referring to the center of the bottom right pixel in an image containing exactly w columns and h rows. That is, from the perspective of the camera, the x-axis points to the right and the y-axis points down.

## Point coordinates

librealsense defines a separate 3D coordinate space for each image, with the coordinate [0,0,0] referring to the center of the physical imager. Within this space, the positive x-axis points to the right, the positive y-axis points down, and the positive z-axis points forward.
