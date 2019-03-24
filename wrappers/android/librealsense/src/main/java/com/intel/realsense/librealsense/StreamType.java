package com.intel.realsense.librealsense;

public enum StreamType {
    ANY(0),
    DEPTH(1),
    COLOR(2),
    INFRARED(3),
    FISHEYE(4);

    private final int mValue;

    private StreamType(int value) { mValue = value; }
    public int value() { return mValue; }
}
