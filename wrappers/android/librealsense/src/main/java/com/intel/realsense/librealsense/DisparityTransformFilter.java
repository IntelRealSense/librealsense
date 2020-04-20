package com.intel.realsense.librealsense;

public class DisparityTransformFilter extends Filter {

    public DisparityTransformFilter(boolean transformToDisparity){
        mHandle = nCreate(mQueue.getHandle(), transformToDisparity);
    }

    private static native long nCreate(long queueHandle, boolean transformToDisparity);
}