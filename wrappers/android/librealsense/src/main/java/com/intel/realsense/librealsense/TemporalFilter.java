package com.intel.realsense.librealsense;

public class TemporalFilter extends Filter {

    public TemporalFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}