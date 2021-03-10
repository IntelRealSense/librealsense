package com.intel.realsense.librealsense;

public class HoleFillingFilter extends Filter {

    public HoleFillingFilter(){
        mHandle = nCreate(mQueue.getHandle());
    }

    private static native long nCreate(long queueHandle);
}