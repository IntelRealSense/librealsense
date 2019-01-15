package com.intel.realsense.librealsense;

public class Frame extends LrsClass {
    private StreamProfile mStreamProfile;

    Frame(long handle){
        mHandle = handle;
        int error = 0;
//        nAddRef(handle, error);
        mStreamProfile = new StreamProfile(nGetStreamProfile(mHandle, error));
    }

    public StreamProfile getProfile() {
        return mStreamProfile;
    }

    public void getData(byte[] data) {
        nGetData(mHandle, data);
    }

    @Override
    public void close() throws Exception {
        nRelease(mHandle);
    }

    private native void nAddRef(long handle, int error);
    private native void nRelease(long handle);
    private native long nGetStreamProfile(long handle, int error);
    private native void nGetData(long handle, byte[] data);
}
