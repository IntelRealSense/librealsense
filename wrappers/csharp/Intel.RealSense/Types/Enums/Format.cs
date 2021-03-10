// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Format identifies how binary data is encoded within a frame
    /// </summary>
    public enum Format
    {
        /// <summary>When passed to enable stream, librealsense will try to provide best suited format</summary>
        Any = 0,

        /// <summary>16-bit linear depth values. The depth is meters is equal to depth scale * pixel value.</summary>
        Z16 = 1,

        /// <summary>16-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth.</summary>
        Disparity16 = 2,

        /// <summary>32-bit floating point 3D coordinates.</summary>
        Xyz32f = 3,

        /// <summary>32-bit y0, u, y1, v data for every two pixels. Similar to YUV422 but packed in a different order - https://en.wikipedia.org/wiki/YUV</summary>
        Yuyv = 4,

        /// <summary>8-bit red, green and blue channels</summary>
        Rgb8 = 5,

        /// <summary>8-bit blue, green, and red channels -- suitable for OpenCV</summary>
        Bgr8 = 6,

        /// <summary>8-bit red, green and blue channels + constant alpha channel equal to FF</summary>
        Rgba8 = 7,

        /// <summary>8-bit blue, green, and red channels + constant alpha channel equal to FF</summary>
        Bgra8 = 8,

        /// <summary>8-bit per-pixel grayscale image</summary>
        Y8 = 9,

        /// <summary>16-bit per-pixel grayscale image</summary>
        Y16 = 10,

        /// <summary>Four 10-bit luminance values encoded into a 5-byte macropixel</summary>
        Raw10 = 11,

        /// <summary>16-bit raw image</summary>
        Raw16 = 12,

        /// <summary>8-bit raw image</summary>
        Raw8 = 13,

        /// <summary>Similar to the standard YUYV pixel format, but packed in a different order</summary>
        Uyvy = 14,

        /// <summary>Raw data from the motion sensor</summary>
        MotionRaw = 15,

        /// <summary>Motion data packed as 3 32-bit float values, for X, Y, and Z axis</summary>
        MotionXyz32f = 16,

        /// <summary>Raw data from the external sensors hooked to one of the GPIO's</summary>
        GpioRaw = 17,

        /// <summary>Pose data packed as floats array, containing translation vector, rotation quaternion and prediction velocities and accelerations vectors</summary>
        SixDOF = 18,

        /// <summary>32-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth</summary>
        Disparity32 = 19,

        /// <summary>16-bit per-pixel grayscale image unpacked from 10 bits per pixel packed ([8:8:8:8:2222]) grey-scale image. The data is unpacked to LSB and padded with 6 zero bits</summary>
        Y10BPack = 20,

        /// <summary>32-bit float-point depth distance value.</summary>
        Distance = 21,

        /// <summary>Bitstream encoding for video in which an image of each frame is encoded as JPEG-DIB.</summary>
        Mjpeg = 22,

        /// <summary>8-bit per pixel interleaved. 8-bit left, 8-bit right.</summary>
        Y8i = 23,

        /// <summary>12-bit per pixel interleaved. 12-bit left, 12-bit right. Each pixel is stored in a 24-bit word in little-endian order.</summary>
        Y12I = 24,

        /// <summary>multi-planar Depth 16bit + IR 10bit.</summary>
        Inzi = 25,

        /// <summary>8-bit IR stream.</summary>
        Invi = 26,

        /// <summary>Grey-scale image as a bit-packed array. 4 pixel data stream taking 5 bytes.</summary>
        W10 = 27,

        /// <summary>Variable-length Huffman-compressed 16-bit depth values.</summary>
        Z16H = 28
    }
}
