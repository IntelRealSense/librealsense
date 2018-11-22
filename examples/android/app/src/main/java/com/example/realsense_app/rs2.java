package com.example.realsense_app;

public class rs2 {
    public enum stream{
        RS2_STREAM_ANY,
        RS2_STREAM_DEPTH     , /**< Native stream of depth data produced by RealSense device */
        RS2_STREAM_COLOR     , /**< Native stream of color data captured by RealSense device */
        RS2_STREAM_INFRARED  , /**< Native stream of infrared data captured by RealSense device */
        RS2_STREAM_FISHEYE   , /**< Native stream of fish-eye (wide) data captured from the dedicate motion camera */
        RS2_STREAM_GYRO      , /**< Native stream of gyroscope motion data produced by RealSense device */
        RS2_STREAM_ACCEL     , /**< Native stream of accelerometer motion data produced by RealSense device */
        RS2_STREAM_GPIO      , /**< Signals from external device connected through GPIO */
        RS2_STREAM_POSE      , /**< 6 Degrees of Freedom pose data, calculated by RealSense device */
        RS2_STREAM_CONFIDENCE,
        RS2_STREAM_COUNT;

        @Override
        public String toString() {
            return StreamToString(this.ordinal());
        }

        public static stream from(String s) {
            return stream.values()[StreamFromString(s)];
        }
    }

    public enum format {
        RS2_FORMAT_ANY           , /**< When passed to enable stream, librealsense will try to provide best suited format */
        RS2_FORMAT_Z16           , /**< 16-bit linear depth values. The depth is meters is equal to depth scale * pixel value. */
        RS2_FORMAT_DISPARITY16   , /**< 16-bit linear disparity values. The depth in meters is equal to depth scale / pixel value. */
        RS2_FORMAT_XYZ32F        , /**< 32-bit floating point 3D coordinates. */
        RS2_FORMAT_YUYV          , /**< Standard YUV pixel format as described in https://en.wikipedia.org/wiki/YUV */
        RS2_FORMAT_RGB8          , /**< 8-bit red, green and blue channels */
        RS2_FORMAT_BGR8          , /**< 8-bit blue, green, and red channels -- suitable for OpenCV */
        RS2_FORMAT_RGBA8         , /**< 8-bit red, green and blue channels + constant alpha channel equal to FF */
        RS2_FORMAT_BGRA8         , /**< 8-bit blue, green, and red channels + constant alpha channel equal to FF */
        RS2_FORMAT_Y8            , /**< 8-bit per-pixel grayscale image */
        RS2_FORMAT_Y16           , /**< 16-bit per-pixel grayscale image */
        RS2_FORMAT_RAW10         , /**< Four 10-bit luminance values encoded into a 5-byte macropixel */
        RS2_FORMAT_RAW16         , /**< 16-bit raw image */
        RS2_FORMAT_RAW8          , /**< 8-bit raw image */
        RS2_FORMAT_UYVY          , /**< Similar to the standard YUYV pixel format, but packed in a different order */
        RS2_FORMAT_MOTION_RAW    , /**< Raw data from the motion sensor */
        RS2_FORMAT_MOTION_XYZ32F , /**< Motion data packed as 3 32-bit float values, for X, Y, and Z axis */
        RS2_FORMAT_GPIO_RAW      , /**< Raw data from the external sensors hooked to one of the GPIO's */
        RS2_FORMAT_6DOF          , /**< Pose data packed as floats array, containing translation vector, rotation quaternion and prediction velocities and accelerations vectors */
        RS2_FORMAT_DISPARITY32   , /**< 32-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth */
        RS2_FORMAT_COUNT         ; /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */

        @Override
        public String toString() {
            return FormatToString(this.ordinal());
        }

        public static format from(String s) {
            return format.values()[FormatFromString(s)];
        }
    }

    public native static String StreamToString(int s);
    public native static String FormatToString(int f);

    public native static int StreamFromString(String s);
    public native static int FormatFromString(String f);

}
