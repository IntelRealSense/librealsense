package com.intel.realsense.librealsense;

public enum TimestampDomain {
    HARDWARE_CLOCK(0),
    SYSTEM_TIME(1),
    GLOBAL_TIME(2);

    private final int mValue;

    private TimestampDomain(int value) { mValue = value; }
    public int value() { return mValue; }
}
