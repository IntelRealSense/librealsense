package com.intel.realsense.librealsense;

public class Updatable extends Device {
    private ProgressListener mListener;

    public void enterUpdateState() {
        nEnterUpdateState(mHandle);
    }

    public synchronized void updateUnsigned(byte[] image, ProgressListener listener){
        mListener = listener;
        nUpdateFirmwareUnsigned(mHandle, image, 0);
    }

    public synchronized byte[] createFlashBackup(ProgressListener listener){
        mListener = listener;
        return nCreateFlashBackup(mHandle);
    }

    public synchronized byte[] createFlashBackup(){
        mListener = new ProgressListener() {
            @Override
            public void onProgress(float progress) {
            }
        };
        return createFlashBackup(mListener);
    }

    public boolean checkFirmwareCompatibility(byte[] image) { return nCheckFirmwareCompatibility(mHandle, image);}

    Updatable(long handle){
        super(handle);
        mOwner = false;
    }

    void onProgress(float progress){
        mListener.onProgress(progress);
    }

    private static native void nEnterUpdateState(long handle);
    private native byte[] nCreateFlashBackup(long handle);
    private native void nUpdateFirmwareUnsigned(long handle, byte[] image, int update_mode);
    private native boolean nCheckFirmwareCompatibility(long handle, byte[] image);
}
