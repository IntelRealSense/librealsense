package com.intel.realsense.librealsense;

import java.util.ArrayList;
import java.util.List;

public class Sensor extends Options {

    Sensor(long h) {
        mHandle = h;
    }

    public List<StreamProfile> getStreamProfiles(){
        long[] streamProfilesHandles = nGetStreamProfiles(mHandle);
        List<StreamProfile> rv = new ArrayList<>();
        for(long h : streamProfilesHandles){
            rv.add(new StreamProfile(h));
        }
        return rv;
    }

    public <T extends Sensor> T as(Extension extension) throws RuntimeException {
        if (this.is(extension)) {
            switch (extension){
                case ROI: return (T) new RoiSensor(mHandle);
                case DEPTH_SENSOR: return (T) new DepthSensor(mHandle);
                case COLOR_SENSOR: return (T) new ColorSensor(mHandle);
                default: throw new RuntimeException("this API version does not support " + extension.name());
            }
        } else{
            throw new RuntimeException("this sensor is not extendable to " + extension.name());
        }
    }

    public boolean is(Extension extension) {
        return nIsSensorExtendableTo(mHandle, extension.value());
    }

    @Override
    public void close() {
        if(mOwner)
            nRelease(mHandle);
    }

    private static native long[] nGetStreamProfiles(long handle);
    private static native void nRelease(long handle);
    private static native boolean nIsSensorExtendableTo(long handle, int extension);
}
