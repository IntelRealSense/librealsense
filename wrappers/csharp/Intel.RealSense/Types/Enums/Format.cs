﻿using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum Format
    {
        ///<summary>When passed to enable stream, librealsense will try to provide best suited format</summary>
        Any = 0,
        ///<summary>16-bit linear depth values. The depth is meters is equal to depth scale * pixel value.</summary>
        Z16 = 1,
        ///<summary>16-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth.</summary>
        Disparity16 = 2,
        ///<summary>32-bit floating point 3D coordinates.</summary>
        Xyz32f = 3,
        ///<summary>32-bit y0, u, y1, v data for every two pixels. Similar to YUV422 but packed in a different order - https://en.wikipedia.org/wiki/YUV</summary>
        Yuyv = 4,
        ///<summary>8-bit red, green and blue channels</summary>
        Rgb8 = 5,
        ///<summary>8-bit blue, green, and red channels -- suitable for OpenCV</summary>
        Bgr8 = 6,
        ///<summary>8-bit red, green and blue channels + constant alpha channel equal to FF</summary>
        Rgba8 = 7,
        ///<summary>8-bit blue, green, and red channels + constant alpha channel equal to FF</summary>
        Bgra8 = 8,
        ///<summary>8-bit per-pixel grayscale image</summary>
        Y8 = 9,
        ///<summary>16-bit per-pixel grayscale image</summary>
        Y16 = 10,
        ///<summary>Four 10-bit luminance values encoded into a 5-byte macropixel</summary>
        Raw10 = 11,
        ///<summary>16-bit raw image</summary>
        Raw16 = 12,
        ///<summary>8-bit raw image</summary>
        Raw8 = 13,
        ///<summary>Similar to the standard YUYV pixel format, but packed in a different order</summary>
        Uyvy = 14,
        ///<summary>Raw data from the motion sensor</summary>
        MotionRaw = 15,
        ///<summary>Motion data packed as 3 32-bit float values, for X, Y, and Z axis</summary>
        MotionXyz32f = 16,
        ///<summary>Raw data from the external sensors hooked to one of the GPIO's</summary>
        GpioRaw = 17,
        ///<summary>Pose data packed as floats array, containing translation vector, rotation quaternion and prediction velocities and accelerations vectors</summary>
        SixDOF = 18,
        ///<summary>32-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth</summary>
        Disparity32 = 19,
    }
}
