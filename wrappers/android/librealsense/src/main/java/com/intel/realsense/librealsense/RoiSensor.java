package com.intel.realsense.librealsense;

public class RoiSensor extends Sensor {

    RoiSensor(long handle) {
        super(handle);
        mOwner = false;
    }

    public void setRegionOfInterest(RegionOfInterest roi) throws Exception{
        nSetRegionOfInterest(mHandle, roi.minX, roi.minY, roi.maxX, roi.maxY);
    }

    public void setRegionOfInterest(int minX, int minY, int maxX, int maxY) throws Exception{
        nSetRegionOfInterest(mHandle, minX, minY, maxX, maxY);
    }

    public RegionOfInterest getRegionOfInterest() throws Exception {
        RegionOfInterest rv = new RegionOfInterest();
        nGetRegionOfInterest(mHandle, rv);
        return rv;
    }

    private static native void nSetRegionOfInterest(long handle, int minX, int minY, int maxX, int maxY);
    private static native void nGetRegionOfInterest(long handle, RegionOfInterest roi);
}
