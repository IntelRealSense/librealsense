package com.intel.realsense.librealsense;

public class UpdateDevice extends Device {
    private ProgressListener mListener;
    public void updateFirmware(byte[] image){
        updateFirmware(image, null);
    }

    public synchronized void updateFirmware(byte[] image, ProgressListener listener){
        mListener = listener;
        nUpdateFirmware(mHandle, image);
    }

    UpdateDevice(long handle){
        super(handle);
        mOwner = false;
    }

    void onProgress(float progress){
        mListener.onProgress(progress);
    }

    private native void nUpdateFirmware(long handle, byte[] image);
}
