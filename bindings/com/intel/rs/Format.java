package com.intel.rs;

public enum Format
{
    ANY(0),
    Z16(1), // 16 bit linear depth values. The depth is meters is equal to depth scale * pixel value
    XYZ32F(2), // 32 bit floating point 3D coordinates. The depth in meters is equal to the Z coordinate
    DISPARITY16(3), // 16 bit linear disparity values. The depth in meters is equal to depth scale / pixel value
    YUYV(4),
    RGB8(5),
    BGR8(6),
    RGBA8(7),
    BGRA8(8),
    Y8(9),
    Y16(10),
    RAW10(11); // Four 10-bit luminance values encoded into a 5-byte macropixel
    public final int code;
    private Format(int code) { this.code = code; }
}
