package com.intel.realsense.librealsense;

public class Updatable extends Device {
    private ProgressListener mListener;

    public void enterUpdateState() {
        nEnterUpdateState(mHandle);
    }

    public synchronized byte[] createFlashBackup(ProgressListener listener){
        mListener = listener;
        return nCreateFlashBackup(mHandle);
    }

    Updatable(long handle){
        super(handle);
        mOwner = false;
    }

    void onProgress(float progress){
        mListener.onProgress(progress);
    }

    private static native void nEnterUpdateState(long handle);
    private native byte[] nCreateFlashBackup(long handle);
}
