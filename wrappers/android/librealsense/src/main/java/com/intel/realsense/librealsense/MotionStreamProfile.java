package com.intel.realsense.librealsense;

public class MotionStreamProfile extends StreamProfile {
    MotionStreamProfile(long handle) {
        super(handle);
        mOwner = false;
    }
}
