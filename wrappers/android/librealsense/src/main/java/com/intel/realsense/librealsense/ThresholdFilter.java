package com.intel.realsense.librealsense;

public class ThresholdFilter extends Filter {

    public ThresholdFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}