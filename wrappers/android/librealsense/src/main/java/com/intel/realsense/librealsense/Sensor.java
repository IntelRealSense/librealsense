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

    public <T extends Sensor> T as(Extension extension) {
        switch (extension){
            case ROI: return (T) new RoiSensor(mHandle);
        }
        throw new RuntimeException("this sensor is not extendable to " + extension.name());
    }

    @Override
    public void close() {
        if(mOwner)
            nRelease(mHandle);
    }

    private static native long[] nGetStreamProfiles(long handle);
    private static native void nRelease(long handle);
}
