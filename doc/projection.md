# Projection and Deprojection in librealsense

This document describes the projection mathematics relating the images provided by `librealsense` to their associated 3D coordinate systems, as well as the relationships between those coordinate systems. These facilities are mathematically equivalent to those provided by previous APIs and SDKs, but may use slightly different phrasing of coefficients and formulas.

## Table of Contents
* [Pixel Coordinates](#pixel-coordinates)
* [Point Coordinates](#point-coordinates)
* [Intrinsic Camera Parameters](#intrinsic-camera-parameters)
  * [Distortion Models](#distortion-models)
* [Extrinsic Camera Parameters](#extrinsic-camera-parameters)
* [Depth Image Formats](#depth-image-formats)
* [Appendix: Model Specific Details](#appendix-model-specific-details)
  * [F200 and SR300](#f200-and-sr300)
  * [R200](#r200)

## Pixel Coordinates

Each stream of images provided by `librealsense` is associated with a separate 2D coordinate space, specified in pixels, with the coordinate `[0,0]` referring to the center of the top left pixel in the image, and `[w-1,h-1]` referring to the center of the bottom right pixel in an image containing exactly `w` columns and `h` rows. That is, from the perspective of the camera, the x-axis points to the right and the y-axis points down. Coordinates within this space are referred to as "pixel coordinates", and are used to index into images to find the content of particular pixels.

## Point Coordinates

Each stream of images provided by `librealsense` is also associated with a separate 3D coordinate space, specified in meters, with the coordinate `[0,0,0]` referring to the center of the physical imager. Within this space, the positive x-axis points to the right, the positive y-axis points down, and the positive z-axis points forward. Coordinates within this space are referred to as "points", and are used to describe locations within 3D space that might be visible within a particular image.

## Intrinsic Camera Parameters

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
  * Projection takes a point from a stream's 3D coordinate space, and maps it to a 2D pixel location on that stream's images. It is provided by the header-only function `rs_project_point_to_pixel(...)`.
2. Deprojection
  * Deprojection takes a 2D pixel location on a stream's images, as well as a depth, specified in meters, and maps it to a 3D point location within the stream's associated 3D coordinate space. It is provided by the header-only function `rs_deproject_pixel_to_point(...)`.

Intrinsic parameters can be retrieved via a call to `rs_get_stream_intrinsics` for any stream which has been enabled with a call to `rs_enable_stream` or `rs_enable_stream_preset`. This is because the intrinsic parameters may be different depending on the resolution/aspect ratio of the requested images.

#### Distortion Models

Based on the design of each model of RealSense device, the different streams may be exposed via different distortion models.

1. None
  * An image has no distortion, as though produced by an idealized pinhole camera. This is typically the result of some hardware or software algorithm undistorting an image produced by a physical imager, but may simply indicate that the image was derived from some other image or images which were already undistorted. Images with no distortion have closed-form formulas for both projection and deprojection, and can be used with both `rs_project_point_to_pixel(...)` and `rs_deproject_pixel_to_point(...)`.
2. Modified Brown-Conrady Distortion
  * An image is distorted, and has been calibrated according to a variation of the Brown-Conrady Distortion model. This model provides a closed-form formula to map from undistorted points to distorted points, while mapping in the other direction requires iteration or lookup tables. Therefore, images with Modified Brown-Conrady Distortion can only be used with `rs_project_point_to_pixel(...)`. This model is used by the RealSense R200's color image stream.
3. Inverse Brown-Conrady Distortion
  * An image is distorted, and has been calibrated according to the inverse of the Brown-Conrady Distortion model. This model provides a closed-form formula to map from distorted points to undistored points, while mapping in the other direction requires iteration or lookup tables. Therefore, images with Inverse Brown-Conrady Distortion can only be used with `rs_deproject_pixel_to_point(...)`. This model is used by the RealSense F200 and SR300's depth and infrared image streams.

Although it is inconvenient that projection and deprojection cannot always be applied to an image, the inconvenience is minimized by the fact that RealSense devices always support calling `rs_project_deprojection from depth images, and always support projection to color images. Therefore, it is always possible to map a depth image into a set of 3D points (a point cloud), and it is always possible to discover where a 3D object would appear on the color image.

## Extrinsic Camera Parameters

The 3D coordinate systems of each stream may in general be distinct. For instance, it is common for depth to be generated from one or more infrared imagers, while the color stream is provided by a separate color imager. The relationship between the separate 3D coordinate systems of separate streams is described by their extrinsic parameters, contained in the `rs_extrinsics` struct. The basic set of assumptions is described below:

1. Imagers may be in separate locations, but are rigidly mounted on the same physical device
  * The `translation` field contains the 3D translation between the imager's physical positions, specified in meters
2. Imagers may be oriented differently, but are rigidly mounted on the same physical device
  * The `rotation` field contains a 3x3 orthonormal rotation matrix between the imager's physical orientations
3. All 3D coordinate systems are specified in meters
  * There is no need for any sort of scaling in the transformation between two coordinate systems
4. All coordinate systems are right handed and have an orthogonal basis
  * There is no need for any sort of mirroring/skewing in the transformation between two coordinate systems

Knowing the extrinsic parameters between two streams allows you to transform points from one coordinate space to another, which can be done by calling `rs_transform_point_to_point(...)`. This operation is defined as a standard affine transformation using a 3x3 rotation matrix and a 3-component translation vector.

Extrinsic parameters can be retrieved via a call to `rs_get_device_extrinsics(...)` between any two streams which are supported by the device. One does not need to enable any streams beforehand, the device extrinsics are assumed to be independent of the content of the streams' images and constant for a given device for the lifetime of the program.

## Depth Image Formats

As mentioned above, mapping from 2D pixel coordinates to 3D point coordinates via the `rs_intrinsics` structure and the `rs_deproject_pixel_to_point(...)` function requires knowledge of the depth of that pixel in meters. Certain pixel formats exposed by `librealsense` contain per-pixel depth information, and can be immediately used with this function. Other images do not contain per-pixel depth information, and thus would typically be projected into instead of deprojected from.

1. `RS_FORMAT_Z16` or `rs::format::z16`
  * Depth is stored as one unsigned 16-bit integer per pixel, mapped linearly to depth in camera-specific units. The distance, in meters, corresponding to one integer increment in depth values can be queried via `rs_get_device_depth_scale(...)`. The following pseudocode shows how to retrieve the depth of a pixel in meters:
    * `const float scale = rs_get_device_depth_scale(dev, NULL);`
    * `const uint16_t * image = (const uint16_t *)rs_get_frame_data(dev, RS_STREAM_DEPTH, NULL);`
    * `float depth_in_meters = scale * image[pixel_index];`
  * If a device fails to determine the depth of a given image pixel, a value of zero will be stored in the depth image. This is a reasonable sentinel for "no depth" because all pixels with a depth of zero would correspond to the same physical location, the location of the imager itself.
  * The default scale of an F200 or SR300 device is 1/32th of a millimeter, allowing for a maximum expressive range of two meters. However, the scale is encoded into the camera's calibration information, potentially allowing for long-range models to use a different scaling factor.
  * The default scale of an R200 device is one millimeter, allowing for a maximum expressive range of ~65 meters. The depth scale can be modified by calling `rs_set_device_option(...)` with `RS_OPTION_R200_DEPTH_UNITS`, which specifies the number of micrometers per one increment of depth. 1000 would indicate millimeter scale, 10000 would indicate centimeter scale, while 31 would roughly approximate the F200's 1/32th of a millimeter scale.

2. `RS_FORMAT_DISPARITY16` or `rs::format::disparity16`
  * Depth is stored as one unsigned 16-bit integer, as a fixed point representation of pixels of disparity. Stereo disparity is related to depth via an inverse linear relationship, and the distance of a point which registers a disparity of 1 can be queried via `rs_get_device_depth_scale(...)`. The following pseudocode shows how to retrieve the depth of a pixel in meters:
    * `const float scale = rs_get_device_depth_scale(dev, NULL);`
    * `const uint16_t * image = (const uint16_t *)rs_get_frame_data(dev, RS_STREAM_DEPTH, NULL);`
    * `float depth_in_meters = scale / image[pixel_index];`
  * Unlike `RS_FORMAT_Z16`, a disparity value of zero is meaningful. A stereo match with zero disparity will occur for objects "at infinity", objects which are so far away that the parallax between the two imagers is negligible. By contrast, there is a maximum possible disparity. The R200 only matches up to 63 pixels of disparity in hardware, and even if a software stereo search were run on an image, you would never see a disparity greater than the total width of the stereo image. Therefore, when the device fails to find a stereo match for a given pixel, a value of `0xFFFF` will be stored in the depth image as a sentinel.
  * Disparity is currently only available on the R200, which by default uses a ratio of 32 units in the disparity map to one pixel of disparity. The ratio of disparity units to pixels of disparity can be modified by calling `rs_set_device_option(...)` with `RS_OPTION_R200_DISPARITY_MULTIPLIER`. For instance, setting it to 100 would indicate that 100 units in the disparity map are equivalent to one pixel of disparity.
  
## Appendix: Model Specific Details

It is not necessary to know what model of RealSense device is plugged in to successfully make use of the projection capabilities of `librealsense`, developers can take advantage of certain known properties of given devices.

#### F200 and SR300

1. Depth images are always pixel-aligned with infrared images
  * The depth and infrared images have identical intrinsics
  * The depth and infrared images will always use the Inverse Brown-Conrady distortion model
  * The extrinsic transformation between depth and infrared is the identity transform
  * Pixel coordinates can be used interchangeably between these two streams

2. Color images have no distortion
  * When projecting to the color image on these devices, the distortion step can be skipped entirely

#### R200

1. Left and right infrared images are rectified
  * The two infrared streams have identical intrinsics
  * The two infrared streams have no distortion
  * There is no rotation between left and right infrared images (identity matrix)
  * There is translation on only one axis between left and right infrared images (`translation[1]` and `translation[2]` are zero)
  * Therefore, the `y` component of pixel coordinates can be used interchangeably between these two streams

2. Depth images are pixel aligned with the first infrared stream except for an optional 6 pixel offset
  * Native depth images are six pixels smaller on all four sides, but are otherwise pixel aligned with infrared
  * `librealsense` will pad the depth image or crop the infrared image if you request matching resolutions
  * If you request matching resolutions, depth and infrared will use the exact same intrinsics
  * If not, pixel coordinates can be mapped by adding or subtracting six pixels from both components

5. R200 color images use Modified Brown-Conrady Distortion, but can be rectified in software
  * Request frames from the rectified color stream to received images with no distortion
  * There is no rotation between depth/infrared and rectified color (identity matrix)
  * There can be translation in all three axes between depth/infrared and rectified color
  * Therefore, the `x` and `y` component of pixel coordinates can be mapped independently between depth/infrared and rectified color
