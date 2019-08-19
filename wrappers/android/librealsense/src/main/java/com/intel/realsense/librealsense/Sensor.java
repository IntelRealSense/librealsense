package com.intel.realsense.librealsense;

import java.util.ArrayList;
import java.util.List;

public class Sensor extends LrsClass {

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

    @Override
    public void close() {
        nRelease(mHandle);
    }

    private static native long[] nGetStreamProfiles(long handle);
    private static native void nRelease(long handle);
}
