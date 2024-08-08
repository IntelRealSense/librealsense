package com.intel.realsense.librealsense;

public enum DistortionType {
    NONE(0),
    MODIFIED_BROWN_CONRADY(1),
    INVERSE_BROWN_CONRADY(2),
    FTHETA(3),
    BROWN_CONRADY(4),
    KANNALA_BRANDT4(5),
    COUNT(6);

    private final int mValue;

    DistortionType(int value) { mValue = value; }
    public int value() { return mValue; }
}
