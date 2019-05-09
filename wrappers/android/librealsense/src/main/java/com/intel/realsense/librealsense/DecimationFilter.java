package com.intel.realsense.librealsense;

public class DecimationFilter extends Filter {

    public DecimationFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}
