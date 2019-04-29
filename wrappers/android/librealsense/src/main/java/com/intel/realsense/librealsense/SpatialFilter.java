package com.intel.realsense.librealsense;

public class SpatialFilter extends Filter {

    public SpatialFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}