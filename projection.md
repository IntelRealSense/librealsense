# Projection and Deprojection in librealsense

This document describes the projection mathematics relating the images provided by librealsense to their associated 3D coordinate systems, as well as the relationships between those coordinate systems.

## Pixel coordinates

Each stream of images provided by librealsense is associated with a separate 2D coordinate space, specified in pixels, with the coordinate `[0,0]` referring to the center of the top left pixel in the image, and `[w-1,h-1]` referring to the center of the bottom right pixel in an image containing exactly `w` columns and `h` rows. That is, from the perspective of the camera, the x-axis points to the right and the y-axis points down. Coordinates within this space are referred to as "pixel coordinates", and are used to index into images to find the content of particular pixels.

## Point coordinates

Each stream of images provided by librealsense is also associated with a separate 3D coordinate space, specified in meters, with the coordinate `[0,0,0]` referring to the center of the physical imager. Within this space, the positive x-axis points to the right, the positive y-axis points down, and the positive z-axis points forward. Coordinates within this space are referred to as "points", and are used to describe locations within 3D space that might be visible within a particular image.

## Intrinsic camera parameters

The relationship between a stream's 2D and 3D coordinate systems is described by its intrinsic camera parameters, contained in the `rs_intrinsics` struct. Each model of RealSense device is somewhat different, and the `rs_intrinsics` struct must be capable of describing the images produced by all of them. The basic set of assumptions is described below:

1. Images may be of arbitrary size
  * The `width` and `height` fields describe the number of rows and columns in the image, respectively
2. The field of view of an image may vary
  * The `fx` and `fy` fields describe the focal length of the image, as a multiple of pixel width and height
3. The pixels of an image are not necessarily square
  * The `fx` and `fy` fields are allowed to be different (though they are commonly close)
4. The center of projection is not necessarily the center of the image
  * The `ppx` and `ppy` fields describe the pixel coordinates of the principal point (center of projection)
5. The image may contain distortion
  * The `model` field describes which of several supported distortion models was used to calibrate the image, and the `coeffs` field provides an array of up to five coefficients describing the distortion model

Knowing the intrinsic camera parameters of an images allows you to carry out two fundamental mapping operations.

1. Projection
  * Projection takes a point from a stream's 3D coordinate space, and maps it to a 2D pixel location on that stream's images. It is provided by the header-only function `rs_project_point_to_pixel`.
2. Deprojection
  * Deprojection takes a 2D pixel location on a stream's images, as well as a depth, specified in meters, and maps it to a 3D point location within the stream's associated 3D coordinate space. It is provided by the header-only function `rs_deproject_pixel_to_point`.

### Distortion models

Based on the design of each model of RealSense device, the different streams may be exposed via different distortion models.
