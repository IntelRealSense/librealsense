package com.intel.realsense.librealsense;

public class UpdateDevice extends Device {
    private ProgressListener mListener;
    public void update(byte[] image){
        update(image, null);
    }

    public synchronized void update(byte[] image, ProgressListener listener){
        mListener = listener;
        nUpdate(mHandle, image);
    }

    UpdateDevice(long handle){
        super(handle);
    }

    void onProgress(float progress){
        mListener.onProgress(progress);
    }

    private native void nUpdate(long handle, byte[] image);
}
