package com.intel.realsense.librealsense;

public class ZeroOrderInvalidationFilter extends Filter {

    public ZeroOrderInvalidationFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}