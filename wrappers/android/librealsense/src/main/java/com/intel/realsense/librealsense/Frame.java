package com.intel.realsense.librealsense;

public class Frame extends LrsClass implements Cloneable{

    Frame(long handle){
        mHandle = handle;
    }

    public boolean is(Extension extension) {
        return nIsFrameExtendableTo(mHandle, extension.value());
    }

    public <T extends Frame> T as(Extension extension) {
        switch (extension){
            case VIDEO_FRAME: return (T) new VideoFrame(mHandle);
            case DEPTH_FRAME: return (T) new DepthFrame(mHandle);
            case MOTION_FRAME: return (T) new MotionFrame(mHandle);
            case POINTS: return (T) new Points(mHandle);
        }
        throw new RuntimeException("this profile is not extendable to " + extension.name());
    }

    public StreamProfile getProfile() {
        return new StreamProfile(nGetStreamProfile(mHandle));
    }

    public int getDataSize() {
        return nGetDataSize(mHandle);
    }

    public void getData(byte[] data) {
        nGetData(mHandle, data);
    }

    public int getNumber(){
        return nGetNumber(mHandle);
    }

    public double getTimestamp(){
        return nGetTimestamp(mHandle);
    }

    public TimestampDomain getTimestampDomain() {
        int rv = nGetTimestampDomain(mHandle);
        return TimestampDomain.values()[rv];
    }

    public boolean supportsMetadata(FrameMetadata type) { return nSupportsMetadata(mHandle, type.value());}

    public long getMetadata(FrameMetadata type) { return nGetMetadata(mHandle, type.value());}

    public Frame applyFilter(FilterInterface filter) {
        return filter.process(this);
    }

    public Frame releaseWith(FrameReleaser frameReleaser){
        frameReleaser.addFrame(this);
        return this;
    }

    @Override
    public void close() {
        if(mOwner)
            nRelease(mHandle);
    }

    @Override
    public Frame clone() {
        Frame rv = new Frame(mHandle);
        nAddRef(mHandle);
        return rv;
    }

    private static native boolean nIsFrameExtendableTo(long handle, int extension);
    private static native void nAddRef(long handle);
    private static native void nRelease(long handle);
    protected static native long nGetStreamProfile(long handle);
    private static native int nGetDataSize(long handle);
    private static native void nGetData(long handle, byte[] data);
    private static native int nGetNumber(long handle);
    private static native double nGetTimestamp(long handle);
    private static native int nGetTimestampDomain(long handle);
    private static native long nGetMetadata(long handle, int metadata_type);
    private static native boolean nSupportsMetadata(long handle, int metadata_type);
}
