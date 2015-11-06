package com.intel.rs;

public enum Format
{
    ANY(0),
    Z16(1),
    YUYV(2),
    RGB8(3),
    BGR8(4),
    RGBA8(5),
    BGRA8(6),
    Y8(7),
    Y16(8),
    RAW10(9); // Four 10-bit luminance values encoded into a 5-byte macropixel
    public final int code;
    private Format(int code) { this.code = code; }
}
