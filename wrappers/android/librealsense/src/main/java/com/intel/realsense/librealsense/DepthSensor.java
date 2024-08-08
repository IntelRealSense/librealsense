package com.intel.realsense.librealsense;

public class DepthSensor extends Sensor {

    DepthSensor(long handle) {
        super(handle);
        mOwner = false;
    }

    public float getDepthScale()  { return nGetDepthScale(mHandle); }


    private static native float nGetDepthScale(long handle);
}

