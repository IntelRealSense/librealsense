package com.intel.realsense.librealsense;

public class MotionFrame extends Frame {
    protected MotionFrame(long handle) {
        super(handle);
        mOwner = false;
    }
}
